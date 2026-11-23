#include "config.hpp"

#include <nstl/exception.hpp>
#include <nstl/ip_tools.hpp>
#include <nstl/temp_dir.hpp>

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#include <fstream>

using nstl::ddns::config;
using nstl::ddns::configs;
using nstl::ddns::custom_dns;
using nstl::ddns::parse_file;
using nstl::ddns::parse_payload;
using nstl::net::IpClass;

namespace
{
nstl::net::ipv4_addr ip4(std::string_view text_)
{
    const auto parsed = nstl::net::parse_ip4_address(text_);
    if (!parsed)
    {
        throw std::invalid_argument{ std::string{ text_ } };
    }
    return parsed.value();
}

nstl::net::ipv6_addr ip6(std::string_view text_)
{
    const auto parsed = nstl::net::parse_ip6_address(text_);
    if (!parsed)
    {
        throw std::invalid_argument{ std::string{ text_ } };
    }
    return parsed.value();
}
} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// config defaults / reset / valid
// ---------------------------------------------------------------------------------------------------------------------

TEST(DdnsConfig, DefaultConstructed)
{
    const config cfg;
    EXPECT_TRUE(cfg.enabled);
    EXPECT_TRUE(cfg.table.empty());
    EXPECT_EQ(cfg.ip_class, IpClass::Ipv4);
    EXPECT_TRUE(cfg.ddns_names.empty());
    EXPECT_EQ(cfg.range, 0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(cfg.mask));
    EXPECT_FALSE(cfg.valid());
}

TEST(DdnsConfig, ValidRequiresTableAndNames)
{
    config cfg;
    EXPECT_FALSE(cfg.valid());
    cfg.table = "inet filter";
    EXPECT_FALSE(cfg.valid());
    cfg.ddns_names.push_back("example.com");
    EXPECT_TRUE(cfg.valid());
    cfg.table.clear();
    EXPECT_FALSE(cfg.valid());
}

TEST(DdnsConfig, Reset)
{
    config cfg{ nlohmann::json::parse(R"({"table":"inet filter","type":"ip6","ddns":"example.com","range":64})") };
    ASSERT_TRUE(cfg.valid());
    cfg.reset();
    EXPECT_TRUE(cfg.table.empty());
    EXPECT_EQ(cfg.ip_class, IpClass::Ipv4);
    EXPECT_TRUE(cfg.ddns_names.empty());
    EXPECT_TRUE(std::holds_alternative<std::monostate>(cfg.mask));
    EXPECT_FALSE(cfg.valid());
}

// ---------------------------------------------------------------------------------------------------------------------
// config(json)
// ---------------------------------------------------------------------------------------------------------------------

TEST(DdnsConfig, EmptyObject)
{
    const config cfg{ nlohmann::json::object() };
    EXPECT_TRUE(cfg.enabled);
    EXPECT_TRUE(cfg.table.empty());
    EXPECT_EQ(cfg.ip_class, IpClass::Ipv4);
    EXPECT_TRUE(cfg.ddns_names.empty());
    EXPECT_FALSE(cfg.valid());
}

TEST(DdnsConfig, NonObjectThrows)
{
    EXPECT_THROW(config{ nlohmann::json::array() }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json("text") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json(42) }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json(nullptr) }, nstl::exception);
}

TEST(DdnsConfig, FullConfig)
{
    const config cfg{ nlohmann::json::parse(
        R"({"table":"inet filter","type":"ip4","ddns":["a.example.com","b.example.com"],"range":24,"enabled":false})") };
    EXPECT_FALSE(cfg.enabled);
    EXPECT_EQ(cfg.table, "inet filter");
    EXPECT_EQ(cfg.ip_class, IpClass::Ipv4);
    ASSERT_EQ(cfg.ddns_names.size(), 2u);
    EXPECT_EQ(cfg.ddns_names[0], "a.example.com");
    EXPECT_EQ(cfg.ddns_names[1], "b.example.com");
    EXPECT_EQ(cfg.range, 24);
    ASSERT_TRUE(std::holds_alternative<nstl::net::ipv4_addr>(cfg.mask));
    EXPECT_EQ(std::get<nstl::net::ipv4_addr>(cfg.mask), nstl::net::create_ipv4_mask(24));
    EXPECT_TRUE(cfg.valid());
}

