#ifndef _NSTL_DNS_TOOLS
#define _NSTL_DNS_TOOLS 1

#include <array>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

namespace nstl::net
{
std::string hostname();
std::optional<std::string> canonical_name(const char* name_);
std::optional<std::string> canonical_name(const std::string& name_);

struct mx_srv
{
    std::string address;
    std::uint16_t priority{ 0 };
};

std::optional<std::vector<mx_srv>> mx_name(const char* name_);
std::optional<std::vector<mx_srv>> mx_name(const std::string& name_);
std::optional<std::vector<std::string>> txt_name(const char* name_);
std::optional<std::vector<std::string>> txt_name(const std::string& name_);
std::optional<std::vector<std::string>> c_name(const char* name_);
std::optional<std::vector<std::string>> c_name(const std::string& name_);

struct gen_srv
{
    std::string address;
    std::uint64_t port{ 0 };
    std::uint16_t priority{ 0 };
    std::uint16_t weight{ 0 };
};

std::optional<std::vector<gen_srv>> srv_name(const char* name_);
std::optional<std::vector<gen_srv>> srv_name(const std::string& name_);

using ipv4_addr = std::uint32_t;
using ipv6_addr = std::array<std::uint8_t, 16>;

using svcb_param = std::variant<
    std::monostate, // no default alpn
    std::vector<std::uint16_t>, // mandatory keys
    std::string, // doh path
    std::uint16_t, // port
    std::vector<std::string>, // alpns
    std::vector<ipv4_addr>, // ipv4 addresses
    std::vector<ipv6_addr> // ipv6 addresses
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

std::optional<std::vector<gen_svcb>> svcb_name(const char* name_, SvcbType type_ = SvcbType::Svcb);
std::optional<std::vector<gen_svcb>> svcb_name(const std::string& name_, SvcbType type_ = SvcbType::Svcb);

std::variant<std::monostate, ipv4_addr, ipv6_addr> parseIpAddress(const char* ipaddr_);

std::string writeIpAddress(const ipv4_addr& ip_);
std::string writeIpAddress(const ipv6_addr& ip_);
std::string writeIpAddress(std::span<const std::uint8_t> ip_);
std::string writeIpAddress(const std::variant<ipv4_addr, ipv6_addr>& addr_);
std::optional<ipv4_addr> is_ipv4(const ipv6_addr& addr_);
} // namespace nstl::net

#endif
