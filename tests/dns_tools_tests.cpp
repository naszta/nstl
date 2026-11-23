#include <nstl/dns_tools.hpp>
#include <nstl/logging.hpp>
#include <nstl/env_var_raii.hpp>
#include <nstl/range_print.hpp>

#include <gtest/gtest.h>

#include <set>
#include <thread>

TEST(DnsTools, AandAAA)
{
    const auto ip_opt = nstl::net::ips_name("naszta.hu");
    EXPECT_TRUE(ip_opt.has_value());
}

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
    oss_https << nstl::range_print(https_opt.value(), "; ");
    EXPECT_EQ(oss_https.view(), ". 1 [ALPNS={\"h3\",\"h2\"} IPV4S={104.18.28.7,104.18.29.7} "
                                "IPV6S={2606:4700::6812:1c07,2606:4700::6812:1d07}]");
    NSTL_INFO("HTTPS request passed " << oss_https.view());

    const auto https2_opt = nstl::net::svcb_name("https.tepj.be", nstl::net::SvcbType::Https);
    ASSERT_TRUE(https2_opt.has_value());
    std::ostringstream oss_https2;
    oss_https2 << nstl::range_print(https2_opt.value(), "; ");
    EXPECT_EQ(oss_https2.view(), "naszta.london 42 [ALPNS={\"h2\",\"h3\"} PORT=443]");
    NSTL_INFO("HTTPS request passed " << oss_https2.view());

    const auto svcb_opt = nstl::net::svcb_name("svcb.tepj.be");
    ASSERT_TRUE(svcb_opt.has_value());
    std::ostringstream oss_svcb;
    oss_svcb << nstl::range_print(svcb_opt.value(), "; ");
    EXPECT_EQ(oss_svcb.view(), "naszta.london 43 [ALPNS={\"h2\",\"h3\"} PORT=853]");
    NSTL_INFO("SVCB request passed " << oss_svcb.view());
}

TEST(DnsTools, SvcbResolver)
{
    if (const auto dns_value = nstl::get_env_var("NSTL_RESOLVER_TEST_ENABLED"); !dns_value.empty())
    {
        const auto svcb_opt = nstl::net::svcb_name("_dns.resolver.arpa");
        ASSERT_TRUE(svcb_opt.has_value());
        const auto& dns_values = svcb_opt.value();
        const auto itr = std::ranges::find_if(dns_values, [dns_value](const nstl::net::gen_svcb& item)
                                              { return item.address == dns_value; });
        ASSERT_NE(itr, dns_values.cend())
            << dns_value << " cannot be resolved as _dns.resolver.arpa (values: " << nstl::range_print(dns_values, ", ")
            << ')';
        NSTL_INFO("DNS found: " << nstl::range_print(dns_values, "; "));
    }
}

namespace
{
void print_ips(const std::vector<nstl::net::ip_addr_gen>& ips_, std::ostringstream& oss_)
{
    auto itr = ips_.cbegin();
    oss_ << '[';
    nstl::net::to_stream(oss_, *itr);
    while (++itr != ips_.cend())
    {
        oss_ << ", ";
        nstl::net::to_stream(oss_, *itr);
    }
    oss_ << ']';
}
} // namespace

TEST(DnsTools, AandAAACheck)
{
    const auto ipv6_around = nstl::net::box_have_valid_ipv6();
    NSTL_INFO("IPv6 on this box: " << (ipv6_around ? "enabled" : "disabled"));
    const auto target = ipv6_around ? nstl::net::IpClass::IpvAll : nstl::net::IpClass::Ipv4;
    const auto ip_opt_a = nstl::net::ips_name("naszta.hu", target);
    const auto ip_opt_b = nstl::net::ips_name_libc("naszta.hu", target);
    ASSERT_TRUE(ip_opt_a.has_value());
    ASSERT_TRUE(ip_opt_b.has_value());
    const std::set<nstl::net::ip_addr_gen> sorted_a{ ip_opt_a->cbegin(), ip_opt_a->cend() };
    const std::set<nstl::net::ip_addr_gen> sorted_b{ ip_opt_b->cbegin(), ip_opt_b->cend() };
    if (sorted_a != sorted_b)
    {
        std::ostringstream side_a_str, side_b_str;
        print_ips(ip_opt_a.value(), side_a_str);
        print_ips(ip_opt_b.value(), side_b_str);
        ADD_FAILURE() << "A: " << side_a_str.view() << " vs. B: " << side_b_str.view();
    }
}

TEST(DnsTools, CustomDNSTest)
{
    nstl::net::dns_config config;
    config.dns_servers.push_back(nstl::net::parse_ip4_address("1.1.1.1").value());
    const auto ip_opt_a = nstl::net::ips_name("naszta.hu", nstl::net::IpClass::IpvAll, nstl::make_observer(&config));
    EXPECT_TRUE(ip_opt_a.has_value());
}

TEST(DnsTools, CustomDNSCachedStateReuse)
{
    nstl::net::dns_config config;
    config.dns_servers.push_back(nstl::net::parse_ip4_address("1.1.1.1").value());
    nstl::net::dns_config config_swapped;
    config_swapped.dns_servers.push_back(nstl::net::parse_ip4_address("8.8.8.8").value());
    config_swapped.dns_servers.push_back(nstl::net::parse_ip4_address("1.1.1.1").value());

    const auto run_lookups = [&]
    {
        for (int i = 0; i < 3; ++i)
        {
            EXPECT_TRUE(nstl::net::ip_a_name("naszta.hu", nstl::make_observer(&config)).has_value());
            EXPECT_TRUE(nstl::net::ip_a_name("naszta.hu", nstl::make_observer(&config_swapped)).has_value());
            EXPECT_TRUE(nstl::net::ip_a_name("naszta.hu").has_value());
        }
    };

    run_lookups();
    // cached states are released at thread exit
    std::thread worker{ run_lookups };
    worker.join();
}
