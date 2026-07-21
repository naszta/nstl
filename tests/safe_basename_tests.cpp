#include <nstl/safe_basename.hpp>

#include <gtest/gtest.h>

#include <cstring>

TEST(SafeBaseName, Tests)
{
	EXPECT_EQ(std::strcmp(::nstl::safe_basename(__FILE__), "safe_basename_tests.cpp"), 0);
	EXPECT_EQ(std::strcmp(::nstl::safe_basename("safe_basename_tests.cpp"), "safe_basename_tests.cpp"), 0);
	EXPECT_EQ(::nstl::safe_basename(nullptr), nullptr);
}
