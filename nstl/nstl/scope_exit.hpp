#ifndef __NSTL_SCOPE_EXIT
#define __NSTL_SCOPE_EXIT 1

#include <functional>

namespace nstl
{
    class scope_exit
    {
        std::function<void()> _func;

    public:
        scope_exit() = default;
        template <class Func>
        explicit scope_exit(Func&& func_)
            : _func{std::forward<Func>(func_)}
        {}

        scope_exit(const scope_exit&) = delete;
        scope_exit& operator=(const scope_exit&) = delete;

        void reset()
        {
            _func = std::function<void()>{};
        }

        ~scope_exit()
        {
            if (_func)
            {
                _func();
            }
        }
    };


    template <class Func>
    auto on_scope_exit(Func func_) -> ::nstl::scope_exit
    {
        return ::nstl::scope_exit(std::forward<Func>(func_));
    }
}

#endif
