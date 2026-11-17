#include "ping.hpp"

#include <nstl/exception.hpp>

#include <array>
#include <cstring>
#include <tuple>
#include <utility>
#include <variant>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>

namespace
{
constexpr int store_size = sizeof(struct sockaddr_storage);
constexpr std::uint64_t payload_magic = 0x6e73746c70696e67ULL; // "nstlping"

struct HandleVisitor
{
    sockaddr_storage& _addr;
    explicit HandleVisitor(sockaddr_storage& addr_) : _addr{ addr_ } {}

    std::pair<HANDLE, int> operator()(const nstl::net::ipv4_addr& ip_) const
    {
        constexpr int address_size = sizeof(struct sockaddr_in);
        static_assert(address_size <= store_size);

        auto ptr = reinterpret_cast<sockaddr_in*>(&_addr);
        ptr->sin_family = AF_INET;
        std::memcpy(&ptr->sin_addr, ip_.data(), ip_.size());
        return std::make_pair(::IcmpCreateFile(), address_size);
    }

    std::pair<HANDLE, int> operator()(const nstl::net::ipv6_addr& ip_) const
    {
        constexpr int address_size = sizeof(struct sockaddr_in6);
        static_assert(address_size <= store_size);

        auto ptr = reinterpret_cast<sockaddr_in6*>(&_addr);
        ptr->sin6_family = AF_INET6;
        std::memcpy(&ptr->sin6_addr, ip_.data(), ip_.size());
        return std::make_pair(::Icmp6CreateFile(), address_size);
    }
};
} // namespace

namespace nstl::net
{
Ping::Ping(const ip_addr_gen& ip_)
{
    std::memset(&_addr, 0, store_size);
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::tie(handle, _addr_len) = std::visit(HandleVisitor{ _addr }, ip_);
    NSTL2_THROW_EXCEPTION_IF(handle == INVALID_HANDLE_VALUE, "ICMP handle creation failed: " << ::GetLastError());
    _handle = handle;
}

Ping::~Ping()
{
    if (_handle != nullptr)
    {
        ::IcmpCloseHandle(_handle);
    }
}

bool Ping::ping(const std::chrono::microseconds to_)
{
    if (to_ <= std::chrono::microseconds::zero())
    {
        return false;
    }
    const auto timeout = static_cast<DWORD>(std::chrono::ceil<std::chrono::milliseconds>(to_).count());

    const std::uint64_t payload[2] = { payload_magic, ++_next };
    // room for the reply struct, the echoed payload, an ICMP error and an IO_STATUS_BLOCK
    alignas(8) std::array<std::uint8_t, 256> reply;

    if (_addr.ss_family == AF_INET)
    {
        const auto dst = reinterpret_cast<const sockaddr_in*>(&_addr);
        const DWORD count =
            ::IcmpSendEcho2(_handle, nullptr, nullptr, nullptr, dst->sin_addr.S_un.S_addr,
                            const_cast<std::uint64_t*>(payload), static_cast<WORD>(sizeof(payload)), nullptr,
                            reply.data(), static_cast<DWORD>(reply.size()), timeout);
        return count != 0 && reinterpret_cast<const ICMP_ECHO_REPLY*>(reply.data())->Status == IP_SUCCESS;
    }
    else
    {
        sockaddr_in6 src;
        std::memset(&src, 0, sizeof(src));
        src.sin6_family = AF_INET6;
        auto dst = reinterpret_cast<sockaddr_in6*>(&_addr);
        const DWORD sent = ::Icmp6SendEcho2(_handle, nullptr, nullptr, nullptr, &src, dst,
                                            const_cast<std::uint64_t*>(payload), static_cast<WORD>(sizeof(payload)),
                                            nullptr, reply.data(), static_cast<DWORD>(reply.size()), timeout);
        return sent != 0 && ::Icmp6ParseReplies(reply.data(), static_cast<DWORD>(reply.size())) != 0 &&
               reinterpret_cast<const ICMPV6_ECHO_REPLY*>(reply.data())->Status == IP_SUCCESS;
    }    
}
} // namespace nstl::net
