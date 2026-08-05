#include <gtest/gtest.h>

#include <nstl/base64.hpp>

#include <string_view>
#include <vector>

namespace
{
bool test_item_iter(const std::string_view data)
{
    std::vector<char> bval, rval;
    nstl::to_base64(data, std::back_inserter(bval));
    nstl::from_base64(bval, std::back_inserter(rval));
    const auto resval = std::string_view{ rval.data(), rval.size() };
    return data == resval;
}
} // namespace

TEST(Base64, TestsIter)
{
    const std::string_view examples{ "0123456789abcdefghijklmnopqrstuvwxyz" };

    EXPECT_TRUE(test_item_iter(std::string_view{}));
    EXPECT_TRUE(test_item_iter(examples.substr(1)));
    EXPECT_TRUE(test_item_iter(examples.substr(2)));
    EXPECT_TRUE(test_item_iter(examples.substr(3)));
    EXPECT_TRUE(test_item_iter(examples.substr(4)));
    EXPECT_TRUE(test_item_iter(examples.substr(5)));
    EXPECT_TRUE(test_item_iter(examples.substr(6)));
    EXPECT_TRUE(test_item_iter(examples.substr(7)));
    EXPECT_TRUE(test_item_iter(examples.substr(8)));
    EXPECT_TRUE(test_item_iter(examples.substr(9)));
    EXPECT_TRUE(test_item_iter(examples.substr(10)));
}
