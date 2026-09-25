#include "dns_tools.hpp"
#include "exception.hpp"
#include "range_print.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <cstring>
#include <array>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace nstl::net
{
std::array<char, detail::max_host_size> detail::stack_name(const std::string_view name_)
{
    NSTL2_THROW_EXCEPTION_IF(name_.empty(), "name_ cannot be nullptr");
    NSTL2_THROW_EXCEPTION_IF(detail::max_host_size <= name_.size(), "Target stack is small (it should be fine: FQDN is 255 bytes maximum)");
    std::array<char, max_host_size> stack_name;
    std::strncpy(stack_name.data(), name_.data(), name_.size());
    stack_name[name_.size()] = '\0';
    return stack_name;
}

std::string hostname()
{
    // https://man7.org/linux/man-pages/man2/gethostname.2.html - SUSv2 guarantees that "Host names are limited to 255
    // bytes". https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostname - So if a buffer of
    // 256 bytes is passed in the name parameter and the namelen parameter is set to 256, the buffer size will always be
    // adequate.
    std::array<char, detail::max_host_size> buffer;
    std::memset(buffer.data(), 0, buffer.size());
    NSTL2_THROW_EXCEPTION_IF(::gethostname(buffer.data(), detail::max_host_size) != 0, "hostname cannot be resolved");
    return std::string{ buffer.data() };
}

namespace
{
#ifdef _WIN32
constexpr int host_not_found = WSAHOST_NOT_FOUND;
#else
constexpr int host_not_found = EAI_NONAME;
#endif

struct AddrinfoDeleter
{
    void operator()(addrinfo* ptr) const { ::freeaddrinfo(ptr); }
};
} // namespace

std::optional<std::string> canonical_name(const std::string_view name_)
{
    const auto name = detail::stack_name(name_);
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_CANONNAME;
    addrinfo* result_raw = nullptr;
    const auto success = ::getaddrinfo(name.data(), nullptr, &hints, &result_raw);
    std::unique_ptr<addrinfo, AddrinfoDeleter> result{ std::exchange(result_raw, nullptr) };
    if (success != 0)
    {
        NSTL2_THROW_EXCEPTION_IF(success != host_not_found, "Issues on resolving " << name_);
        return std::nullopt;
    }

    std::optional<std::string> retval;

    for (auto ptr = result.get(); ptr != nullptr; ptr = ptr->ai_next)
    {
        if (ptr->ai_canonname)
        {
            retval.emplace(ptr->ai_canonname);
            return retval;
        }
    }
    return retval;
}

std::optional<std::vector<ip_addr_gen>> ips_name(const std::string_view name_, const IpClass class_)
{
    const auto name = detail::stack_name(name_);
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    switch (class_)
    {
    case IpClass::Ipv4:
        hints.ai_family = AF_INET;
        break;
    case IpClass::Ipv6:
        hints.ai_family = AF_INET6;
        break;
    case IpClass::IpvAll:
        hints.ai_family = AF_UNSPEC;
        break;
    default:
        NSTL2_THROW_EXCEPTION("Unknown IpClass type");
    }
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result_raw = nullptr;
    const auto success = ::getaddrinfo(name.data(), nullptr, &hints, &result_raw);
    std::unique_ptr<addrinfo, AddrinfoDeleter> result{ std::exchange(result_raw, nullptr) };
    if (success != 0)
    {
        NSTL2_THROW_EXCEPTION_IF(success != host_not_found, "Issues on resolving " << name_);
        return std::nullopt;
    }

    std::optional<std::vector<ip_addr_gen>> retval;
    for (auto ptr = result.get(); ptr != nullptr; ptr = ptr->ai_next)
    {
        if (ptr->ai_family == AF_INET)
        {
            const auto address = reinterpret_cast<sockaddr_in*>(ptr->ai_addr)->sin_addr;
            ipv4_addr target;
            std::memcpy(target.data(), &address, target.size());
            if (!retval.has_value())
            {
                retval.emplace();
            }
            retval->emplace_back(std::move(target));
        }
        else if (ptr->ai_family == AF_INET6)
        {
            const auto address = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr)->sin6_addr;
            ipv6_addr target;
            std::memcpy(target.data(), &address, target.size());
            if (!retval.has_value())
            {
                retval.emplace();
            }
            retval->emplace_back(std::move(target));
        }
    }

    return retval;
}

namespace
{
struct param_visitor
{
    std::ostream& oss;
    explicit param_visitor(std::ostream& os_) : oss{ os_ } {}

    void operator()(const std::monostate&) const { oss << "NO_DEF_ALPN"; }
    void operator()(const std::vector<std::uint16_t>& keys_) const
    {
        oss << "KEYS={" << range_print(keys_, ',') << '}';
    }
    void operator()(const std::string& doh_) const { oss << "DOH=\"" << doh_ << '\"'; }
    void operator()(const std::uint16_t port_) const { oss << "PORT=" << port_; }
    void operator()(const std::vector<std::string>& alpns_) const
    {
        oss << "ALPNS={";
        if (!alpns_.empty())
        {
            oss << '\"' << range_print(alpns_, "\",\"") << '\"';
        }
        oss << '}';
    }
    void operator()(const std::vector<ipv4_addr>& ipv4s_) const
    {
        oss << "IPV4S={";
        if (auto itr = ipv4s_.cbegin(); itr != ipv4s_.cend())
        {
            oss << *itr;
            while (++itr != ipv4s_.cend())
            {
                oss << ",";
                oss << *itr;
            }
        }
        oss << '}';
    }
    void operator()(const std::vector<ipv6_addr>& ipv6s_) const
    {
        oss << "IPV6S={";
        if (auto itr = ipv6s_.cbegin(); itr != ipv6s_.cend())
        {
            oss << *itr;
            while (++itr != ipv6s_.cend())
            {
                oss << ",";
                oss << *itr;
            }
        }
        oss << '}';
    }
};
} // namespace

std::ostream& operator<<(std::ostream& os_, const gen_svcb& item)
{
    os_ << item.address << ' ' << item.priority << " [";
    if (auto itr = item.params.cbegin(); itr != item.params.cend())
    {
        const param_visitor visitor{ os_ };
        std::visit(visitor, *itr);
        while (++itr != item.params.cend())
        {
            os_ << ' ';
            std::visit(visitor, *itr);
        }
    }
    os_ << ']';
    return os_;
}
} // namespace nstl::net
