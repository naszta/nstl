#include <nstl/exception.hpp>
#include <nstl/ping.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

namespace
{
template <typename Target> std::unique_ptr<nstl::net::Ping> make_ping(const Target& target_)
{
    try
    {
        return std::make_unique<nstl::net::Ping>(target_);
    }
    catch (const nstl::exception&)
    {
        // e.g. Linux without the user in net.ipv4.ping_group_range
        return nullptr;
    }
}
} // namespace

TEST(Ping, LocalhostIpv4)
{
    const auto ping = make_ping(nstl::net::ip_addr_gen{ nstl::net::ipv4_addr{ 127, 0, 0, 1 } });
    if (!ping)
    {
        GTEST_SKIP() << "ICMP sockets are not permitted";
    }
    EXPECT_TRUE(ping->ping(1s));
    EXPECT_TRUE(ping->ping(1s));
}

TEST(Ping, LocalhostByName)
{
    const auto ping = make_ping(std::string_view{ "127.0.0.1" });
    if (!ping)
    {
        GTEST_SKIP() << "ICMP sockets are not permitted";
    }
    EXPECT_TRUE(ping->ping(1s));
}

TEST(Ping, UnreachableTimesOut)
{
    // TEST-NET-1, RFC 5737: never answers
    const auto ping = make_ping(nstl::net::ip_addr_gen{ nstl::net::ipv4_addr{ 192, 0, 2, 1 } });
    if (!ping)
    {
        GTEST_SKIP() << "ICMP sockets are not permitted";
    }
    const auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(ping->ping(200ms));
    EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
}

TEST(Ping, UnresolvableName)
{
    EXPECT_THROW(nstl::net::Ping{ std::string_view{ "nonexistent.invalid" } }, nstl::exception);
}
