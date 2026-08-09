#ifndef _NSTL_ARGS_EDITOR
#define _NSTL_ARGS_EDITOR

#include <cstring>

namespace nstl
{
    bool is_arg_set(int& argc_, char** argv_, const char* value_)
    {
        bool retval = false;
        for (auto ptr = argv_; ptr < argv_ + argc_; ++ptr)
        {
            char* ptrval = ptr ? *ptr : nullptr;
            if (ptr && std::strcmp(*ptr, value_) == 0)
            {
                retval = true;
                if (const size_t move_size = (argc_ - 1)*sizeof(char *); 0 < move_size)
                {
                    std::memmove(ptr, ptr + 1, move_size);
                }
                --argc_;
                argv_[argc_] = ptrval;
            }
        }
        return retval;
    }
}

#endif
