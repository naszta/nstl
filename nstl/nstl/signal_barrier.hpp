#ifndef _NSTL_SIGNAL_BARRIER
#define _NSTL_SIGNAL_BARRIER 1

#include <chrono>
#include <optional>

namespace nstl
{
class SignalBarrier
{
    const std::chrono::nanoseconds _period;

public:
    SignalBarrier(std::chrono::nanoseconds period_ = std::chrono::milliseconds{10});
    ~SignalBarrier();

    SignalBarrier(const SignalBarrier&) = delete;
    SignalBarrier& operator=(const SignalBarrier&) = delete;

    int wait();
    std::optional<int> wait_for(const std::chrono::nanoseconds& to_);
};
} // namespace nstl

#endif
