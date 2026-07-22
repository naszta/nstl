#include <nstl/dns_tools.hpp>

#include <gtest/gtest.h>

TEST(DnsTools, CannonName)
{
	const auto cname = nstl::net::cannonical_name("media.naszta.hu");
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
}

TEST(DnsTools, Txt)
{
	const auto txtnames = nstl::net::txt_name("naszta.com");
	ASSERT_TRUE(txtnames.has_value());
	const auto& txts = txtnames.value();
	ASSERT_EQ(txts.size(), 1U);
	EXPECT_EQ(txts.front(), "v=spf1 include:_spf.google.com -all");
}

TEST(DnsTools, Cname)
{
	const auto cnames = nstl::net::c_name("autodiscover.naszta.hu");
	ASSERT_TRUE(cnames.has_value());
	const auto& items = cnames.value();
	EXPECT_EQ(items.size(), 1U);
	EXPECT_EQ(items.front(), "autodiscover.outlook.com");
}
