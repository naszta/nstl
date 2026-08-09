#include <nstl/signal_barrier.hpp>
#include <nstl/scope_exit.hpp>
#include <nstl/logging.hpp>

#include <gtest/gtest.h>

#include <csignal>
#include <atomic>
#include <future>
#include <thread>

constexpr auto future_wait = std::chrono::seconds{ 300 };

#ifdef __linux__
#define RAISE_SIGNAL(signal) EXPECT_EQ(::kill(::getpid(), signal), 0)
#else
#define RAISE_SIGNAL(signal) EXPECT_EQ(std::raise(signal), 0)
#endif

TEST(SignalBarrier, BasicTest)
{
#ifndef __linux__
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });
#endif

    std::promise<std::optional<nstl::SigVal>> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<std::optional<nstl::SigVal>> promise_)
                        {
                            nstl::SignalBarrier barrier;
                            is_running.store(true);

                            auto retval = barrier.wait_for(std::chrono::seconds{ 10 });

                            promise_.set_value(std::move(retval));
                        },
                        std::move(barrier_result) };

    while (!is_running.load(std::memory_order::relaxed))
    {
        std::this_thread::yield();
    }

    RAISE_SIGNAL(SIGINT);
    const auto status = result.wait_for(future_wait);
    if (status != std::future_status::ready)
    {
        FAIL() << "barrier.wait_for did not return after " << future_wait;
    }
    const auto value_opt = result.get();
    EXPECT_TRUE(value_opt.has_value());
    EXPECT_EQ(value_opt.value_or(nstl::SigVal::Unknown), nstl::SigVal::SigInt);
    runner.join();
}

TEST(SignalBarrier, BasicNoSignal)
{
#ifndef __linux__
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });
#endif

    std::promise<std::optional<nstl::SigVal>> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<std::optional<nstl::SigVal>> promise_)
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
#ifndef __linux__
    const auto prev_hndlr = std::signal(SIGINT, SIG_IGN);
    const auto cleanup = nstl::on_scope_exit([&prev_hndlr]() { std::signal(SIGINT, prev_hndlr); });
#endif

    std::promise<nstl::SigVal> barrier_result;
    std::atomic_bool is_running{ false };

    auto result = barrier_result.get_future();

    std::thread runner{ [&is_running](std::promise<nstl::SigVal> promise_)
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

    RAISE_SIGNAL(SIGINT);
    const auto status = result.wait_for(future_wait);
    if (status != std::future_status::ready)
    {
        FAIL() << "barrier.wait did not return after " << future_wait;
    }
    const auto value = result.get();
    EXPECT_EQ(value, nstl::SigVal::SigInt);
    runner.join();
}
