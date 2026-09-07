#include <nstl/version.hpp>

#include <gtest/gtest.h>

#include <regex>

TEST(VersionTest, ContentCheck)
{
    using results_type = std::match_results<std::string_view::const_iterator>;

    const std::regex version_check{ "^(\\d+)\\.(\\d+)\\.(\\w+)$" };
    const auto version = nstl::version();

    results_type results;
    EXPECT_TRUE(std::regex_match(version.begin(), version.end(), results, version_check))
        << version << " is not proper version string";
    ASSERT_EQ(results.size(), 4U);
    const auto& build_ver = results[3];
    EXPECT_NE(build_ver.compare("0"), 0) << "cmake build generation failed";
}
