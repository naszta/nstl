#ifndef _NSTL_IP_TOOLS
#define _NSTL_IP_TOOLS 1

#include <nstl/exception.hpp>

#include <array>
#include <iosfwd>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace nstl::net
{
constexpr const size_t bytes_in_bits = 8;

template <size_t data_size>
void and_arrays(const std::array<std::uint8_t, data_size>& a_, const std::array<std::uint8_t, data_size>& b_,
                std::array<std::uint8_t, data_size>& dst_)
{
    for (size_t i = 0; i < data_size; ++i)
    {
        dst_[i] = a_[i] & b_[i];
    }
}

template <size_t data_size> auto create_mask(std::uint8_t mask_) -> std::array<std::uint8_t, data_size>
{
    NSTL2_THROW_EXCEPTION_IF(data_size * bytes_in_bits < mask_, mask_ << " mask is invalid");
    std::array<std::uint8_t, data_size> retval;
    std::memset(retval.data(), 0, retval.size());
    for (unsigned i = 0; i < data_size && 0 < mask_; ++i)
    {
        const std::uint8_t bits = bytes_in_bits <= mask_ ? bytes_in_bits : mask_;
        retval[i] = static_cast<std::uint8_t>(0xFF << (bytes_in_bits - bits));
        mask_ -= bits;
    }
    return retval;
}

template <std::size_t ip_size> struct ip_range_base
{
    static constexpr auto ip_max_bits = ip_size * bytes_in_bits;
    using ip_add_type = std::array<std::uint8_t, ip_size>;

    ip_range_base()
    {
        std::memset(ip.data(), 0, ip.size());
        std::memset(mask.data(), 0, mask.size());
    }
    explicit ip_range_base(const ip_add_type& ip_, std::uint8_t mask_ = ip_max_bits)
        : ip_range_base{ ip_, create_mask<ip_size>(mask_) }
    {
    }
    ip_range_base(const ip_add_type& ip_, const ip_add_type& mask_) : ip{ ip_ }, mask{ mask_ } {}

    ip_add_type ip;
    ip_add_type mask;

    bool contains(const ip_add_type& ip_) const
    {
        ip_add_type ip_result, local_result;
        and_arrays(mask, ip_, ip_result);
        and_arrays(mask, ip, local_result);
        return std::memcmp(ip_result.data(), local_result.data(), ip_result.size()) == 0;
    }
};

constexpr const size_t ipv4_size = 4;
constexpr const size_t ipv6_size = 16;

using ipv4_addr = std::array<std::uint8_t, ipv4_size>;
std::ostream& operator<<(std::ostream& os_, const ipv4_addr& ip_);

using ipv6_addr = std::array<std::uint8_t, ipv6_size>;
std::ostream& operator<<(std::ostream& os_, const ipv6_addr& ip_);

ipv4_addr create_ipv4_mask(const std::uint8_t mask_);
using ipv4_range = ip_range_base<ipv4_size>;
std::ostream& operator<<(std::ostream& os_, const ipv4_range& ip_);
bool operator==(const ipv4_range& l_, const ipv4_range& r_);
bool operator<(const ipv4_range& l_, const ipv4_range& r_);
std::optional<ipv4_range> parse_ip4_range(std::string_view range_);

ipv6_addr create_ipv6_mask(const std::uint8_t mask_);
using ipv6_range = ip_range_base<ipv6_size>;
std::ostream& operator<<(std::ostream& os_, const ipv6_range& ip_);
bool operator==(const ipv6_range& l_, const ipv6_range& r_);
bool operator<(const ipv6_range& l_, const ipv6_range& r_);
std::optional<ipv6_range> parse_ip6_range(std::string_view range_);

std::optional<ipv4_addr> parse_ip4_address(std::string_view ipaddr_);
std::optional<ipv6_addr> parse_ip6_address(std::string_view ipaddr_);
std::variant<std::monostate, ipv4_addr, ipv6_addr> parse_ip_address(std::string_view ipaddr_);

std::string to_string(const ipv4_addr& ip_);
std::string to_string(const ipv6_addr& ip_);
std::string to_string(std::span<const std::uint8_t> ip_);
std::string to_string(const std::variant<ipv4_addr, ipv6_addr>& addr_);
std::optional<ipv4_addr> is_ipv4(std::span<const std::uint8_t> addr_);
} // namespace nstl::net

#endif
