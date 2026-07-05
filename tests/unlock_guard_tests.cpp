#include "nstl/unlock_guard.hpp"

#include <gtest/gtest.h>

TEST(UnlockGuard, BasicTest)
{
    std::mutex lock;
    std::unique_lock ul{lock};
    {
        EXPECT_TRUE(ul.owns_lock());
        nstl::unlock_guard ulg{ul};
        EXPECT_FALSE(ul.owns_lock());
    }
    EXPECT_TRUE(ul.owns_lock());
}


TEST(UnlockGuard, MultiTest)
{
    std::mutex lock0, lock1;
    std::unique_lock ul0{lock0}, ul1{lock1};
    {
        EXPECT_TRUE(ul0.owns_lock());
        EXPECT_TRUE(ul1.owns_lock());
        nstl::unlock_guard ulg{ul0, ul1};
        EXPECT_FALSE(ul0.owns_lock());
        EXPECT_FALSE(ul1.owns_lock());
    }
    EXPECT_TRUE(ul0.owns_lock());
    EXPECT_TRUE(ul1.owns_lock());
}
