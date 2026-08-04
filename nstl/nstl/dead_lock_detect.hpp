#ifndef _NSTL_DEAD_LOCK_DETECT
#define _NSTL_DEAD_LOCK_DETECT 1

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace nstl
{
using time_type = std::chrono::steady_clock;
using time_point = time_type::time_point;

class PerfCheck
{
    time_point _tp;

public:
    PerfCheck() : _tp{ time_type::now() } {}
    explicit PerfCheck(time_point tp_) : _tp{ tp_ } {}

    template <class Duration = std::chrono::nanoseconds> Duration snap() const
    {
        return std::chrono::duration_cast<Duration>(time_type::now() - _tp);
    }
};

class DeadLockThreadExecutor : public std::enable_shared_from_this<DeadLockThreadExecutor>
{
    mutable std::mutex _lock;
    time_point _tp;

public:
    DeadLockThreadExecutor();
    explicit DeadLockThreadExecutor(time_point tp_);
    ~DeadLockThreadExecutor();

    DeadLockThreadExecutor(const DeadLockThreadExecutor&) = delete;
    DeadLockThreadExecutor& operator=(const DeadLockThreadExecutor&) = delete;

    void bump();
    void operator()() { this->bump(); }

    std::chrono::nanoseconds elapsed() const;
};

class DeadLockChecker
{
    const std::chrono::nanoseconds _timeout;
    const std::function<void()> _alerter;
    std::vector<std::weak_ptr<const DeadLockThreadExecutor>> _items;
    mutable std::mutex _lock;
    std::condition_variable _cond;
    bool _running{ false };

    bool _check();

public:
    DeadLockChecker(std::chrono::nanoseconds timeout_, std::function<void()> alerter_);
    template <class RepT, class RatioT>
    DeadLockChecker(std::chrono::duration<RepT, RatioT> timeout_, std::function<void()> alerter_)
        : DeadLockChecker{ std::chrono::duration_cast<std::chrono::nanoseconds>(timeout_), std::move(alerter_) }
    {
    }
    ~DeadLockChecker();
    DeadLockChecker(const DeadLockChecker&) = delete;
    DeadLockChecker& operator=(const DeadLockChecker&) = delete;

    bool check();
    std::shared_ptr<DeadLockThreadExecutor> addCheckedThread();

    void runner(std::chrono::nanoseconds period_);
    template <class RepT, class RatioT> void runner(std::chrono::duration<RepT, RatioT> period_)
    {
        this->runner(std::chrono::duration_cast<std::chrono::nanoseconds>(period_));
    }
    void stop();
    bool empty() const;
};

} // namespace nstl

#endif
