#include "nstl/ro_buffer.hpp"

#include <gtest/gtest.h>

#include <istream>
#include <string_view>

TEST(RoBuffer, IntParser)
{
    const std::string_view data{"123456 other"};
    {
        nstl::ro_buffer buffer{data};
        std::istream stream{&buffer};
        int value = 0;
        stream >> value;
        EXPECT_FALSE(stream.fail());
        EXPECT_FALSE(stream.eof());
        EXPECT_EQ(value, 123456);
    }
    {
        nstl::ro_buffer buffer{data.substr(1, 3)};
        std::istream stream{&buffer};
        int value = 0;
        stream >> value;
        EXPECT_FALSE(stream.fail());
        EXPECT_TRUE(stream.eof());
        EXPECT_EQ(value, 234);
    }
}
