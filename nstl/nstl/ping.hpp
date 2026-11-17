#ifndef _NSTL_PING_HPP
#define _NSTL_PING_HPP 1

#include <nstl/ip_tools.hpp>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

#include <chrono>
#include <string_view>

#include <cstdint>

namespace nstl::net
{
class Ping
{
    sockaddr_storage _addr;
    int _addr_len{ 0 };
#ifdef _WIN32
    void* _handle{ nullptr };
#else
    int _sock{ -1 };
#endif
    std::uint64_t _next{ 0 };

public:
    explicit Ping(std::string_view fqdn_);
    explicit Ping(const ip_addr_gen& ip_);
    ~Ping();
    Ping(const Ping&) = delete;
    Ping& operator=(const Ping&) = delete;

    // Sends one ICMP echo request and waits at most to_ for the matching reply.
    bool ping(std::chrono::microseconds to_);
};
} // namespace nstl::net

#endif