TEST(DdnsConfig, TableMustBeString)
{
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"table":1})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"table":["a"]})") }, nstl::exception);
}

TEST(DdnsConfig, TypeString)
{
    EXPECT_EQ(config{ nlohmann::json::parse(R"({"type":"ip4"})") }.ip_class, IpClass::Ipv4);
    EXPECT_EQ(config{ nlohmann::json::parse(R"({"type":"ip6"})") }.ip_class, IpClass::Ipv6);
}

TEST(DdnsConfig, TypeInteger)
{
    EXPECT_EQ(config{ nlohmann::json::parse(R"({"type":4})") }.ip_class, IpClass::Ipv4);
    EXPECT_EQ(config{ nlohmann::json::parse(R"({"type":6})") }.ip_class, IpClass::Ipv6);
}

TEST(DdnsConfig, TypeInvalid)
{
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":"ip5"})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":"IP4"})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":""})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":5})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":0})") }, nstl::exception);
    // negative integers are not unsigned
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":-4})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":4.0})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":true})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":null})") }, nstl::exception);
}

TEST(DdnsConfig, DdnsSingleString)
{
    const config cfg{ nlohmann::json::parse(R"({"ddns":"example.com"})") };
    ASSERT_EQ(cfg.ddns_names.size(), 1u);
    EXPECT_EQ(cfg.ddns_names.front(), "example.com");
}

TEST(DdnsConfig, DdnsEmptyArray)
{
    const config cfg{ nlohmann::json::parse(R"({"table":"t","ddns":[]})") };
    EXPECT_TRUE(cfg.ddns_names.empty());
    EXPECT_FALSE(cfg.valid());
}

TEST(DdnsConfig, DdnsInvalid)
{
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"ddns":1})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"ddns":{"a":"b"}})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"ddns":["a",1]})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"ddns":[null]})") }, nstl::exception);
}

TEST(DdnsConfig, RangeIpv4)
{
    for (const unsigned int bits : { 0u, 1u, 8u, 16u, 24u, 31u, 32u })
    {
        nlohmann::json src{ { "type", "ip4" }, { "range", bits } };
        const config cfg{ src };
        EXPECT_EQ(cfg.range, bits);
        ASSERT_TRUE(std::holds_alternative<nstl::net::ipv4_addr>(cfg.mask)) << bits;
        EXPECT_EQ(std::get<nstl::net::ipv4_addr>(cfg.mask),
                  nstl::net::create_ipv4_mask(static_cast<std::uint8_t>(bits)))
            << bits;
    }
}

TEST(DdnsConfig, RangeIpv6)
{
    for (const unsigned int bits : { 0u, 1u, 48u, 64u, 127u, 128u })
    {
        nlohmann::json src{ { "type", "ip6" }, { "range", bits } };
        const config cfg{ src };
        EXPECT_EQ(cfg.range, bits);
        ASSERT_TRUE(std::holds_alternative<nstl::net::ipv6_addr>(cfg.mask)) << bits;
        EXPECT_EQ(std::get<nstl::net::ipv6_addr>(cfg.mask),
                  nstl::net::create_ipv6_mask(static_cast<std::uint8_t>(bits)))
            << bits;
    }
}

TEST(DdnsConfig, RangeDefaultsToIpv4WhenTypeMissing)
{
    const config cfg{ nlohmann::json::parse(R"({"range":16})") };
    EXPECT_EQ(cfg.ip_class, IpClass::Ipv4);
    EXPECT_TRUE(std::holds_alternative<nstl::net::ipv4_addr>(cfg.mask));
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"range":64})") }, nstl::exception);
}

