#include "ip_tools.hpp"
#include "exception.hpp"

#include <charconv>
#include <ostream>
#include <tuple>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include <cstring>

namespace nstl::net
{
ipv4_addr create_ipv4_mask(const std::uint8_t mask_) { return create_mask<ipv4_size>(mask_); }

ipv6_addr create_ipv6_mask(const std::uint8_t mask_) { return create_mask<ipv6_size>(mask_); }

template <size_t ip_size, size_t str_size, int ip_class>
std::optional<std::array<std::uint8_t, ip_size>> parse_ip_address(const std::string_view ipaddr_)
{
    if (str_size <= ipaddr_.size())
    {
        return std::nullopt;
    }
    std::array<char, str_size> buffer;
    std::strncpy(buffer.data(), ipaddr_.data(), ipaddr_.size());
    buffer[ipaddr_.size()] = '\0';
    std::optional<std::array<std::uint8_t, ip_size>> ipval;
    ipval.emplace();
    if (::inet_pton(ip_class, buffer.data(), ipval->data()) == 1)
    {
        return ipval;
    }
    ipval.reset();
    return ipval;
}

template <size_t ip_size, size_t str_size, int ip_class>
std::optional<ip_range_base<ip_size>> parse_ip_range(const std::string_view range_)
{
    const auto pos = range_.find('/');
    const auto ip_part = range_.substr(0, pos);
    const auto ip_opt = parse_ip_address<ip_size, str_size, ip_class>(ip_part);
    if (!ip_opt.has_value())
    {
        return std::nullopt;
    }
    std::optional<ip_range_base<ip_size>> retval;
    if (pos == std::string_view::npos)
    {
        retval.emplace(ip_opt.value());
        return retval;
    }

    const auto num_view = range_.substr(pos + 1);
    if (num_view.empty())
    {
        return retval;
    }
    if (const auto ip_mask = parse_ip_address<ip_size, str_size, ip_class>(num_view))
    {
        retval.emplace(ip_opt.value(), ip_mask.value());
    }
    else
    {
        const auto endptr = num_view.data() + num_view.size();
        std::uint8_t mask = 0;

        const auto val = std::from_chars(num_view.data(), endptr, mask);
        if (val.ptr == endptr && val.ec == std::errc{} && mask <= ip_range_base<ip_size>::ip_max_bits)
        {
            retval.emplace(ip_opt.value(), mask);
        }
    }
    return retval;
}

std::optional<ipv4_range> parse_ip4_range(const std::string_view range_)
{
    return parse_ip_range<ipv4_size, INET_ADDRSTRLEN, AF_INET>(range_);
}

std::optional<ipv6_range> parse_ip6_range(const std::string_view range_)
{
    return parse_ip_range<ipv6_size, INET6_ADDRSTRLEN, AF_INET6>(range_);
}

std::ostream& operator<<(std::ostream& os_, const ipv4_addr& ip_)
{
    std::array<char, INET_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET, ip_.data(), buffer.data(), buffer.size());
    if (!ptr) [[unlikely]]
    {
        os_.setstate(std::ios_base::failbit);
        return os_;
    }
    const std::string_view ip_name{ ptr, strnlen(ptr, buffer.size()) };
    os_ << ip_name;
    return os_;
}

std::ostream& operator<<(std::ostream& os_, const ipv6_addr& ip_)
{
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET6, ip_.data(), buffer.data(), buffer.size());
    if (!ptr) [[unlikely]]
    {
        os_.setstate(std::ios_base::failbit);
        return os_;
    }
    const std::string_view ip_name{ ptr, strnlen(ptr, buffer.size()) };
    os_ << ip_name;
    return os_;
}

std::ostream& operator<<(std::ostream& os_, const ipv4_range& ip_) { return os_ << ip_.ip << '/' << ip_.mask; }

std::ostream& operator<<(std::ostream& os_, const ipv6_range& ip_) { return os_ << ip_.ip << '/' << ip_.mask; }

std::optional<ipv4_addr> parse_ip4_address(const std::string_view ipaddr_)
{
    return parse_ip_address<ipv4_size, INET_ADDRSTRLEN, AF_INET>(ipaddr_);
}

std::optional<ipv6_addr> parse_ip6_address(const std::string_view ipaddr_)
{
    return parse_ip_address<ipv6_size, INET6_ADDRSTRLEN, AF_INET6>(ipaddr_);
}

std::variant<std::monostate, ipv4_addr, ipv6_addr> parse_ip_address(const std::string_view ipaddr_)
{
    if (auto ipv4_opt = parse_ip4_address(ipaddr_))
    {
        return ipv4_opt.value();
    }
    if (auto ipv6_opt = parse_ip6_address(ipaddr_))
    {
        return ipv6_opt.value();
    }
    return std::monostate{};
}

