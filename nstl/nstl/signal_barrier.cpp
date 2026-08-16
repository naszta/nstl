#include "signal_barrier.hpp"
#include "exception.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <csignal>
#include <atomic>
#include <thread>

namespace nstl
{
namespace
{
std::atomic_bool in_use{ false };
std::atomic_int signal_received{ 0 };

#ifdef _WIN32
constexpr std::uint32_t running_true = std::numeric_limits<std::uint32_t>::max();
std::atomic_uint32_t app_running{ running_true };

// Windows style signal handling
BOOL WINAPI ConsoleHandler(DWORD signal)
{
    switch (signal)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        app_running.store(signal);
        return TRUE;
    default:
        return FALSE;
    }
}

using sighandler_t = _crt_signal_t;
#elif defined(__APPLE__)
using sighandler_t = sig_t;
#endif
void signal_receiver(int value_) { signal_received.store(value_); }

sighandler_t int_hndlr = SIG_DFL;
sighandler_t int_term = SIG_DFL;
#ifdef SIGQUIT
sighandler_t int_quit = SIG_DFL;
#endif

std::optional<SigVal> load_value()
{
    if (const auto value = signal_received.exchange(0, std::memory_order::relaxed); 0 < value) [[unlikely]]
    {
        return from_signal_conv(value);
    }
#ifdef _WIN32
    if (const auto wvalue = app_running.exchange(running_true); wvalue != running_true) [[unlikely]]
    {
        switch (wvalue)
        {
        case CTRL_C_EVENT:
            return SigVal::SigInt;
        case CTRL_CLOSE_EVENT:
            return SigVal::SigTerm;
        case CTRL_LOGOFF_EVENT:
            return SigVal::SigLogoff;
        case CTRL_SHUTDOWN_EVENT:
            return SigVal::SigShutdown;
        default:
            return std::nullopt;
        }
    }
#endif
    return std::nullopt;
}

} // namespace

SignalBarrier::SignalBarrier(std::chrono::nanoseconds period_) : _period{ period_ }
{
    static_assert(std::atomic_int::is_always_lock_free, "signal_received must be atomic");
    NSTL2_THROW_EXCEPTION_IF(_period <= std::chrono::nanoseconds::zero(), "period time must be greater than 0");
    NSTL2_THROW_EXCEPTION_IF(in_use.exchange(true), "SignalBarrier is already in use");
    int_hndlr = std::signal(SIGINT, signal_receiver);
    int_term = std::signal(SIGTERM, signal_receiver);
#ifdef SIGQUIT
    int_quit = std::signal(SIGQUIT, signal_receiver);
#endif

#ifdef _WIN32
    NSTL2_THROW_EXCEPTION_IF(!::SetConsoleCtrlHandler(ConsoleHandler, TRUE), "SetConsoleCtrlHandler failed");
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

SigVal SignalBarrier::wait()
{
    auto value = load_value();
    while (!value)
    {
        std::this_thread::sleep_for(_period);
        value = load_value();
    }
    return value.value();
}

std::optional<SigVal> SignalBarrier::wait_for(const std::chrono::nanoseconds& to_)
{
    const auto start = std::chrono::steady_clock::now();

    if (auto value = load_value())
    {
        return value.value();
    }

    while (std::chrono::steady_clock::now() - start < to_)
    {
        std::this_thread::sleep_for(_period);
        if (auto value = load_value())
        {
            return value.value();
        }
    }
    return std::nullopt;
}
} // namespace nstl
