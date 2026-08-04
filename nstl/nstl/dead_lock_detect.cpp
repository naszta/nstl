#include "dead_lock_detect.hpp"
#include "exception.hpp"
#include "logging.hpp"

namespace nstl
{
DeadLockThreadExecutor::DeadLockThreadExecutor() : _tp{ time_type::now() } {}
DeadLockThreadExecutor::DeadLockThreadExecutor(time_point tp_) : _tp{ tp_ } {}
DeadLockThreadExecutor::~DeadLockThreadExecutor() = default;

void DeadLockThreadExecutor::bump()
{
    std::lock_guard lg{ _lock };
    _tp = time_type::now();
}

std::chrono::nanoseconds DeadLockThreadExecutor::elapsed() const
{
    std::lock_guard lg{ _lock };
    return time_type::now() - _tp;
}

bool DeadLockChecker::_check()
{
    bool retval = false;
    for (auto itr = _items.begin(); itr != _items.end();)
    {
        if (const auto ptr = itr->lock())
        {
            ++itr;
            if (_timeout < ptr->elapsed())
            {
                NSTL_ERROR("Dead lock detected after " << _timeout.count() << " ns");
                _alerter();
                retval = true;
            }
        }
        else
        {
            itr = _items.erase(itr);
        }
    }
    return retval;
}

DeadLockChecker::DeadLockChecker(std::chrono::nanoseconds timeout_, std::function<void()> alerter_)
    : _timeout{ timeout_ }, _alerter{ std::move(alerter_) }
{
    NSTL2_THROW_EXCEPTION_IF(_timeout <= std::chrono::nanoseconds::zero(), "timeout must be greater than zero");
    NSTL2_THROW_EXCEPTION_IF(!_alerter, "alerter function is empty");
}

DeadLockChecker::~DeadLockChecker() { this->stop(); }

bool DeadLockChecker::check()
{
    std::lock_guard lg{ _lock };
    return this->_check();
}

std::shared_ptr<DeadLockThreadExecutor> DeadLockChecker::addCheckedThread()
{
    std::lock_guard lg{ _lock };
    auto retval = std::make_shared<DeadLockThreadExecutor>();
    _items.emplace_back(retval);
    return retval;
}

void DeadLockChecker::runner(const std::chrono::nanoseconds period_)
{
    std::unique_lock ul{ _lock };
    _running = true;
    while (_running)
    {
        _cond.wait_for(ul, period_);
        _check();
    }
}

void DeadLockChecker::stop()
{
    bool running = false;
    std::unique_lock ul{ _lock };
    std::swap(_running, running);
    if (running)
    {
        _cond.notify_all();
    }
}

bool DeadLockChecker::empty() const
{
    std::lock_guard lg{ _lock };
    return _items.empty();
}
} // namespace nstl
