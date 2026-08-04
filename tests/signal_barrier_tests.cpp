#include <nstl/signal_barrier.hpp>
#include <nstl/scope_exit.hpp>

#include <gtest/gtest.h>

#include <csignal>
#include <atomic>
#include <future>
#include <thread>

constexpr auto future_wait = std::chrono::seconds{ 10 };

TEST(SignalBarrier, BasicTest)
{
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });

    std::promise<std::optional<int>> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<std::optional<int>> promise_)
                        {
                            nstl::SignalBarrier barrier;
                            is_running.store(true);

                            auto retval = barrier.wait_for(std::chrono::seconds{ 5 });

                            promise_.set_value(std::move(retval));
                        },
                        std::move(barrier_result) };

    while (!is_running.load(std::memory_order::relaxed))
    {
        std::this_thread::yield();
    }

    ASSERT_EQ(std::raise(SIGINT), 0);
    const auto status = result.wait_for(future_wait);
    if (status != std::future_status::ready)
    {
        FAIL() << "barrier.wait_for did not return after " << future_wait;
    }
    const auto value_opt = result.get();
    ASSERT_TRUE(value_opt.has_value());
    EXPECT_EQ(value_opt.value(), SIGINT);
    runner.join();
}

TEST(SignalBarrier, BasicNoSignal)
{
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });

    std::promise<std::optional<int>> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<std::optional<int>> promise_)
                        {
                            nstl::SignalBarrier barrier;
                            is_running.store(true);

                            auto retval = barrier.wait_for(std::chrono::seconds{ 1 });

                            promise_.set_value(std::move(retval));
                        },
                        std::move(barrier_result) };

    while (!is_running.load(std::memory_order::relaxed))
    {
        std::this_thread::yield();
    }

    const auto status = result.wait_for(future_wait);
    if (status != std::future_status::ready)
    {
        FAIL() << "barrier.wait_for did not return after " << future_wait;
    }
    const auto value_opt = result.get();
    EXPECT_FALSE(value_opt.has_value()) << "value is " << value_opt.value();
    runner.join();
}

TEST(SignalBarrier, MassiveTest)
{
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });

    std::promise<int> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<int> promise_)
                        {
                            nstl::SignalBarrier barrier;
                            is_running.store(true);

                            auto retval = barrier.wait();

                            promise_.set_value(retval);
                        },
                        std::move(barrier_result) };

    while (!is_running.load(std::memory_order::relaxed))
    {
        std::this_thread::yield();
    }

    ASSERT_EQ(std::raise(SIGINT), 0);
    const auto status = result.wait_for(future_wait);
    if (status != std::future_status::ready)
    {
        FAIL() << "barrier.wait did not return after " << future_wait;
    }
    const auto value = result.get();
    EXPECT_EQ(value, SIGINT);
    runner.join();
}
