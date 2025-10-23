#ifndef __NSTL_UNLOCK_GUARD
#define __NSTL_UNLOCK_GUARD 1

#include <mutex>

namespace nstl
{
    template <class BasicLockableT>
    class unlock_guard
    {
        BasicLockableT& _lock;
    public:
        explicit unlock_guard(BasicLockableT& lock_)
            : _lock{lock_}
        {
            _lock.unlock();
        }
        ~unlock_guard()
        {
            _lock.lock();
        }
        unlock_guard(const unlock_guard&) = delete;
        unlock_guard& operator= (const unlock_guard&) = delete;
    };
}

#endif
