#include <nstl/dead_lock_detect.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

TEST(DeadLock, BasicTest)
{
    std::atomic_uint32_t called{ 0 };
    nstl::DeadLockChecker checker{ std::chrono::milliseconds{ 10 }, [&called]() { ++called; } };

    auto runner = checker.addCheckedThread();
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
    EXPECT_TRUE(checker.check());
    runner.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
    EXPECT_FALSE(checker.check());
    EXPECT_EQ(called, 1U);
}

TEST(DeadLock, Threaded)
{
    std::atomic_uint32_t called{ 0 };
    const auto checker =
        std::make_shared<nstl::DeadLockChecker>(std::chrono::milliseconds{ 10 }, [&called]() { ++called; });
    std::thread runner{ [checker]() { checker->runner(std::chrono::milliseconds{ 50 }); } };

    auto current = checker->addCheckedThread();
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
    current.reset();

    checker->stop();
    runner.join();
    EXPECT_GE(called, 1U);
}
