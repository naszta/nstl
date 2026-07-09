#include <nstl/scope_exit.hpp>

#include <gtest/gtest.h>

TEST(ScopeExit, Basic)
{
	int value = 1;
	{
		const auto cleaner = nstl::on_scope_exit([&value]() {value = 0; });
		++value;
	}
	EXPECT_EQ(value, 0);
}


TEST(ScopeExit, Reset)
{
	int value = 1;
	{
		auto cleaner = nstl::on_scope_exit([&value]() {value = 0; });
		++value;
		cleaner.reset();
	}
	EXPECT_EQ(value, 2);
}
