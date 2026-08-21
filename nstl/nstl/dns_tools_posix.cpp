#include "dns_tools.hpp"
#include "exception.hpp"
#include "scope_exit.hpp"

#include <stdexcept>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <array>
#include <functional>
#include <cstring>
#include <vector>

#include <string.h>

namespace nstl::net
{
namespace
{
enum SvcParamKey : uint16_t
{
    KEY_MANDATORY = 0,
    KEY_ALPN = 1,
    KEY_NO_DEF_ALPN = 2,
    KEY_PORT = 3,
    KEY_IPV4HINT = 4,
    KEY_ECH = 5,
    KEY_IPV6HINT = 6,
    KEY_DOH_PATH = 7,
};

std::vector<svcb_param> resolve_params(const uint8_t* p, const uint8_t* end)
{
    std::vector<svcb_param> retval;
    while (p + 4 <= end)
    {
        const std::uint16_t key = ns_get16(p);
        const std::uint16_t len = ns_get16(p + 2);
        p += 4;
        if (end < p + len)
        {
            return retval;
        }
        const uint8_t* v = p;

        switch (key)
        {
        case KEY_ALPN:
        {
            std::vector<std::string> alpns;

            const uint8_t* q = v;
            const uint8_t* vend = v + len;
            while (q < vend)
            {
                const uint8_t l = *q++;
                NSTL2_THROW_EXCEPTION_IF(vend < q + l, "Invalid ALPN request");
                std::string value{ reinterpret_cast<const char*>(q), l };
                alpns.push_back(std::move(value));
                q += l;
            }
            retval.emplace_back(std::move(alpns));
            break;
        }
        case KEY_PORT:
            NSTL2_THROW_EXCEPTION_IF(len != 2, "Invalid port keys size");
            if (len == 2)
            {
                std::uint16_t port = ns_get16(v);
                retval.emplace_back(port);
            }
            break;
        case KEY_MANDATORY:
        {
            NSTL2_THROW_EXCEPTION_IF(len % 2 != 0, "Invalid mandatory keys size");
            std::vector<std::uint16_t> keys;
            for (std::uint16_t idx = 0; idx < len; idx += 2)
            {
                keys.push_back(ns_get16(v + idx));
            }
            retval.emplace_back(std::move(keys));
            break;
        }
        case KEY_IPV4HINT:
        {
            NSTL2_THROW_EXCEPTION_IF(len % 4 != 0, "Invalid IPV4HINTS size");
            std::vector<std::uint32_t> ipv4s;

            for (std::uint16_t idx = 0; idx < len; idx += 4)
            {
                std::uint32_t ipv4 = 0;
                std::memcpy(&ipv4, v + idx, 4);
                ipv4s.push_back(ipv4);
            }
            retval.emplace_back(std::move(ipv4s));
            break;
        }
        case KEY_IPV6HINT:
        {
            NSTL2_THROW_EXCEPTION_IF(len % 16 != 0, "Invalid IPV6HINTS size");
            std::vector<std::array<std::uint8_t, 16>> ipv6s;
            for (std::uint16_t idx = 0; idx < len; idx += 16)
            {
                auto& tgt = ipv6s.emplace_back();
                std::memcpy(tgt.data(), v + idx, tgt.size());
            }
            retval.emplace_back(std::move(ipv6s));
            break;
        }
        case KEY_NO_DEF_ALPN:
            retval.emplace_back(std::monostate{});
            break;
        case KEY_DOH_PATH:
        {
            std::string doh_path{ reinterpret_cast<const char*>(v), len };
            retval.emplace_back(std::move(doh_path));
            break;
        }
        default:
            break;
        }

        p += len;
    }
    return retval;
}

class DnsClient
{
    static constexpr size_t buff_size = 4096;
    std::vector<unsigned char> _response;
    struct __res_state _state;

