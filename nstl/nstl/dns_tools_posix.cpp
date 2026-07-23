#include "nstl/dns_tools.hpp"
#include "nstl/macros.hpp"
#include "nstl/scope_exit.hpp"

#include <stdexcept>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <array>
#include <functional>
#include <cstring>

#include <string.h>

namespace nstl::net
{
namespace
{
class DnsClient
{
    static constexpr size_t buff_size = 4096;
    std::array<unsigned char, buff_size> _response;
    struct __res_state _state;

    bool _process_item(const char* name_, const int ns_type_, const std::function<void (ns_msg& message, int count)>& func_)
    {
        NSTL_THROW_EXCEPTION_IF(!name_, std::invalid_argument, "name_ cannot be nullptr");
        const auto length = res_nquery(&_state, name_, ns_c_in, ns_type_, _response.data(), _response.size());
        if (length <= 0) {
            return false;
        }
        NSTL_THROW_EXCEPTION_IF(_response.size() < static_cast<size_t>(length), std::runtime_error, length << " is greater than " << _response.size());
        ns_msg message;
        NSTL_THROW_EXCEPTION_IF(ns_initparse(_response.data(), length, &message) < 0, std::runtime_error, "ns_initparse failed");

        const int count = ns_msg_count(message, ns_s_an);

        func_(message, count);
        return true;
    }

public:
    static DnsClient& instance()
    {
        thread_local DnsClient ins;
        return ins;
    }

    DnsClient()
    {
        NSTL_THROW_EXCEPTION_IF(res_ninit(&_state) < 0, std::runtime_error, "Resolver initialize failed");
    }

    ~DnsClient()
    {
        res_nclose(&_state);
    }
    DnsClient(const DnsClient&) = delete;
    DnsClient& operator= (const DnsClient&) = delete;

    std::optional<std::vector<mx_srv>> mx_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_mx;
        std::optional<std::vector<mx_srv>> retval;

        return this->_process_item(name_, tgt_type, [&retval](ns_msg& message, const int count){
            retval.emplace();
            retval->reserve(count);

            for (int idx = 0; idx < count; ++idx) {
                ns_rr raw_record;
                NSTL_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0, std::runtime_error, "Failed to parse DNS message");
                // message is not MX
                if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type) {
                    continue;
                }
                const unsigned char *rdata = ns_rr_rdata(raw_record);
                const size_t rdata_length = ns_rr_rdlen(raw_record);

                if (rdata_length < NS_INT16SZ + 1) {
                    continue;
                }
                const uint16_t priority = ns_get16(rdata);
                std::array<char, NS_MAXDNAME> item_name;

                const int expanded_length = dn_expand(
                    ns_msg_base(message),
                    ns_msg_end(message),
                    rdata + NS_INT16SZ,
                    item_name.data(),
                    item_name.size()
                );
                NSTL_THROW_EXCEPTION_IF(expanded_length < 0, std::runtime_error, "invalid response");

                mx_srv item{.address = std::string{item_name.data(), strnlen(item_name.data(), static_cast<size_t>(expanded_length))}, .priority = priority};
                retval->push_back(std::move(item));
            }
        }) ? retval : std::nullopt;
    }

    std::optional<std::vector<std::string>> txt_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_txt;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type, [&retval](ns_msg& message, const int count){
            retval.emplace();
            retval->reserve(count);

            for (int idx = 0; idx < count; ++idx) {
                ns_rr raw_record;
                NSTL_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0, std::runtime_error, "Failed to parse DNS message");
                // message is not TXT
                if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type) {
                    continue;
                }
                const auto rdata = ns_rr_rdata(raw_record);
                const size_t rdata_length = ns_rr_rdlen(raw_record);

                const auto end = rdata + rdata_length;

                std::string item;

                for (auto p = rdata, next_p = p; p < end; p = next_p) {
                    const std::uint8_t text_len = *p;
                    ++p;
                    next_p = p + text_len;
                    NSTL_THROW_EXCEPTION_IF(end < next_p, std::runtime_error, "Invalid TXT record");
                    item.append(reinterpret_cast<const char*>(p), text_len);
                }
                retval->push_back(std::move(item));
            }
        }) ? retval : std::nullopt;
    }

    std::optional<std::vector<std::string>> c_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_cname;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type, [&retval](ns_msg& message, const int count){
            retval.emplace();
            retval->reserve(count);

            for (int idx = 0; idx < count; ++idx) {
                ns_rr raw_record;
                NSTL_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0, std::runtime_error, "Failed to parse DNS message");
                // message is not TXT
                if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type) {
                    continue;
                }
                const auto rdata = ns_rr_rdata(raw_record);
                std::array<char, NS_MAXDNAME> item_name;

                const int expanded_length = dn_expand(
                    ns_msg_base(message),
                    ns_msg_end(message),
                    rdata,
                    item_name.data(),
                    item_name.size()
                );
                NSTL_THROW_EXCEPTION_IF(expanded_length < 0, std::runtime_error, "invalid response");
                std::string cname{item_name.data(), strnlen(item_name.data(), static_cast<size_t>(expanded_length))};
                retval->emplace_back(std::move(cname));
            }
        }) ? retval : std::nullopt;
    }
};
}

std::optional<std::vector<mx_srv>> mx_name(const char* name_)
{
    return DnsClient::instance().mx_name(name_);
}

std::optional<std::vector<std::string>> txt_name(const char* name_)
{
    return DnsClient::instance().txt_name(name_);
}

std::optional<std::vector<std::string>> c_name(const char* name_)
{
    return DnsClient::instance().c_name(name_);
}
}
