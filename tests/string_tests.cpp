#include <nstl/string.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(String, Trim)
{
    const std::string_view test_item_0{ " tada " };
    const std::string_view test_item_1{ "no trim" };

    EXPECT_EQ(nstl::trim_view(test_item_0), "tada");
    EXPECT_EQ(nstl::left_trim_view(test_item_0), "tada ");
    EXPECT_EQ(nstl::right_trim_view(test_item_0), " tada");

    EXPECT_EQ(nstl::trim_view(test_item_1), test_item_1);
    EXPECT_EQ(nstl::left_trim_view(test_item_1), test_item_1);
    EXPECT_EQ(nstl::right_trim_view(test_item_1), test_item_1);
}

TEST(String, TrimEdgeCases)
{
    const std::string_view empty{};
    EXPECT_TRUE(nstl::trim_view(empty).empty());
    EXPECT_TRUE(nstl::left_trim_view(empty).empty());
    EXPECT_TRUE(nstl::right_trim_view(empty).empty());

    const std::string_view all_whitespace{ "   \t  " };
    EXPECT_TRUE(nstl::trim_view(all_whitespace).empty());
    EXPECT_TRUE(nstl::left_trim_view(all_whitespace).empty());
    EXPECT_TRUE(nstl::right_trim_view(all_whitespace).empty());
}

TEST(String, TrimWide)
{
    const std::wstring_view padded{ L" tada " };
    EXPECT_EQ(nstl::trim_view(padded), L"tada");
    EXPECT_EQ(nstl::left_trim_view(padded), L"tada ");
    EXPECT_EQ(nstl::right_trim_view(padded), L" tada");
}

TEST(String, Split)
{
    const std::string_view source0{ ",,value0,,value1,," };
    const std::string_view source1{ "value3,,value0,,value1,,value2" };

    std::vector<std::string> items;
    const auto inserter = [&items](const std::string_view item_) { items.emplace_back(item_.data(), item_.size()); };
    items.reserve(10);

    nstl::split_view_func(source0, ',', inserter, true);

    ASSERT_EQ(items.size(), 2U);
    EXPECT_EQ(items.front(), "value0");
    EXPECT_EQ(items.back(), "value1");

    items.clear();
    nstl::split_view_func(source0, ',', inserter, false);

    ASSERT_EQ(items.size(), 7U);
    EXPECT_TRUE(items[0].empty());
    EXPECT_TRUE(items[1].empty());
    EXPECT_EQ(items[2], "value0");
    EXPECT_TRUE(items[3].empty());
    EXPECT_EQ(items[4], "value1");
    EXPECT_TRUE(items[5].empty());
    EXPECT_TRUE(items[6].empty());

    items.clear();
    nstl::split_view_func(source1, ',', inserter, true);

    ASSERT_EQ(items.size(), 4U);
    EXPECT_EQ(items.front(), "value3");
    EXPECT_EQ(items.back(), "value2");

    items.clear();
    nstl::split_view_func(source1, ',', inserter, false);

    ASSERT_EQ(items.size(), 7U);
    EXPECT_EQ(items[0], "value3");
    EXPECT_TRUE(items[1].empty());
    EXPECT_EQ(items[2], "value0");
    EXPECT_TRUE(items[3].empty());
    EXPECT_EQ(items[4], "value1");
    EXPECT_TRUE(items[5].empty());
    EXPECT_EQ(items[6], "value2");
}

TEST(String, SplitEdgeCases)
{
    std::vector<std::string> items;
    const auto inserter = [&items](const std::string_view item_) { items.emplace_back(item_.data(), item_.size()); };

    const std::string_view empty{};
    const auto retval0 = nstl::split_view_func(empty, ',', inserter, true);
    EXPECT_EQ(retval0, 0U);
    EXPECT_TRUE(items.empty());

    const auto retval1 = nstl::split_view_func(empty, ',', inserter, false);
    EXPECT_EQ(retval1, 1U);
    ASSERT_EQ(items.size(), 1U);
    EXPECT_TRUE(items.front().empty());

    items.clear();
    const std::string_view no_delimiter{ "single-token" };
    const auto retval2 = nstl::split_view_func(no_delimiter, ',', inserter, true);
    EXPECT_EQ(retval2, 1U);
    ASSERT_EQ(items.size(), 1U);
    EXPECT_EQ(items.front(), no_delimiter);
}

TEST(String, SplitReturnValueMatchesCallbackCount)
{
    const std::string_view source{ "a,,b,,c" };
    size_t called = 0;
    const auto counter = [&called](std::string_view) { ++called; };

    const auto retval = nstl::split_view_func(source, ',', counter, true);
    EXPECT_EQ(retval, called);
    EXPECT_EQ(retval, 3U);
}

TEST(String, SplitWide)
{
    const std::wstring_view source{ L"value0,value1" };
    std::vector<std::wstring> items;
    const auto inserter = [&items](const std::wstring_view item_) { items.emplace_back(item_.data(), item_.size()); };

    const auto retval = nstl::split_view_func(source, L',', inserter, true);
    EXPECT_EQ(retval, 2U);
    ASSERT_EQ(items.size(), 2U);
    EXPECT_EQ(items[0], L"value0");
    EXPECT_EQ(items[1], L"value1");
}
