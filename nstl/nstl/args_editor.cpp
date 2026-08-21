#include "args_editor.hpp"
#include "exception.hpp"

#include <algorithm>
#include <cstring>

namespace nstl
{
bool is_arg_set(int& argc_, const char** argv_, const char* value_)
{
    NSTL2_THROW_EXCEPTION_IF(argc_ < 0, "argc_ is negative");
    NSTL2_THROW_EXCEPTION_IF(!argv_, "argv_ is nullptr");
    NSTL2_THROW_EXCEPTION_IF(!value_ || value_[0] == '\0', "value_ is nullptr or empty");

    bool found = false;

    for (int idx = 0; idx < argc_; ++idx)
    {
        auto ptr = argv_ + idx;
        const char* ptrval = ptr ? *ptr : nullptr;

        if (ptrval && std::strcmp(value_, ptrval) == 0)
        {
            found = true;
            const auto last_idx = argc_ - 1;
            if (idx < last_idx)
            {
                const auto move_size = last_idx - idx;
                std::memmove(argv_ + idx, argv_ + idx + 1, move_size * sizeof(const char*));
                argv_[last_idx] = ptrval;
            }
            --argc_;
        }
    }

    return found;
}
}
