// <span> must be included before nstl/ro_buffer.hpp: that header only defines its span constructor
// when __cpp_lib_span is already visible, and feature-test macros only become defined once a header
// that provides them has actually been included.
#include <span>

#include <nstl/ro_buffer.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <istream>
#include <ostream>
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

TEST(RoBuffer, WriteIsRejected)
{
    const std::string_view data{ "123456" };
    nstl::ro_buffer buffer{ data };
    std::iostream stream{ &buffer };

    stream << "abc";
    EXPECT_TRUE(stream.fail());
    // the underlying buffer must be untouched.
    EXPECT_EQ(buffer.content(), data);
}

TEST(RoBuffer, SyncIsRejected)
{
    const std::string_view data{ "123456" };
    nstl::ro_buffer buffer{ data };
    std::iostream stream{ &buffer };

    EXPECT_EQ(stream.rdbuf()->pubsync(), -1);
}

TEST(RoBuffer, PutbackIsRejected)
{
    const std::string_view data{ "123456" };
    nstl::ro_buffer buffer{ data };
    std::istream stream{ &buffer };

    char first = 0;
    stream.get(first);
    ASSERT_EQ(first, '1');
    stream.unget();
    // a putback position is available (we just read one char), so this first unget() succeeds
    // without ever reaching pbackfail().
    EXPECT_FALSE(stream.fail());

    // back at the very start (gptr == eback): no putback position is available, so this unget()
    // must fall through to pbackfail(), which this read-only buffer rejects.
    stream.unget();
    EXPECT_TRUE(stream.fail());
}

#ifdef __cpp_lib_span
TEST(RoBuffer, SpanConstructor)
{
    std::array<char, 6> raw{ '1', '2', '3', '4', '5', '6' };
    nstl::ro_buffer buffer{ std::span{ raw } };
    EXPECT_EQ(buffer.content(), std::string_view{ "123456" });
}
#endif

TEST(RoBuffer, Swap)
{
    const std::string_view data0{ "first" };
    const std::string_view data1{ "second value" };
    nstl::ro_buffer buffer0{ data0 };
    nstl::ro_buffer buffer1{ data1 };

    buffer0.swap(buffer1);

    EXPECT_EQ(buffer0.content(), data1);
    EXPECT_EQ(buffer1.content(), data0);
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

TEST(RoBuffer, InvalidInput)
{
    EXPECT_THROW(nstl::ro_buffer(nullptr, 42), std::exception);
    EXPECT_THROW(nstl::wro_buffer(nullptr, 42), std::exception);
}
