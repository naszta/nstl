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
