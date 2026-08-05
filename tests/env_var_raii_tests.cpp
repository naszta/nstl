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

TEST(EnvVar, RestoresPreviousValue)
{
    nstl::env_var_raii outer{ "NSTL_TEST_VAR_1", "outer value" };
    ASSERT_EQ(nstl::get_env_var("NSTL_TEST_VAR_1"), "outer value");
    {
        nstl::env_var_raii inner{ "NSTL_TEST_VAR_1", "inner value" };
        EXPECT_EQ(nstl::get_env_var("NSTL_TEST_VAR_1"), "inner value");
    }
    EXPECT_EQ(nstl::get_env_var("NSTL_TEST_VAR_1"), "outer value");
}

TEST(EnvVar, RemovesExistingValue)
{
    nstl::env_var_raii outer{ "NSTL_TEST_VAR_2", "outer value" };
    ASSERT_EQ(nstl::get_env_var("NSTL_TEST_VAR_2"), "outer value");
    {
        nstl::env_var_raii remover{ "NSTL_TEST_VAR_2", nullptr };
        EXPECT_TRUE(nstl::get_env_var("NSTL_TEST_VAR_2").empty());
    }
    EXPECT_EQ(nstl::get_env_var("NSTL_TEST_VAR_2"), "outer value");
}

TEST(EnvVar, NoopWhenRemovingUnsetValue)
{
    ASSERT_TRUE(nstl::get_env_var("NSTL_TEST_VAR_3").empty());
    {
        nstl::env_var_raii tmp{ "NSTL_TEST_VAR_3", nullptr };
        EXPECT_TRUE(nstl::get_env_var("NSTL_TEST_VAR_3").empty());
    }
    EXPECT_TRUE(nstl::get_env_var("NSTL_TEST_VAR_3").empty());
}

TEST(EnvVar, EmptyNameThrows)
{
    EXPECT_THROW(nstl::env_var_raii(std::string{}, "value"), std::exception);
}

TEST(EnvVar, GetEnvVarNullptrThrows)
{
    EXPECT_THROW(nstl::get_env_var(nullptr), std::exception);
}
