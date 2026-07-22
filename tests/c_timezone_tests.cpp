#include <nstl/c_timezone.hpp>

#include <gtest/gtest.h>

#include <chrono>

TEST(CTimeZone, Basic)
{
	const auto now = std::chrono::system_clock::now();
	const nstl::c_timezone czone;

	const std::chrono::zoned_time zt0{std::chrono::current_zone(), now};
	const std::chrono::zoned_time zt1{&czone, now};

	EXPECT_EQ(zt0.get_local_time(), zt1.get_local_time());

	const std::chrono::zoned_time zt2{ std::chrono::current_zone(), zt0.get_local_time() };
	const std::chrono::zoned_time zt3{ &czone, zt1.get_local_time() };

	EXPECT_EQ(zt2.get_sys_time(), zt3.get_sys_time());
	EXPECT_EQ(zt0.get_sys_time(), zt3.get_sys_time());

    const auto zt0_str = std::format("{0:%F}T{0:%T%z}", zt0);
    const auto zt1_str = std::format("{0:%F}T{0:%T%z}", zt1);

    EXPECT_EQ(zt0_str, zt1_str);
}