TEST(DdnsConfig, RangeOutOfBounds)
{
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":"ip4","range":33})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":"ip6","range":129})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"type":"ip6","range":1000})") }, nstl::exception);
}

TEST(DdnsConfig, RangeInvalidType)
{
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"range":-1})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"range":"24"})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"range":24.5})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"range":null})") }, nstl::exception);
}

TEST(DdnsConfig, Enabled)
{
    EXPECT_TRUE(config{ nlohmann::json::parse(R"({"enabled":true})") }.enabled);
    EXPECT_FALSE(config{ nlohmann::json::parse(R"({"enabled":false})") }.enabled);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"enabled":1})") }, nstl::exception);
    EXPECT_THROW(config{ nlohmann::json::parse(R"({"enabled":"true"})") }, nstl::exception);
}

TEST(DdnsConfig, UnknownKeysIgnored)
{
    const config cfg{ nlohmann::json::parse(R"({"table":"t","ddns":"x","comment":"whatever","extra":[1,2]})") };
    EXPECT_TRUE(cfg.valid());
}

// ---------------------------------------------------------------------------------------------------------------------
// apply_mask
// ---------------------------------------------------------------------------------------------------------------------

TEST(DdnsConfig, ApplyMaskNoMaskIpv4)
{
    const config cfg;
    EXPECT_EQ(cfg.apply_mask(ip4("192.168.1.42")), "192.168.1.42");
}

TEST(DdnsConfig, ApplyMaskNoMaskIpv6)
{
    config cfg;
    cfg.ip_class = IpClass::Ipv6;
    EXPECT_EQ(cfg.apply_mask(ip6("2001:db8::1")), "2001:db8::1");
}

TEST(DdnsConfig, ApplyMaskIpv4)
{
    const config cfg24{ nlohmann::json::parse(R"({"type":"ip4","range":24})") };
    EXPECT_EQ(cfg24.apply_mask(ip4("192.168.1.42")), "192.168.1.0/24");

    const config cfg16{ nlohmann::json::parse(R"({"type":"ip4","range":16})") };
    EXPECT_EQ(cfg16.apply_mask(ip4("10.20.30.40")), "10.20.0.0/16");

    const config cfg32{ nlohmann::json::parse(R"({"type":"ip4","range":32})") };
    EXPECT_EQ(cfg32.apply_mask(ip4("10.20.30.40")), "10.20.30.40/32");

    const config cfg0{ nlohmann::json::parse(R"({"type":"ip4","range":0})") };
    EXPECT_EQ(cfg0.apply_mask(ip4("10.20.30.40")), "0.0.0.0/0");
}

TEST(DdnsConfig, ApplyMaskIpv6)
{
    const config cfg64{ nlohmann::json::parse(R"({"type":"ip6","range":64})") };
    EXPECT_EQ(cfg64.apply_mask(ip6("2001:db8:1:2:3:4:5:6")), "2001:db8:1:2::/64");

    const config cfg48{ nlohmann::json::parse(R"({"type":"ip6","range":48})") };
    EXPECT_EQ(cfg48.apply_mask(ip6("2001:db8:1:2:3:4:5:6")), "2001:db8:1::/48");

    const config cfg128{ nlohmann::json::parse(R"({"type":"ip6","range":128})") };
    EXPECT_EQ(cfg128.apply_mask(ip6("2001:db8::1")), "2001:db8::1/128");
}

TEST(DdnsConfig, ApplyMaskSameSubnetCollapses)
{
    const config cfg{ nlohmann::json::parse(R"({"type":"ip4","range":24})") };
    EXPECT_EQ(cfg.apply_mask(ip4("192.168.1.1")), cfg.apply_mask(ip4("192.168.1.254")));
    EXPECT_NE(cfg.apply_mask(ip4("192.168.1.1")), cfg.apply_mask(ip4("192.168.2.1")));
}

