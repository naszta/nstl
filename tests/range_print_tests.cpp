#include <nstl/range_print.hpp>

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

TEST(RangePrint, EmptyTest)
{
	const std::vector<std::string> values;
	std::ostringstream oss;
	oss << nstl::range_print(values, ',');
	const auto value = oss.str();
	EXPECT_TRUE(value.empty());
}

TEST(RangePrint, BasicTest)
{
	const std::vector<std::string> values{"1", "2", "3"};
	std::ostringstream oss;
	oss << nstl::range_print(values, ',');
	const auto value = oss.str();
	EXPECT_FALSE(value.empty());
	EXPECT_EQ(value, "1,2,3");
}

TEST(RangePrint, MapEmptyTest)
{
	const std::map<std::string, int> values;
	std::ostringstream oss;
	oss << nstl::range_map_print(values, ',', "->");
	const auto value = oss.str();
	EXPECT_TRUE(value.empty());
}

TEST(RangePrint, BasicMapTest)
{
	const std::map<std::string, int> values{ {"one", 1}, {"two", 2}, {"three", 3} };
	std::ostringstream oss;
	oss << nstl::range_map_print(values, ',', "->");
	const auto value = oss.str();
	EXPECT_FALSE(value.empty());
	EXPECT_EQ(value, "one->1,three->3,two->2");
}