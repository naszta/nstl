#include <nstl/env_var_raii.hpp>

#include <gtest/gtest.h>

TEST(EnvVar, BasicTest)
{
    const auto before = nstl::get_env_var("NSTL_TEST_VAR_0");
    EXPECT_TRUE(before.empty());
    {
        nstl::env_var_raii tmp{ "NSTL_TEST_VAR_0", "test value" };
        const auto in_block = nstl::get_env_var("NSTL_TEST_VAR_0");
        ASSERT_FALSE(in_block.empty());
        EXPECT_EQ(in_block, "test value");
    }
    const auto after = nstl::get_env_var("NSTL_TEST_VAR_0");
    EXPECT_TRUE(after.empty());
}
