#include "ping.hpp"

#include <nstl/exception.hpp>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <tuple>
#include <utility>
#include <variant>

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

namespace
{
constexpr int store_size = sizeof(struct sockaddr_storage);
constexpr std::uint64_t payload_magic = 0x6e73746c70696e67ULL; // "nstlping"

constexpr std::uint8_t ICMP4_ECHO_REQUEST = 8;
constexpr std::uint8_t ICMP4_ECHO_REPLY = 0;
constexpr std::uint8_t ICMP6_ECHO_REQ = 128;
constexpr std::uint8_t ICMP6_ECHO_REP = 129;

struct EchoHdr
{
    std::uint8_t type;
    std::uint8_t code;
    std::uint16_t cksum;
    std::uint16_t id;
    std::uint16_t seq;
};
static_assert(sizeof(EchoHdr) == 8, "unexpected padding");

// RFC 1071 internet checksum, returned in network byte order
std::uint16_t checksum(const std::uint8_t* data_, const std::size_t size_)
{
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i + 1 < size_; i += 2)
    {
        sum += static_cast<std::uint32_t>(data_[i] << 8 | data_[i + 1]);
    }
    if (size_ % 2 != 0)
    {
        sum += static_cast<std::uint32_t>(data_[size_ - 1] << 8);
    }
    while (sum >> 16 != 0)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return htons(static_cast<std::uint16_t>(~sum));
}

struct SocketVisitor
{
    sockaddr_storage& _addr;
    explicit SocketVisitor(sockaddr_storage& addr_) : _addr{ addr_ } {}

    std::pair<int, int> operator()(const nstl::net::ipv4_addr& ip_) const
    {
        constexpr int address_size = sizeof(struct sockaddr_in);
        static_assert(address_size <= store_size);

        const int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
        auto ptr = reinterpret_cast<sockaddr_in*>(&_addr);

        ptr->sin_family = AF_INET;
        std::memcpy(&ptr->sin_addr, ip_.data(), ip_.size());
        return std::make_pair(fd, address_size);
    }

    std::pair<int, int> operator()(const nstl::net::ipv6_addr& ip_) const
    {
        constexpr int address_size = sizeof(struct sockaddr_in6);
        static_assert(address_size <= store_size);

        const int fd = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6);
        auto ptr = reinterpret_cast<sockaddr_in6*>(&_addr);

        ptr->sin6_family = AF_INET6;
        std::memcpy(&ptr->sin6_addr, ip_.data(), ip_.size());
        return std::make_pair(fd, address_size);
    }
};
} // namespace

namespace nstl::net
{
Ping::Ping(const ip_addr_gen& ip_)
{
    std::memset(&_addr, 0, store_size);
    std::tie(_sock, _addr_len) = std::visit(SocketVisitor{ _addr }, ip_);
    NSTL2_THROW_EXCEPTION_IF(_sock < 0, "ICMP socket creation failed: " << std::strerror(errno));
    ::fcntl(_sock, F_SETFD, FD_CLOEXEC);
    ::fcntl(_sock, F_SETFL, ::fcntl(_sock, F_GETFL) | O_NONBLOCK);
}

Ping::~Ping()
{
    if (0 <= _sock)
    {
        ::close(_sock);
    }
}

bool Ping::ping(const std::chrono::microseconds to_)
{
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + to_;
    const bool is_v4 = _addr.ss_family == AF_INET;

    EchoHdr hdr;
    hdr.type = is_v4 ? ICMP4_ECHO_REQUEST : ICMP6_ECHO_REQ;
    hdr.code = 0;
    hdr.cksum = 0;
    // Linux ping sockets overwrite the identifier with the socket's local port
    hdr.id = htons(static_cast<std::uint16_t>(::getpid()));
    hdr.seq = htons(static_cast<std::uint16_t>(++_next));

    const std::uint64_t payload[2] = { payload_magic, _next };
    std::array<std::uint8_t, sizeof(EchoHdr) + sizeof(payload)> pkt;
    std::memcpy(pkt.data(), &hdr, sizeof(hdr));
    std::memcpy(pkt.data() + sizeof(hdr), payload, sizeof(payload));
    if (is_v4)
    {
        // the kernel always fills the ICMPv6 checksum, as it needs the pseudo header
        const auto sum = checksum(pkt.data(), pkt.size());
        std::memcpy(pkt.data() + offsetof(EchoHdr, cksum), &sum, sizeof(sum));
    }

    const auto sent = ::sendto(_sock, pkt.data(), pkt.size(), 0, reinterpret_cast<const sockaddr*>(&_addr),
                               static_cast<socklen_t>(_addr_len));
    if (sent != static_cast<ssize_t>(pkt.size()))
    {
        return false;
    }

    std::array<std::uint8_t, 1500> buffer;
    for (auto now = clock::now(); now < deadline; now = clock::now())
    {
        const auto wait = std::chrono::ceil<std::chrono::milliseconds>(deadline - now);
        pollfd pfd{ _sock, POLLIN, 0 };
        const int ready = ::poll(&pfd, 1, static_cast<int>(wait.count()));
        if (ready < 0 && errno != EINTR)
        {
            return false;
        }
        if (ready <= 0)
        {
            continue;
        }

        sockaddr_storage from;
        socklen_t from_len = sizeof(from);
        const auto received =
            ::recvfrom(_sock, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (received < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            return false;
        }

        // MacOS delivers the IPv4 header too, Linux does not (an ICMP type never looks like 0x4X)
        std::size_t offset = 0;
        if (is_v4 && 0 < received && buffer[0] >> 4 == 4)
        {
            offset = static_cast<std::size_t>(buffer[0] & 0x0F) * 4;
        }
        if (static_cast<std::size_t>(received) < offset + sizeof(EchoHdr))
        {
            continue;
        }

        EchoHdr answer;
        std::memcpy(&answer, buffer.data() + offset, sizeof(answer));
        if (answer.type != (is_v4 ? ICMP4_ECHO_REPLY : ICMP6_ECHO_REP) || answer.seq != hdr.seq)
        {
            continue;
        }
#ifndef __linux__
        if (answer.id != hdr.id)
        {
            continue;
        }
#endif
        if (is_v4 ? std::memcmp(&reinterpret_cast<const sockaddr_in*>(&from)->sin_addr,
                                &reinterpret_cast<const sockaddr_in*>(&_addr)->sin_addr, sizeof(in_addr)) != 0
                  : std::memcmp(&reinterpret_cast<const sockaddr_in6*>(&from)->sin6_addr,
                                &reinterpret_cast<const sockaddr_in6*>(&_addr)->sin6_addr, sizeof(in6_addr)) != 0)
        {
            continue;
        }
        return true;
    }
    return false;
}
} // namespace nstl::net
