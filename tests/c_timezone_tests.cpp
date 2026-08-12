#include <nstl/c_timezone.hpp>
#include <nstl/compiler.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

#ifdef NSTL_USING_HH_DATE
NSTL_WRN_DATE_PUSH
#include <date/date.h>
#include <date/tz.h>
NSTL_WRN_DATE_POP
#else
namespace date = std::chrono;
#endif

namespace fs = std::filesystem;

namespace
{
fs::path test_data_path()
{
    const fs::path src_file{ __FILE__ };
    return src_file.parent_path() / "data" / "tzdata";
}
} // namespace

TEST(CTimeZone, Basic)
{
    const auto path = test_data_path();
#if defined(NSTL_USING_HH_DATE) && defined(_WIN32)
    date::set_install(path.string());
#endif
    EXPECT_TRUE(fs::exists(path));

    const auto now = std::chrono::system_clock::now();
    const nstl::c_timezone czone;

    const date::zoned_time zt0{ date::current_zone(), now };
    const date::zoned_time zt1{ &czone, now };

    EXPECT_EQ(zt0.get_local_time(), zt1.get_local_time());

    const date::zoned_time zt2{ date::current_zone(), zt0.get_local_time() };
    const date::zoned_time zt3{ &czone, zt1.get_local_time() };

    EXPECT_EQ(zt2.get_sys_time(), zt3.get_sys_time());
    EXPECT_EQ(zt0.get_sys_time(), zt3.get_sys_time());

#ifdef NSTL_USING_HH_DATE
    const auto zt0_str = date::format("%FT%T%z", zt0);
    const auto zt1_str = date::format("%FT%T%z", zt1);
#else
    const auto zt0_str = std::format("{:%FT%T%z}", zt0);
    const auto zt1_str = std::format("{:%FT%T%z}", zt1);
#endif

    EXPECT_EQ(zt0_str, zt1_str);
}

TEST(CTimeZone, Name)
{
    const nstl::c_timezone default_zone;
    EXPECT_EQ(default_zone.name(), "current");

    const nstl::c_timezone named_zone{ "Europe/Budapest" };
    EXPECT_EQ(named_zone.name(), "Europe/Budapest");
}

TEST(CTimeZone, EqualityAndOrdering)
{
    const nstl::c_timezone alpha{ "Alpha" };
    const nstl::c_timezone alpha2{ "Alpha" };
    const nstl::c_timezone beta{ "Beta" };

    EXPECT_TRUE(alpha == alpha2);
    EXPECT_FALSE(alpha == beta);
    EXPECT_TRUE(alpha < beta);
    EXPECT_TRUE(beta > alpha);
    EXPECT_EQ(alpha <=> alpha2, std::strong_ordering::equal);
}

TEST(CTimeZone, GetInfoSys)
{
    const nstl::c_timezone czone;
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const auto info = czone.get_info(now);

    const auto expected = date::current_zone()->get_info(now);
    EXPECT_EQ(info.offset, expected.offset);
    EXPECT_EQ(info.begin, now);
    EXPECT_EQ(info.end, now + std::chrono::seconds{ 1 });
}

TEST(CTimeZone, GetInfoLocal)
{
    const nstl::c_timezone czone;
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const date::zoned_time zt{ date::current_zone(), now };
    const auto local_now = zt.get_local_time();

    const auto info = czone.get_info(local_now);
    EXPECT_EQ(info.result, date::local_info::unique);
    EXPECT_EQ(info.first.offset, date::current_zone()->get_info(now).offset);
}
