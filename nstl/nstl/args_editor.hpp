#ifndef _NSTL_ARGS_EDITOR
#define _NSTL_ARGS_EDITOR

#include <cstring>

namespace nstl
{
inline bool is_arg_set(int& argc_, const char** argv_, const char* value_)
{
    const auto end_ptr = argv_ + argc_;

    bool retval = false;
    for (int idx = 0; idx < argc_; ++idx)
    {
        auto ptr = argv_ + idx;

        const char* ptrval = ptr ? *ptr : nullptr;
        if (ptr && std::strcmp(*ptr, value_) == 0)
        {
            retval = true;
            if (const size_t move_size = end_ptr - (ptr + 1); 0 < move_size)
            {
                std::memmove(ptr, ptr + 1, move_size);
                --argc_;
                argv_[argc_] = ptrval;
            }
            else
            {
                --argc_;
            }
        }
    }
    return retval;
}
} // namespace nstl

#endif