namespace
{
struct ip_visitor
{
    std::string operator()(const ipv4_addr& ip_) const { return to_string(ip_); }
    std::string operator()(const ipv6_addr& ip_) const { return to_string(ip_); }
};
} // namespace

std::string to_string(const ipv4_addr& ip_)
{
    std::array<char, INET_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET, ip_.data(), buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::string to_string(const ipv6_addr& ip_)
{
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const char* ptr = ::inet_ntop(AF_INET6, ip_.data(), buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::string to_string(const std::variant<ipv4_addr, ipv6_addr>& addr_) { return std::visit(ip_visitor{}, addr_); }

std::string to_string(const std::span<const std::uint8_t> ip_)
{
    NSTL2_THROW_EXCEPTION_IF(ip_.size() != ipv4_size && ip_.size() != ipv6_size,
                             "Blob size is invalid (4 or 16 expected)");
    std::array<char, INET6_ADDRSTRLEN> buffer;
    const int family = ip_.size() == ipv4_size ? AF_INET : AF_INET6;
    const char* ptr = ::inet_ntop(family, ip_.data(), buffer.data(), buffer.size());
    NSTL2_THROW_EXCEPTION_IF(!ptr, "IP cannot converted to string");
    return std::string{ ptr, strnlen(ptr, buffer.size()) };
}

std::optional<ipv4_addr> is_ipv4(const std::span<const std::uint8_t> addr_)
{
    constexpr std::array<std::uint8_t, 12> prefix{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
    if (addr_.size() == ipv4_size)
    {
        ipv4_addr retval;
        std::memcpy(retval.data(), addr_.data(), retval.size());
        return retval;
    }
    else if (addr_.size() == ipv6_size)
    {
        if (std::memcmp(prefix.data(), addr_.data(), prefix.size()) == 0)
        {
            ipv4_addr retval;
            std::memcpy(retval.data(), addr_.data() + prefix.size(), retval.size());
            return retval;
        }
        return std::nullopt;
    }
    NSTL2_THROW_EXCEPTION("Address is not 4 or 16 bytes long: it is not either IPv4 or IPv6");
}

bool operator==(const ipv4_range& l_, const ipv4_range& r_)
{
    return std::tie(l_.ip, l_.mask) == std::tie(r_.ip, r_.mask);
}

bool operator<(const ipv4_range& l_, const ipv4_range& r_)
{
    return std::tie(l_.ip, l_.mask) < std::tie(r_.ip, r_.mask);
}

bool operator==(const ipv6_range& l_, const ipv6_range& r_)
{
    return std::tie(l_.ip, l_.mask) == std::tie(r_.ip, r_.mask);
}

bool operator<(const ipv6_range& l_, const ipv6_range& r_)
{
    return std::tie(l_.ip, l_.mask) < std::tie(r_.ip, r_.mask);
}

template <size_t ip_size>
class address_checker
{
    const std::array<std::uint8_t, ip_size>& addr;

public:
    explicit address_checker(const std::array<std::uint8_t, ip_size>& addr_) : addr{ addr_ } {}

    template <size_t range_size> bool operator()(const ip_range_base<range_size>& range_) const
    {
        if constexpr (ip_size == range_size)
        {
            return range_.contains(addr);
        }
        else
        {
            return false;
        }
    }
};

template <size_t ip_size>
bool contains_t(const std::span<const ip_range_gen> ranges_, const std::array<std::uint8_t, ip_size>& address_)
{
    const address_checker checker{ address_ };

    return std::ranges::find_if(ranges_, [&checker](const ip_range_gen& range_)
                                { return std::visit(checker, range_); }) !=
           ranges_.end();
}


bool contains(const std::span<const ip_range_gen> ranges_, const ipv4_addr& address_)
{
    return contains_t(ranges_, address_);
}

bool contains(const std::span<const ip_range_gen> ranges_, const ipv6_addr& address_)
{
    return contains_t(ranges_, address_);
}

class ip_contain_visitor
{
    const std::span<const ip_range_gen>& ranges;

public:
    explicit ip_contain_visitor(const std::span<const ip_range_gen>& ranges_) : ranges{ ranges_ } {}

    bool operator()(const ipv4_addr& address_) const { return contains(ranges, address_); }
    bool operator()(const ipv6_addr& address_) const { return contains(ranges, address_); }
};

bool contains(const std::span<const ip_range_gen> ranges_, const ip_addr_gen& address_)
{
    return std::visit(ip_contain_visitor{ranges_}, address_);
}
} // namespace nstl::net
