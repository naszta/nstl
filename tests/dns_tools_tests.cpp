#include <nstl/dns_tools.hpp>

#include <gtest/gtest.h>

TEST(DnsTools, CannonName)
{
    const auto cname = nstl::net::canonical_name("media.naszta.hu");
    ASSERT_TRUE(cname.has_value());
    EXPECT_EQ(cname.value(), "harmonia.bysh.me");
}

TEST(DnsTools, Hostname)
{
    const auto hostname = nstl::net::hostname();
    EXPECT_FALSE(hostname.empty());
}

TEST(DnsTools, MxName)
{
    const auto mxname = nstl::net::mx_name("naszta.com");
    ASSERT_TRUE(mxname.has_value());
    EXPECT_LE(1, mxname->size());
}

TEST(DnsTools, Txt)
{
    const auto txtnames = nstl::net::txt_name("test.naszta.com");
    ASSERT_TRUE(txtnames.has_value());
    const auto& txts = txtnames.value();
    ASSERT_GE(txts.size(), 1U);
    EXPECT_EQ(txts.front(),
              "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean "
              "massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam "
              "felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. Donec pede "
              "justo, fringilla vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, imperdiet a, "
              "venenatis vitae, justo. Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras dapibu");
}

TEST(DnsTools, Cname)
{
    const auto cnames = nstl::net::c_name("autodiscover.naszta.hu");
    ASSERT_TRUE(cnames.has_value());
    const auto& items = cnames.value();
    ASSERT_GE(items.size(), 1U);
    EXPECT_EQ(items.front(), "autodiscover.outlook.com");
}
