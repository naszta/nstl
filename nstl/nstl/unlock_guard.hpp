#ifndef _NSTL_UNLOCK_GUARD
#define _NSTL_UNLOCK_GUARD 1

#include <mutex>
#include <tuple>

namespace nstl
{
template <class... _Mutexes>
class unlock_guard
{
    std::tuple<_Mutexes&...> _locks;

public:
    explicit unlock_guard(_Mutexes&... locks_)
        : _locks{locks_...}
    {
        std::apply([](_Mutexes&... locks_) { (..., (void) locks_.unlock()); }, _locks);
    }

    explicit unlock_guard(std::defer_lock_t, _Mutexes&... locks_)
        : _locks{locks_...}
    {}

    ~unlock_guard()
    {
        std::apply([](_Mutexes&... locks_) { std::lock(locks_...); }, _locks);
    }

    unlock_guard(const unlock_guard&) = delete;
    unlock_guard& operator= (const unlock_guard&) = delete;
};

template <class BasicLockableT>
class unlock_guard<BasicLockableT>
{
    BasicLockableT& _lock;
public:
    explicit unlock_guard(BasicLockableT& lock_)
        : _lock{lock_}
    {
        _lock.unlock();
    }

    explicit unlock_guard(std::defer_lock_t, BasicLockableT& lock_)
        : _lock{lock_}
    {}

    ~unlock_guard()
    {
        _lock.lock();
    }
    unlock_guard(const unlock_guard&) = delete;
    unlock_guard& operator= (const unlock_guard&) = delete;
};

template <>
class unlock_guard<> {
public:
    explicit unlock_guard() = default;
    explicit unlock_guard(std::defer_lock_t) noexcept {}
    ~unlock_guard() = default;

    unlock_guard(const unlock_guard&)            = delete;
    unlock_guard& operator=(const unlock_guard&) = delete;
};
}

#endif
