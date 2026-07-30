#include <nstl/string.hpp>

#include <gtest/gtest.h>

TEST(String, Trim)
{
	const std::string_view test_item_0{" tada "};
	const std::string_view test_item_1{ "no trim" };

	EXPECT_EQ(nstl::trim_view(test_item_0), "tada");
	EXPECT_EQ(nstl::left_trim_view(test_item_0), "tada ");
	EXPECT_EQ(nstl::right_trim_view(test_item_0), " tada");

	EXPECT_EQ(nstl::trim_view(test_item_1), test_item_1);
	EXPECT_EQ(nstl::left_trim_view(test_item_1), test_item_1);
	EXPECT_EQ(nstl::right_trim_view(test_item_1), test_item_1);
}

TEST(String, Split)
{
	const std::string_view source0{ ",,value0,,value1,," };
	const std::string_view source1{ "value3,,value0,,value1,,value2" };

	std::vector<std::string> items;
	items.reserve(10);

	nstl::split_view_func(source0, ',',
		[&items](const std::string_view item_) {
			items.emplace_back(item_.data(), item_.size());
		}, true);

	ASSERT_EQ(items.size(), 2U);
	EXPECT_EQ(items.front(), "value0");
	EXPECT_EQ(items.back(), "value1");

	items.clear();
	nstl::split_view_func(source0, ',',
		[&items](const std::string_view item_) {
			items.emplace_back(item_.data(), item_.size());
		}, false);

	ASSERT_EQ(items.size(), 7U);
	EXPECT_TRUE(items[0].empty());
	EXPECT_TRUE(items[1].empty());
	EXPECT_EQ(items[2], "value0");
	EXPECT_TRUE(items[3].empty());
	EXPECT_EQ(items[4], "value1");
	EXPECT_TRUE(items[5].empty());
	EXPECT_TRUE(items[6].empty());

	items.clear();
	nstl::split_view_func(source1, ',',
		[&items](const std::string_view item_) {
			items.emplace_back(item_.data(), item_.size());
		}, true);

	ASSERT_EQ(items.size(), 4U);
	EXPECT_EQ(items.front(), "value3");
	EXPECT_EQ(items.back(), "value2");

	items.clear();
	nstl::split_view_func(source1, ',',
		[&items](const std::string_view item_) {
			items.emplace_back(item_.data(), item_.size());
		}, false);

	ASSERT_EQ(items.size(), 7U);
	EXPECT_EQ(items[0], "value3");
	EXPECT_TRUE(items[1].empty());
	EXPECT_EQ(items[2], "value0");
	EXPECT_TRUE(items[3].empty());
	EXPECT_EQ(items[4], "value1");
	EXPECT_TRUE(items[5].empty());
	EXPECT_EQ(items[6], "value2");
}
