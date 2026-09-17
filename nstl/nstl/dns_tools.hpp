#ifndef _NSTL_DNS_TOOLS
#define _NSTL_DNS_TOOLS 1

#include <nstl/ip_tools.hpp>

#include <array>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace nstl::net
{
namespace detail
{
constexpr int max_host_size = 256;
std::array<char, max_host_size> stack_name(std::string_view name_);
}

std::string hostname();
std::optional<std::string> canonical_name(std::string_view name_);

enum class IpClass
{
    Ipv4 = 1,
    Ipv6 = 2,
    IpvAll = 3,
};

std::optional<std::vector<ip_addr_gen>> ips_name(std::string_view name_, IpClass class_ = IpClass::IpvAll);

struct mx_srv
{
    std::string address;
    std::uint16_t priority{ 0 };
};

std::optional<std::vector<mx_srv>> mx_name(std::string_view name_);
std::optional<std::vector<std::string>> txt_name(std::string_view name_);
std::optional<std::vector<std::string>> c_name(std::string_view name_);

struct gen_srv
{
    std::string address;
    std::uint64_t port{ 0 };
    std::uint16_t priority{ 0 };
    std::uint16_t weight{ 0 };
};

std::optional<std::vector<gen_srv>> srv_name(std::string_view name_);

using svcb_param = std::variant<std::monostate,             // no default alpn
                                std::vector<std::uint16_t>, // mandatory keys
                                std::string,                // doh path
                                std::uint16_t,              // port
                                std::vector<std::string>,   // alpns
                                std::vector<ipv4_addr>,     // ipv4 addresses
                                std::vector<ipv6_addr>      // ipv6 addresses
                                >;

struct gen_svcb
{
    std::string address;
    std::uint16_t priority{ 0 };
    std::vector<svcb_param> params;
};

std::ostream& operator<<(std::ostream& os_, const gen_svcb& item);

enum class SvcbType : std::uint16_t
{
    Svcb = 64,
    Https = 65,
};

std::optional<std::vector<gen_svcb>> svcb_name(std::string_view name_, SvcbType type_ = SvcbType::Svcb);
} // namespace nstl::net

#endif