    bool _process_item(const char* name_, const int ns_type_,
                       const std::function<void(ns_msg& message, int count)>& func_)
    {
        NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
        int length = -1;
        while (length < 0)
        {
            length = res_nquery(&_state, name_, ns_c_in, ns_type_, _response.data(), _response.size());
            if (length <= 0)
            {
                return false;
            }
            if (_response.size() <= static_cast<size_t>(length)) [[unlikely]]
            {
                _response.resize(length + 1);
                length = -1;
            }
        }

        ns_msg message;
        NSTL2_THROW_EXCEPTION_IF(ns_initparse(_response.data(), length, &message) < 0,
                                 "ns_initparse failed (" << length << " vs " << _response.size() << ')');

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
        NSTL2_THROW_EXCEPTION_IF(res_ninit(&_state) < 0, "Resolver initialize failed");
        _response.resize(buff_size);
    }

    ~DnsClient() { res_nclose(&_state); }
    DnsClient(const DnsClient&) = delete;
    DnsClient& operator=(const DnsClient&) = delete;

    std::optional<std::vector<mx_srv>> mx_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_mx;
        std::optional<std::vector<mx_srv>> retval;

        return this->_process_item(
                   name_, tgt_type,
                   [&retval](ns_msg& message, const int count)
                   {
                       for (int idx = 0; idx < count; ++idx)
                       {
                           ns_rr raw_record;
                           NSTL2_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0,
                                                    "Failed to parse DNS message");
                           // message is not MX
                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                           {
                               continue;
                           }
                           const unsigned char* rdata = ns_rr_rdata(raw_record);
                           const size_t rdata_length = ns_rr_rdlen(raw_record);

                           if (rdata_length < NS_INT16SZ + 1)
                           {
                               continue;
                           }
                           const uint16_t priority = ns_get16(rdata);
                           std::array<char, NS_MAXDNAME> item_name;

                           const int expanded_length =
                               dn_expand(ns_msg_base(message), ns_msg_end(message), rdata + NS_INT16SZ,
                                         item_name.data(), item_name.size());
                           NSTL2_THROW_EXCEPTION_IF(expanded_length < 0, "invalid response");

                           mx_srv item{ .address = std::string{ item_name.data(),
                                                                strnlen(item_name.data(),
                                                                        static_cast<size_t>(expanded_length)) },
                                        .priority = priority };
                           if (!retval.has_value())
                           {
                               retval.emplace();
                           }
                           retval->push_back(std::move(item));
                       }
                   })
                   ? retval
                   : std::nullopt;
    }

