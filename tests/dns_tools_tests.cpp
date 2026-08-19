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

TEST(DnsTools, NullptrThrows)
{
    EXPECT_THROW(nstl::net::canonical_name(static_cast<const char*>(nullptr)), std::exception);
    EXPECT_THROW(nstl::net::mx_name(static_cast<const char*>(nullptr)), std::exception);
    EXPECT_THROW(nstl::net::txt_name(static_cast<const char*>(nullptr)), std::exception);
    EXPECT_THROW(nstl::net::c_name(static_cast<const char*>(nullptr)), std::exception);
    EXPECT_THROW(nstl::net::srv_name(static_cast<const char*>(nullptr)), std::exception);
}

TEST(DnsTools, NonResolvingNameReturnsNullopt)
{
    // ".invalid" is reserved by RFC 2606 to never resolve, so this is deterministic without depending
    // on any live infrastructure.
    constexpr const char* unresolvable = "this-host-should-never-exist.invalid";
    // mx_name/txt_name/c_name/srv_name are backed by res_nquery and report "no such record" as
    // nullopt.
    EXPECT_FALSE(nstl::net::mx_name(unresolvable).has_value());
    EXPECT_FALSE(nstl::net::txt_name(unresolvable).has_value());
    EXPECT_FALSE(nstl::net::c_name(unresolvable).has_value());
    EXPECT_FALSE(nstl::net::srv_name(unresolvable).has_value());
    EXPECT_FALSE(nstl::net::canonical_name(unresolvable).has_value());
}

TEST(DnsTools, StringOverloads)
{
    const std::string name{ "media.naszta.hu" };
    const auto cname = nstl::net::canonical_name(name);
    ASSERT_TRUE(cname.has_value());
    EXPECT_EQ(cname.value(), "harmonia.bysh.me");
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

TEST(DnsTools, Srv)
{
    const auto srvs_opt = nstl::net::srv_name("_ping._udp.test.naszta.com");
    ASSERT_TRUE(srvs_opt.has_value());
    const auto& items = srvs_opt.value();
    ASSERT_GE(items.size(), 1U);
    const auto& front = items.front();
    EXPECT_EQ(front.port, 1024);
    EXPECT_EQ(front.priority, 5);
    EXPECT_EQ(front.weight, 50);
    EXPECT_EQ(front.address, "test.naszta.com");
}

TEST(DnsTools, Svcb)
{
    const auto https_opt = nstl::net::svcb_name("blog.cloudflare.com", nstl::net::SvcbType::Https);
    ASSERT_TRUE(https_opt.has_value());
    std::ostringstream oss_https;
    for (const auto& item : https_opt.value())
    {
        oss_https << item << "\n";
    }
    EXPECT_EQ(oss_https.view(), ". 1 [ALPNS={\"h3\",\"h2\"} IPV4S={104.18.28.7,104.18.29.7} "
                                "IPV6S={2606:4700::6812:1c07,2606:4700::6812:1d07}]\n");

    // Enable when _dns.resolver.arpa is around
    /*/
    const auto svcb_opt = nstl::net::svcb_name("_dns.resolver.arpa");
    ASSERT_TRUE(svcb_opt.has_value());
    std::ostringstream oss_svcb;
    for (const auto& item : svcb_opt.value())
    {
        oss_svcb << item << "\n";
    }
    EXPECT_EQ(oss_svcb.view(), "dns.controld.com 1 [ALPNS={\"h3\",\"h2\",\"http/1.1\"} PORT=443 DOH=\"/1vuatasnf7m{?dns}\"]\n"
                               "1vuatasnf7m.dns.controld.com 2 [ALPNS={\"dot\"} PORT=853]\n"
                               "1vuatasnf7m.dns.controld.com 3 [ALPNS={\"doq\"} PORT=853]\n");
    //*/
}
