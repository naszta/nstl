#include <nstl/ip_tools.hpp>

#include <gtest/gtest.h>

TEST(IpTools, Masking)
{
    const auto long_test = nstl::net::parse_ip4_address("255.255.255.255");
    EXPECT_TRUE(long_test.has_value());
    const auto range_opt = nstl::net::parse_ip4_range("192.168.1.0/24");
    const auto range_opt_other = nstl::net::parse_ip4_range("192.168.1.0/255.255.255.0");
    EXPECT_EQ(range_opt, range_opt_other);
    ASSERT_TRUE(range_opt.has_value());
    const auto subnet_ip = nstl::net::parse_ip4_address("255.255.255.0");
    const auto generated_ip = nstl::net::create_ipv4_mask(24);
    const auto test_ip_pass = nstl::net::parse_ip4_address("192.168.1.17");
    const auto test_ip_fail = nstl::net::parse_ip4_address("192.168.0.17");
    EXPECT_EQ(subnet_ip, generated_ip);
    ASSERT_TRUE(test_ip_pass.has_value());
    EXPECT_TRUE(range_opt->contains(test_ip_pass.value()));
    ASSERT_TRUE(test_ip_fail.has_value());
    EXPECT_FALSE(range_opt->contains(test_ip_fail.value()));

    const auto range_opt_ip_only = nstl::net::parse_ip4_range("192.168.1.17");
    ASSERT_TRUE(range_opt_ip_only.has_value());
    EXPECT_EQ(range_opt_ip_only->ip, test_ip_pass);
    EXPECT_EQ(range_opt_ip_only->mask, long_test);
}

TEST(IpTools, BasicCheck)
{
    {
        const auto address_v6 = nstl::net::parse_ip_address("2606:4700::6812:1c07");
        const auto ipv6ptr = std::get_if<nstl::net::ipv6_addr>(&address_v6);
        ASSERT_NE(ipv6ptr, nullptr);
        EXPECT_EQ(nstl::net::to_string(*ipv6ptr), "2606:4700::6812:1c07");
        const auto ipv4 = nstl::net::is_ipv4(*ipv6ptr);
        EXPECT_FALSE(ipv4.has_value());
    }
    {
        const auto address_v4 = nstl::net::parse_ip_address("192.168.1.254");
        const auto ipv4ptr = std::get_if<nstl::net::ipv4_addr>(&address_v4);
        ASSERT_NE(ipv4ptr, nullptr);
        EXPECT_EQ(nstl::net::to_string(*ipv4ptr), "192.168.1.254");
    }
    {
        const auto tricky = nstl::net::parse_ip_address("0::ffff:0101:0101");
        const auto trickyptr4 = std::get_if<nstl::net::ipv4_addr>(&tricky);
        const auto trickyptr6 = std::get_if<nstl::net::ipv6_addr>(&tricky);
        EXPECT_EQ(trickyptr4, nullptr);
        ASSERT_NE(trickyptr6, nullptr);
        const auto ip4val = nstl::net::is_ipv4(*trickyptr6);
        ASSERT_TRUE(ip4val.has_value());
        EXPECT_EQ(nstl::net::to_string(*ip4val), "1.1.1.1");
        EXPECT_EQ(nstl::net::to_string(*trickyptr6), "::ffff:1.1.1.1");
    }
}
