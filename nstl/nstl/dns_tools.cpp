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
std::string hostname()
{
    // https://man7.org/linux/man-pages/man2/gethostname.2.html - SUSv2 guarantees that "Host names are limited to 255
    // bytes". https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostname - So if a buffer of
    // 256 bytes is passed in the name parameter and the namelen parameter is set to 256, the buffer size will always be
    // adequate.
    constexpr int max_host_size = 256;
    std::array<char, max_host_size> buffer;
    std::memset(buffer.data(), 0, buffer.size());
    NSTL2_THROW_EXCEPTION_IF(::gethostname(buffer.data(), max_host_size) != 0, "hostname cannot be resolved");
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

std::optional<std::string> canonical_name(const char* name_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_CANONNAME;
    addrinfo* result_raw = nullptr;
    const auto success = ::getaddrinfo(name_, nullptr, &hints, &result_raw);
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

std::optional<std::string> canonical_name(const std::string& name_) { return canonical_name(name_.c_str()); }
std::optional<std::vector<mx_srv>> mx_name(const std::string& name_) { return mx_name(name_.c_str()); }
std::optional<std::vector<std::string>> txt_name(const std::string& name_) { return txt_name(name_.c_str()); }
std::optional<std::vector<std::string>> c_name(const std::string& name_) { return c_name(name_.c_str()); }
std::optional<std::vector<gen_srv>> srv_name(const std::string& name_) { return srv_name(name_.c_str()); }
std::optional<std::vector<gen_svcb>> svcb_name(const std::string& name_, const SvcbType type_)
{
    return svcb_name(name_.c_str(), type_);
}

namespace
{
void inet4_to_os(std::ostream& os_, std::uint32_t ip_)
{
    std::array<char, INET_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET, &ip_, buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "inet_ntop failed");
    const std::string_view ip_name{ ptr, strnlen(ptr, buffer.size()) };
    os_ << ip_name;
}

void inet6_to_os(std::ostream& os_, const std::array<std::uint8_t, 16>& ip_)
{
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET6, ip_.data(), buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "inet_ntop failed");
    const std::string_view ip_name{ ptr, strnlen(ptr, buffer.size()) };
    os_ << ip_name;
}

struct param_visitor
{
    std::ostream& oss;
    explicit param_visitor(std::ostream& os_) : oss{ os_ } {}

    void operator()(const std::monostate&) const { oss << "NO_DEF_ALPN"; }
    void operator()(const std::vector<std::uint16_t>& keys_) const { oss << "KEYS={" << range_print(keys_, ',') << '}'; }
    void operator()(const std::string& doh_) const { oss << "DOH=\"" << doh_ << '\"'; }
    void operator()(const std::uint16_t port_) const { oss << "PORT=" << port_; }
    void operator()(const std::vector<std::string>& alpns_) const {
        oss << "ALPNS={";
        if (!alpns_.empty())
        {
            oss << '\"' << range_print(alpns_, "\",\"") << '\"';
        }
        oss << '}';
    }
    void operator()(const std::vector<std::uint32_t>& ipv4s_) const
    {
        oss << "IPV4S={";
        if (auto itr = ipv4s_.cbegin(); itr != ipv4s_.cend())
        {
            inet4_to_os(oss, *itr);
            while (++itr != ipv4s_.cend())
            {
                oss << ",";
                inet4_to_os(oss, *itr);
            }
        }
        oss << '}';
    }
    void operator()(const std::vector<std::array<std::uint8_t, 16>>& ipv4s_) const
    {
        oss << "IPV6S={";
        if (auto itr = ipv4s_.cbegin(); itr != ipv4s_.cend())
        {
            inet6_to_os(oss, *itr);
            while (++itr != ipv4s_.cend())
            {
                oss << ",";
                inet6_to_os(oss, *itr);
            }
        }
        oss << '}';
    }
};
}

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

std::variant<std::monostate, ipv4_addr, ipv6_addr> parseIpAddress(const char* ipaddr_)
{
    NSTL2_THROW_EXCEPTION_IF(!ipaddr_, "input is nullptr");
    ipv4_addr ipv4 = 0;
    if (::inet_pton(AF_INET, ipaddr_, &ipv4) == 1)
    {
        return ipv4;
    }

    ipv6_addr ipv6;
    if (::inet_pton(AF_INET6, ipaddr_, ipv6.data()) == 1)
    {
        return ipv6;
    }
    return std::monostate{};
}

namespace
{
struct ip_visitor
{
    std::string operator()(const ipv4_addr& ip_) const { return writeIpAddress(ip_); }
    std::string operator()(const ipv6_addr& ip_) const { return writeIpAddress(ip_); }
};
}

std::string writeIpAddress(const ipv4_addr& ip_)
{
    std::array<char, INET_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET, &ip_, buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::string writeIpAddress(const ipv6_addr& ip_)
{
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET6, &ip_, buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::string writeIpAddress(const std::variant<ipv4_addr, ipv6_addr>& addr_) { return std::visit(ip_visitor{}, addr_); }

std::string writeIpAddress(const std::span<const std::uint8_t> ip_)
{
    NSTL2_THROW_EXCEPTION_IF(ip_.size() != 4 && ip_.size() != 16, "Blob size is invalid (4 or 16 expected)");
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const int family = ip_.size() == 4 ? AF_INET : AF_INET6;
    const char* ptr = ::inet_ntop(family, ip_.data(), buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::optional<ipv4_addr> is_ipv4(const ipv6_addr& addr_)
{
    constexpr std::array<std::uint8_t, 12> prefix{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
    if (std::memcmp(prefix.data(), addr_.data(), prefix.size()) == 0)
    {
        ipv4_addr retval = 0;
        std::memcpy(&retval, addr_.data() + 12, 4);
        return retval;
    }
    return std::nullopt;
}
} // namespace nstl::net
