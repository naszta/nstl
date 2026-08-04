#include <nstl/ro_buffer.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <istream>
#include <string_view>

#ifdef NSTL_USING_HH_DATE
#include <date/date.h>
#include <date/tz.h>
#else
namespace date = std::chrono;
#endif

TEST(RoBuffer, IntParser)
{
    const std::string_view data{ "123456 other" };
    {
        nstl::ro_buffer buffer{ data };
        std::istream stream{ &buffer };
        int value = 0;
        stream >> value;
        EXPECT_FALSE(stream.fail());
        EXPECT_FALSE(stream.eof());
        EXPECT_EQ(value, 123456);
    }
    {
        nstl::ro_buffer buffer{ data.substr(1, 3) };
        std::istream stream{ &buffer };
        int value = 0;
        stream >> value;
        EXPECT_FALSE(stream.fail());
        EXPECT_TRUE(stream.eof());
        EXPECT_EQ(value, 234);
    }
}

TEST(RoBuffer, Content)
{
    const std::string_view data{ "123456 other" };
    nstl::ro_buffer buffer{ data };
    EXPECT_EQ(buffer.content(), data);

    std::istream stream{ &buffer };
    int value = 0;
    stream >> value;
    EXPECT_EQ(value, 123456);
    // content() must still cover the whole underlying buffer after reads,
    // not just the unread remainder (that's readBytes()'s job).
    EXPECT_EQ(buffer.content(), data);
    EXPECT_EQ(buffer.readBytes(), 6u);
}

TEST(RoBuffer, WideContent)
{
    const std::wstring_view data{ L"123456 other" };
    nstl::wro_buffer buffer{ data };
    std::wistream stream{ &buffer };
    int value = 0;
    stream >> value;
    EXPECT_FALSE(stream.fail());
    EXPECT_EQ(value, 123456);
    EXPECT_EQ(buffer.content(), data);
}

TEST(RoBuffer, TimeStamp)
{
    const std::string_view stamp_view{ "2026-08-04T15:02:00.125Z" };
    nstl::ro_buffer buffer{ stamp_view };
    std::istream stream{ &buffer };
    date::sys_time<std::chrono::milliseconds> target;
    if (stream >> date::parse("%FT%TZ", target))
    {
        const auto epoch_millis = target.time_since_epoch().count();
        EXPECT_EQ(epoch_millis, 1785855720125LL);
    }
    else
    {
        ADD_FAILURE() << stamp_view << " cannot be parsed";
    }
}