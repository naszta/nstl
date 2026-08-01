#include <nstl/c_timezone.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

#ifdef NSTL_USING_HH_DATE
#include <date/date.h>
#include <date/tz.h>
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
    const auto zt0_str = std::format("{0:%F}T{0:%T%z}", zt0);
    const auto zt1_str = std::format("{0:%F}T{0:%T%z}", zt1);
#endif

    EXPECT_EQ(zt0_str, zt1_str);
}
