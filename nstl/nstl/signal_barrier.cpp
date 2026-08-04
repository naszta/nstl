#include "signal_barrier.hpp"
#include "exception.hpp"

#include <csignal>
#include <atomic>
#include <thread>

namespace nstl
{
namespace
{
#ifdef _WIN32
using sighandler_t = _crt_signal_t;
#endif
std::atomic_bool in_use{ false };
std::atomic_int signal_received{ 0 };
void signal_receiver(int value_) { signal_received.store(value_); }

sighandler_t int_hndlr = SIG_DFL;
sighandler_t int_term = SIG_DFL;
#ifdef SIGQUIT
sighandler_t int_quit = SIG_DFL;
#endif
} // namespace

SignalBarrier::SignalBarrier()
{
    static_assert(std::atomic_int::is_always_lock_free, "signal_received must be atomic");
    NSTL2_THROW_EXCEPTION_IF(in_use.exchange(true), "SignalBarrier is already in use");
    int_hndlr = std::signal(SIGINT, signal_receiver);
    int_term = std::signal(SIGTERM, signal_receiver);
#ifdef SIGQUIT
    int_quit = std::signal(SIGQUIT, signal_receiver);
#endif
}

SignalBarrier::~SignalBarrier()
{
    int_hndlr = std::signal(SIGINT, int_hndlr);
    int_term = std::signal(SIGTERM, int_term);
#ifdef SIGQUIT
    int_quit = std::signal(SIGQUIT, int_quit);
#endif
    in_use.store(false);
}

int SignalBarrier::wait()
{
    int value = signal_received.exchange(0, std::memory_order::relaxed);
    while (value == 0)
    {
        std::this_thread::yield();
        value = signal_received.exchange(0, std::memory_order::relaxed);
    }
    return value;
}

std::optional<int> SignalBarrier::wait_for(const std::chrono::nanoseconds& to_)
{
    const auto start = std::chrono::steady_clock::now();

    if (auto value = signal_received.exchange(0, std::memory_order::relaxed); value != 0)
    {
        return value;
    }

    while (std::chrono::steady_clock::now() - start < to_)
    {
        std::this_thread::yield();
        if (auto value = signal_received.exchange(0, std::memory_order::relaxed); value != 0)
        {
            return value;
        }
    }
    return std::nullopt;
}
} // namespace nstl