    std::optional<std::vector<std::string>> txt_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_txt;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           NSTL2_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0,
                                                                    "Failed to parse DNS message");
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           const size_t rdata_length = ns_rr_rdlen(raw_record);

                                           const auto end = rdata + rdata_length;

                                           std::string item;

                                           for (auto p = rdata, next_p = p; p < end; p = next_p)
                                           {
                                               const std::uint8_t text_len = *p;
                                               ++p;
                                               next_p = p + text_len;
                                               NSTL2_THROW_EXCEPTION_IF(end < next_p, "Invalid TXT record");
                                               item.append(reinterpret_cast<const char*>(p), text_len);
                                           }
                                           if (!retval.has_value())
                                           {
                                               retval.emplace();
                                           }
                                           retval->push_back(std::move(item));
                                       }
                                   })
                   ? retval
                   : std::nullopt;
    }

    std::optional<std::vector<std::string>> c_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_cname;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           NSTL2_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0,
                                                                    "Failed to parse DNS message");
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           std::array<char, NS_MAXDNAME> item_name;

                                           const int len = dn_expand(ns_msg_base(message), ns_msg_end(message), rdata,
                                                                     item_name.data(), item_name.size());
                                           NSTL2_THROW_EXCEPTION_IF(len < 0, "invalid response");
                                           std::string cname{ item_name.data(),
                                                              strnlen(item_name.data(), static_cast<size_t>(len)) };
                                           if (!retval.has_value())
                                           {
                                               retval.emplace();
                                           }
                                           retval->emplace_back(std::move(cname));
                                       }
                                   })
                   ? retval
                   : std::nullopt;
    }

    std::optional<std::vector<gen_srv>> srv_name(const char* name_)
    {
        constexpr int tgt_type = ns_t_srv;
        std::optional<std::vector<gen_srv>> retval;

        return this->_process_item(name_, tgt_type,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           NSTL2_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0,
                                                                    "Failed to parse DNS message");
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           const auto size = ns_rr_rdlen(raw_record);
                                           NSTL2_THROW_EXCEPTION_IF(size <= 6, "Invalid SRV record");

                                           gen_srv record;
                                           record.priority = ns_get16(rdata);
                                           record.weight = ns_get16(rdata + 2);
                                           record.port = ns_get16(rdata + 4);
                                           std::array<char, NS_MAXDNAME> item_name;
                                           const int len = dn_expand(ns_msg_base(message), ns_msg_end(message),
                                                                     rdata + 6, item_name.data(), item_name.size());
                                           NSTL2_THROW_EXCEPTION_IF(len < 0, "invalid response");
                                           record.address.assign(item_name.data(),
                                                                 strnlen(item_name.data(), static_cast<size_t>(len)));

                                           if (!retval.has_value())
                                           {
                                               retval.emplace();
                                           }
                                           retval->emplace_back(std::move(record));
                                       }
                                   })
                   ? retval
                   : std::nullopt;
    }

    std::optional<std::vector<gen_svcb>> svcb_name(const char* name_, const SvcbType type_)
    {
        const int tgt_type = static_cast<std::underlying_type_t<SvcbType>>(type_);
        std::optional<std::vector<gen_svcb>> retval;

        return this->_process_item(name_, tgt_type,
                                   [&retval, tgt_type](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           NSTL2_THROW_EXCEPTION_IF(ns_parserr(&message, ns_s_an, idx, &raw_record) < 0,
                                                                    "Failed to parse DNS message");
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           const auto size = ns_rr_rdlen(raw_record);
                                           if (size < 2)
                                           {
                                               continue;
                                           }
                                           gen_svcb target;

                                           auto p = rdata;
                                           target.priority = ns_get16(p);
                                           p += 2;

                                           std::array<char, NS_MAXDNAME> buffer;
                                           const int buff_n = dn_expand(ns_msg_base(message), ns_msg_end(message), p,
                                                                        buffer.data(), buffer.size());
                                           NSTL2_THROW_EXCEPTION_IF(buff_n < 0, "dn_expand failed");
                                           p += buff_n;
                                           if (buffer[0] == '\0')
                                           {
                                               buffer[0] = '.';
                                               buffer[1] = '\0';
                                           }
                                           target.address.assign(buffer.data(), strnlen(buffer.data(), buffer.size()));
                                           target.params = resolve_params(p, rdata + size);

                                           if (!retval.has_value())
                                           {
                                               retval.emplace();
                                           }
                                           retval->push_back(std::move(target));
                                       }
                                   })
                   ? retval
                   : std::nullopt;
    }
};

} // namespace

std::optional<std::vector<mx_srv>> mx_name(const char* name_) { return DnsClient::instance().mx_name(name_); }

std::optional<std::vector<std::string>> txt_name(const char* name_) { return DnsClient::instance().txt_name(name_); }

std::optional<std::vector<std::string>> c_name(const char* name_) { return DnsClient::instance().c_name(name_); }

std::optional<std::vector<gen_srv>> srv_name(const char* name_) { return DnsClient::instance().srv_name(name_); }

std::optional<std::vector<gen_svcb>> svcb_name(const char* name_, const SvcbType type_)
{
    return DnsClient::instance().svcb_name(name_, type_);
}
} // namespace nstl::net