TEST(DdnsConfig, ApplyMaskFamilyMismatchThrows)
{
    const config cfg4{ nlohmann::json::parse(R"({"type":"ip4","range":24})") };
    EXPECT_THROW(cfg4.apply_mask(ip6("2001:db8::1")), std::exception);
    const config cfg6{ nlohmann::json::parse(R"({"type":"ip6","range":64})") };
    EXPECT_THROW(cfg6.apply_mask(ip4("10.0.0.1")), std::exception);
}

// ---------------------------------------------------------------------------------------------------------------------
// parse_payload / parse_file
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
constexpr std::string_view sample_payload = R"({ "items": {
    "home": { "table": "inet filter", "type": "ip4", "ddns": "home.example.com" },
    "office": { "table": "inet filter", "type": 6, "ddns": ["a.example.com", "b.example.com"], "range": 64, "enabled": false }
}
})";

void check_sample(const std::pair<configs, custom_dns>& cfgs_)
{
    const auto& cfgs = cfgs_.first;
    ASSERT_EQ(cfgs.size(), 2u);
    const auto home = cfgs.find("home");
    ASSERT_NE(home, cfgs.end());
    EXPECT_EQ(home->second.table, "inet filter");
    EXPECT_EQ(home->second.ip_class, IpClass::Ipv4);
    EXPECT_EQ(home->second.ddns_names, std::vector<std::string>{ "home.example.com" });
    EXPECT_TRUE(home->second.enabled);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(home->second.mask));

    const auto office = cfgs.find("office");
    ASSERT_NE(office, cfgs.end());
    EXPECT_EQ(office->second.ip_class, IpClass::Ipv6);
    EXPECT_EQ(office->second.ddns_names.size(), 2u);
    EXPECT_EQ(office->second.range, 64);
    EXPECT_FALSE(office->second.enabled);
}
} // namespace

TEST(DdnsParse, PayloadString) { check_sample(parse_payload(sample_payload)); }

TEST(DdnsParse, PayloadJson) { check_sample(parse_payload(nlohmann::json::parse(sample_payload))); }

TEST(DdnsParse, EmptyObject) { EXPECT_TRUE(parse_payload(std::string_view{ "{}" }).first.empty()); }

TEST(DdnsParse, NotAnObject)
{
    EXPECT_THROW(parse_payload(std::string_view{ "[]" }), nstl::exception);
    EXPECT_THROW(parse_payload(std::string_view{ "\"text\"" }), nstl::exception);
}

TEST(DdnsParse, MalformedJson)
{
    EXPECT_THROW(parse_payload(std::string_view{ "{" }), nlohmann::json::parse_error);
    EXPECT_THROW(parse_payload(std::string_view{ "" }), nlohmann::json::parse_error);
}

TEST(DdnsParse, InvalidEntryRejected)
{
    // missing ddns
    EXPECT_THROW(parse_payload(std::string_view{ R"({"items":{"x":{"table":"t"}}})" }), nstl::exception);
    // missing table
    EXPECT_THROW(parse_payload(std::string_view{ R"({"items":{"x":{"ddns":"a"}}})" }), nstl::exception);
    // entry not an object
    EXPECT_THROW(parse_payload(std::string_view{ R"({"items":{"x":"y"}})" }), nstl::exception);
    // nested field error propagates
    EXPECT_THROW(parse_payload(std::string_view{ R"({"items":{"x":{"table":"t","ddns":"a","type":"ip9"}}})" }),
                 nstl::exception);
}

TEST(DdnsParse, DisabledEntryStillValidated)
{
    EXPECT_THROW(parse_payload(std::string_view{ R"({"items":{"x":{"table":"t","enabled":false}}})" }),
                 nstl::exception);
}

TEST(DdnsParse, File)
{
    const nstl::temp_dir dir;
    const auto path = dir / "ddns.json";
    {
        std::ofstream out{ path, std::ios_base::binary };
        out << sample_payload;
    }
    check_sample(parse_file(path));
}

TEST(DdnsParse, MissingFile)
{
    const nstl::temp_dir dir;
    EXPECT_ANY_THROW(parse_file(dir / "does_not_exist.json"));
}
