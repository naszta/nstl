#include <nstl/scope_exit.hpp>

#include <gtest/gtest.h>

TEST(ScopeExit, Basic)
{
    int value = 1;
    {
        const auto cleaner = nstl::on_scope_exit([&value]() { value = 0; });
        ++value;
    }
    EXPECT_EQ(value, 0);
}

TEST(ScopeExit, Reset)
{
    int value = 1;
    {
        auto cleaner = nstl::on_scope_exit([&value]() { value = 0; });
        ++value;
        cleaner.reset();
    }
    EXPECT_EQ(value, 2);
}

TEST(ScopeExit, DefaultConstructedIsEmpty)
{
    nstl::scope_exit guard;
    EXPECT_TRUE(guard.empty());
    EXPECT_FALSE(static_cast<bool>(guard));
}

TEST(ScopeExit, EmptyAndOperatorBool)
{
    nstl::scope_exit guard{ []() {} };
    EXPECT_FALSE(guard.empty());
    EXPECT_TRUE(static_cast<bool>(guard));

    guard.reset();
    EXPECT_TRUE(guard.empty());
    EXPECT_FALSE(static_cast<bool>(guard));
}

TEST(ScopeExit, Swap)
{
    int value = 0;
    std::function<void()> other{ [&value]() { value = 42; } };
    {
        nstl::scope_exit guard{ [&value]() { value = 1; } };
        guard.swap(other);
        // guard now runs what "other" used to hold; "other" holds guard's original functor.
    }
    EXPECT_EQ(value, 42);
    ASSERT_TRUE(static_cast<bool>(other));
    other();
    EXPECT_EQ(value, 1);
}
