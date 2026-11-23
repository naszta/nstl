#include "dns_tools.hpp"
#include "exception.hpp"
#include "memory.hpp"
#include "scope_exit.hpp"

#include <stdexcept>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <ifaddrs.h>
#include <net/if.h>

#include <array>
#include <functional>
#include <cstring>
#include <utility>
#include <vector>

#include <string.h>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define NSTL_RES_HAS_SETSERVERS 1
#endif

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
            bool valid = true;
            while (q < vend)
            {
                const uint8_t l = *q++;
                if (vend < q + l) [[unlikely]]
                {
                    valid = false;
                    break;
                }
                std::string value{ reinterpret_cast<const char*>(q), l };
                alpns.push_back(std::move(value));
                q += l;
            }
            if (valid)
            {
                retval.emplace_back(std::move(alpns));
            }
            break;
        }
        case KEY_PORT:
            if (len == 2)
            {
                std::uint16_t port = ns_get16(v);
                retval.emplace_back(port);
            }
            break;
        case KEY_MANDATORY:
        {
            if (len % 2 == 0)
            {
                std::vector<std::uint16_t> keys;
                for (std::uint16_t idx = 0; idx < len; idx += 2)
                {
                    keys.push_back(ns_get16(v + idx));
                }
                retval.emplace_back(std::move(keys));
            }
            break;
        }
        case KEY_IPV4HINT:
        {
            if (len % ipv4_size == 0)
            {
                std::vector<ipv4_addr> ipv4s;

                for (std::uint16_t idx = 0; idx < len; idx += ipv4_size)
                {
                    auto& tgt = ipv4s.emplace_back();
                    std::memcpy(tgt.data(), v + idx, tgt.size());
                }
                retval.emplace_back(std::move(ipv4s));
            }
            break;
        }
        case KEY_IPV6HINT:
        {
            if (len % ipv6_size == 0)
            {
                std::vector<ipv6_addr> ipv6s;
                for (std::uint16_t idx = 0; idx < len; idx += ipv6_size)
                {
                    auto& tgt = ipv6s.emplace_back();
                    std::memcpy(tgt.data(), v + idx, tgt.size());
                }
                retval.emplace_back(std::move(ipv6s));
            }
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

constexpr std::uint16_t dns_port = 53;

#ifdef NSTL_RES_HAS_SETSERVERS

struct sockaddr_union_visitor
{
    union res_sockaddr_union& _tgt;
    std::uint16_t _port{ dns_port };

    void operator()(const ipv4_addr& ip_) const
    {
        auto& sin = _tgt.sin;
        sin.sin_len = sizeof(sin);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(_port);
        std::memcpy(&sin.sin_addr, ip_.data(), ip_.size());
    }

    void operator()(const ipv6_addr& ip_) const
    {
        auto& sin6 = _tgt.sin6;
        sin6.sin6_len = sizeof(sin6);
        sin6.sin6_family = AF_INET6;
        sin6.sin6_port = htons(_port);
        std::memcpy(&sin6.sin6_addr, ip_.data(), ip_.size());
    }
};

#else

struct state_dns_visitor
{
    struct __res_state& _state;
    std::uint16_t _port;
    size_t _idx;

    state_dns_visitor(struct __res_state& state_, size_t idx_, std::uint16_t port_ = dns_port)
        : _state{ state_ }, _port{ port_ }, _idx{ idx_ }
    {
    }

    void operator()(const ipv4_addr& ip_) const
    {
        auto& tgt4 = _state.nsaddr_list[_idx];
        tgt4.sin_family = AF_INET;
        tgt4.sin_port = htons(_port);
        std::memcpy(&tgt4.sin_addr, ip_.data(), ip_.size());
    }

    void operator()(const ipv6_addr& ip_) const
    {
        std::unique_ptr<struct sockaddr_in6, free_deleter> sa6{ reinterpret_cast<struct sockaddr_in6*>(
            std::malloc(sizeof(sockaddr_in6))) };
        std::memset(sa6.get(), 0, sizeof(sockaddr_in6));
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(_port);
        std::memcpy(&sa6->sin6_addr, ip_.data(), ip_.size());

        auto& tgt4 = _state.nsaddr_list[_idx];
        tgt4.sin_family = 0;

        auto raw_ptr = std::exchange(_state._u._ext.nsaddrs[_idx], sa6.release());
        sa6.reset(raw_ptr);
    }
};
#endif

class state_manager
{
    struct __res_state _state;

public:
    state_manager()
    {
        std::memset(&_state, 0, sizeof(struct __res_state));
        NSTL2_THROW_EXCEPTION_IF(res_ninit(&_state) < 0, "Resolver initialize failed");
    }
    ~state_manager()
    {
#ifdef NSTL_RES_HAS_SETSERVERS
        res_ndestroy(&_state);
#else
        res_nclose(&_state);
#endif
    }
    state_manager(const state_manager&) = delete;
    state_manager& operator=(const state_manager&) = delete;

    void set_servers(const std::vector<ip_addr_gen>& servers_)
    {
        const auto num_of_dns = std::min<size_t>(MAXNS, servers_.size());
#ifdef NSTL_RES_HAS_SETSERVERS
        std::array<union res_sockaddr_union, MAXNS> addrs{};
        for (size_t idx = 0; idx < num_of_dns; ++idx)
        {
            std::visit(sockaddr_union_visitor{ addrs[idx], dns_port }, servers_[idx]);
        }
        res_setservers(&_state, addrs.data(), static_cast<int>(num_of_dns)); // sets nscount itself
#else
        _state.nscount = num_of_dns;

        for (size_t idx = 0; idx < num_of_dns; ++idx)
        {
            std::visit(state_dns_visitor{ _state, idx }, servers_[idx]);
        }
#endif
    }

    observer_ptr<const struct __res_state> get() const { return make_observer(&_state); }
    observer_ptr<struct __res_state> get() { return make_observer(&_state); }
};

std::pair<observer_ptr<struct __res_state>, std::unique_ptr<state_manager>>
resolve_state_ctx(state_manager& state_, observer_ptr<const dns_config> cfg_)
{
    if (cfg_ && !cfg_->dns_servers.empty())
    {
        auto ret_state = std::make_unique<state_manager>();
        ret_state->set_servers(cfg_->dns_servers);
        return std::make_pair(ret_state->get(), std::move(ret_state));
    }
    return std::make_pair(state_.get(), nullptr);
}

class DnsClient
{
    static constexpr size_t buff_size = 4096;
    std::vector<unsigned char> _response;
    state_manager _state;

    bool _process_item(const char* name_, const int ns_type_, observer_ptr<const dns_config> cfg_,
                       const std::function<void(ns_msg& message, int count)>& func_)
    {
        const auto [ptr, ctx] = resolve_state_ctx(_state, cfg_);

        int length = -1;
        while (length < 0)
        {
            length = res_nquery(ptr.get(), name_, ns_c_in, ns_type_, _response.data(), _response.size());
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
        if (ns_initparse(_response.data(), length, &message) < 0) [[unlikely]]
        {
            return false;
        }

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

    DnsClient() { _response.resize(buff_size); }

    template <size_t ip_size, int tgt_type>
    std::optional<std::vector<ip_addr_base<ip_size>>> ip_ip_name_t(const char* name_,
                                                                   observer_ptr<const dns_config> cfg_)
    {
        std::optional<std::vector<ip_addr_base<ip_size>>> retval;

        return this->_process_item(name_, tgt_type, cfg_,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                                           {
                                               continue;
                                           }
                                           // message is A/AAAA
                                           if (ns_rr_class(raw_record) == ns_c_in &&
                                               ns_rr_type(raw_record) == tgt_type && ns_rr_rdlen(raw_record) == ip_size)
                                           {
                                               if (!retval.has_value())
                                               {
                                                   retval.emplace();
                                               }
                                               ip_addr_base<ip_size>& target = retval->emplace_back();
                                               std::memcpy(target.data(), ns_rr_rdata(raw_record), ip_size);
                                           }
                                       }
                                   })
                   ? retval
                   : std::nullopt;
    }

    std::optional<std::vector<mx_srv>> mx_name(const char* name_, observer_ptr<const dns_config> cfg_)
    {
        constexpr int tgt_type = ns_t_mx;
        std::optional<std::vector<mx_srv>> retval;

        return this->_process_item(
                   name_, tgt_type, cfg_,
                   [&retval](ns_msg& message, const int count)
                   {
                       for (int idx = 0; idx < count; ++idx)
                       {
                           ns_rr raw_record;
                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                           {
                               continue;
                           }
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
                           if (expanded_length < 0)
                           {
                               continue;
                           }

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

    std::optional<std::vector<std::string>> txt_name(const char* name_, observer_ptr<const dns_config> cfg_)
    {
        constexpr int tgt_type = ns_t_txt;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type, cfg_,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                                           {
                                               continue;
                                           }
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           const size_t rdata_length = ns_rr_rdlen(raw_record);

                                           const auto end = rdata + rdata_length;

                                           std::string item;
                                           bool valid = true;

                                           for (auto p = rdata, next_p = p; p < end; p = next_p)
                                           {
                                               const std::uint8_t text_len = *p;
                                               ++p;
                                               next_p = p + text_len;
                                               if (end < next_p)
                                               {
                                                   valid = false;
                                                   break;
                                               }
                                               item.append(reinterpret_cast<const char*>(p), text_len);
                                           }
                                           if (!valid)
                                           {
                                               continue;
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

    std::optional<std::vector<std::string>> c_name(const char* name_, observer_ptr<const dns_config> cfg_)
    {
        constexpr int tgt_type = ns_t_cname;
        std::optional<std::vector<std::string>> retval;

        return this->_process_item(name_, tgt_type, cfg_,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                                           {
                                               continue;
                                           }
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           std::array<char, NS_MAXDNAME> item_name;

                                           const int len = dn_expand(ns_msg_base(message), ns_msg_end(message), rdata,
                                                                     item_name.data(), item_name.size());
                                           if (len < 0)
                                           {
                                               continue;
                                           }
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

    std::optional<std::vector<gen_srv>> srv_name(const char* name_, observer_ptr<const dns_config> cfg_)
    {
        constexpr int tgt_type = ns_t_srv;
        std::optional<std::vector<gen_srv>> retval;

        return this->_process_item(name_, tgt_type, cfg_,
                                   [&retval](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                                           {
                                               continue;
                                           }
                                           // message is not TXT
                                           if (ns_rr_class(raw_record) != ns_c_in || ns_rr_type(raw_record) != tgt_type)
                                           {
                                               continue;
                                           }
                                           const auto rdata = ns_rr_rdata(raw_record);
                                           const auto size = ns_rr_rdlen(raw_record);
                                           if (size <= 6)
                                           {
                                               continue;
                                           }

                                           gen_srv record;
                                           record.priority = ns_get16(rdata);
                                           record.weight = ns_get16(rdata + 2);
                                           record.port = ns_get16(rdata + 4);
                                           std::array<char, NS_MAXDNAME> item_name;
                                           const int len = dn_expand(ns_msg_base(message), ns_msg_end(message),
                                                                     rdata + 6, item_name.data(), item_name.size());
                                           if (len < 0)
                                           {
                                               continue;
                                           }
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

    std::optional<std::vector<gen_svcb>> svcb_name(const char* name_, const SvcbType type_,
                                                   observer_ptr<const dns_config> cfg_)
    {
        const int tgt_type = static_cast<std::underlying_type_t<SvcbType>>(type_);
        std::optional<std::vector<gen_svcb>> retval;

        return this->_process_item(name_, tgt_type, cfg_,
                                   [&retval, tgt_type](ns_msg& message, const int count)
                                   {
                                       for (int idx = 0; idx < count; ++idx)
                                       {
                                           ns_rr raw_record;
                                           if (ns_parserr(&message, ns_s_an, idx, &raw_record) < 0)
                                           {
                                               continue;
                                           }
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
                                           if (buff_n < 0)
                                           {
                                               continue;
                                           }
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

std::optional<std::vector<ipv4_addr>> ip_a_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().ip_ip_name_t<ipv4_size, ns_t_a>(name.data(), cfg_);
}

std::optional<std::vector<ipv6_addr>> ip_aaaa_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().ip_ip_name_t<ipv6_size, ns_t_aaaa>(name.data(), cfg_);
}

std::optional<std::vector<mx_srv>> mx_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().mx_name(name.data(), cfg_);
}

std::optional<std::vector<std::string>> txt_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().txt_name(name.data(), cfg_);
}

std::optional<std::vector<std::string>> c_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().c_name(name.data(), cfg_);
}

std::optional<std::vector<gen_srv>> srv_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().srv_name(name.data(), cfg_);
}

std::optional<std::vector<gen_svcb>> svcb_name(const std::string_view name_, const SvcbType type_,
                                               observer_ptr<const dns_config> cfg_)
{
    const auto name = detail::stack_name(name_);
    return DnsClient::instance().svcb_name(name.data(), type_, cfg_);
}

namespace
{
enum class Ipv6Classes
{
    Loopback,
    LinkLocal,
    ULA,
    V4Mapped,
    Teredo,
    Global,
    Other,
};

Ipv6Classes classify(const in6_addr& a)
{
    if (IN6_IS_ADDR_LOOPBACK(&a))
    {
        return Ipv6Classes::Loopback;
    }
    if (IN6_IS_ADDR_LINKLOCAL(&a))
    {
        return Ipv6Classes::LinkLocal;
    }
    if (IN6_IS_ADDR_V4MAPPED(&a))
    {
        return Ipv6Classes::V4Mapped;
    }
    if ((a.s6_addr[0] & 0xfe) == 0xfc)
    {
        return Ipv6Classes::ULA;
    }
    if (a.s6_addr[0] == 0x20 && a.s6_addr[1] == 0x01 && a.s6_addr[2] == 0 && a.s6_addr[3] == 0)
    {
        return Ipv6Classes::Teredo;
    }
    if ((a.s6_addr[0] & 0xe0) == 0x20)
    {
        return Ipv6Classes::Global;
    }
    return Ipv6Classes::Other;
}

} // namespace

bool box_have_valid_ipv6()
{
    ifaddrs* list = nullptr;
    NSTL2_THROW_EXCEPTION_IF(::getifaddrs(&list) != 0, "Network cannot be resolved");
    const auto list_cleanup = on_scope_exit([list]() { ::freeifaddrs(list); });

    for (auto it = list; it; it = it->ifa_next)
    {
        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET6)
        {
            continue;
        }
        if (!(it->ifa_flags & IFF_UP))
        {
            continue;
        }

        const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(it->ifa_addr);
        const auto kind = classify(sin6->sin6_addr);
        if (kind == Ipv6Classes::Global)
        {
            return true;
        }
    }

    return false;
}

} // namespace nstl::net
