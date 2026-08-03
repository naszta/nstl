#ifndef _NSTL_DNS_TOOLS
#define _NSTL_DNS_TOOLS 1

#include <optional>
#include <string>
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
} // namespace nstl::net

#endif
