#include <nstl/spf_tools.hpp>

#include <gtest/gtest.h>

TEST(SpfTools, Example)
{
    nstl::net::spf::rules_type rules;
	nstl::net::spf::parse_spf("hwsw.hu", "v=spf1 mx ip4:185.43.207.93 ip4:185.43.207.91 ip4:185.43.206.168 ip4:185.43.206.127 ip4:185.43.206.128 ip4:185.80.48.200 ip6:2a01:6ee0:1::27:1 include:_spf.mito.hu include:servers.mcsv.net include:_spf.google.com include:spf.mandrillapp.com include:cspf.rackforest.hu ~all",
        [&rules](auto rule) { rules.emplace_back(std::move(rule)); });
    EXPECT_FALSE(rules.empty());
}