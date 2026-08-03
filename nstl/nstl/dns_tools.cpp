#include "dns_tools.hpp"
#include "exception.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <cstring>
#include <array>
#include <memory>
#include <stdexcept>

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
struct AddrinfoDeleter
{
    void operator()(addrinfo* ptr) const
    {
        if (ptr)
        {
            ::freeaddrinfo(ptr);
        }
    }
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
    NSTL2_THROW_EXCEPTION_IF(::getaddrinfo(name_, nullptr, &hints, &result_raw) != 0, name_ << " cannot be resolved");
    std::unique_ptr<addrinfo, AddrinfoDeleter> result{ result_raw };

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
} // namespace nstl::net
