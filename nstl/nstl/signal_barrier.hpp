#ifndef _NSTL_SIGNAL_BARRIER
#define _NSTL_SIGNAL_BARRIER 1

#include <cstdint>
#include <chrono>
#include <iosfwd>
#include <optional>

namespace nstl
{
enum class SigVal : std::int16_t
{
    Unknown = -1,     // unknown
    SigInt = 2,       // CTRL_C_EVENT on Windows,
    SigTerm = 15,     // CTRL_CLOSE_EVENT on Windows,
    SigQuit = 3,      // quit: no pair on Windows
    SigLogoff = 12,   // CTRL_LOGOFF_EVENT no pair on Linux
    SigShutdown = 13, // CTRL_SHUTDOWN_EVENT no pair on Linux
};

SigVal from_signal_conv(const int value_);
int to_signal_conv(SigVal value_);

std::ostream& operator<<(std::ostream& os_, SigVal sig_);

class SignalBarrier
{
#ifdef __linux__
    int _sfd{-1};
#else
    const std::chrono::nanoseconds _period;
#endif

public:
    SignalBarrier(std::chrono::nanoseconds period_ = std::chrono::milliseconds{10});
    ~SignalBarrier();

    SignalBarrier(const SignalBarrier&) = delete;
    SignalBarrier& operator=(const SignalBarrier&) = delete;

    SigVal wait();
    std::optional<SigVal> wait_for(const std::chrono::nanoseconds& to_);
};
} // namespace nstl

#endif
