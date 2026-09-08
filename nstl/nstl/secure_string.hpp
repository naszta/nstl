#ifndef _NSTL_SECURE_STRING
#define _NSTL_SECURE_STRING 1

#include <cstring>

namespace nstl
{
inline size_t strlen(const char* ptr_)
{
    if (ptr_) [[likely]]
    {
        return ::std::strlen(ptr_);
    }

    return 0;
}

inline int strcmp(const char* left_, const char* right_)
{
    if (left_ && right_) [[likely]]
    {
        return ::std::strcmp(left_, right_);
    }
    if (left_)
    {
        return -1;
    }
    if (right_)
    {
        return 1;
    }
    return 0;
}

inline int strncmp(const char* left_, const char* right_, const size_t max_count_)
{
    if (left_ && right_) [[likely]]
    {
        return ::std::strncmp(left_, right_, max_count_);
    }
    if (left_)
    {
        return -1;
    }
    if (right_)
    {
        return 1;
    }
    return 0;
}

inline size_t wcslen(const wchar_t* text_)
{
    if (text_) [[likely]]
    {
        return std::wcslen(text_);
    }
    return 0;
}

inline void secure_zero(void* ptr_, const size_t size_)
{
    if (ptr_ == nullptr || size_ == 0) [[unlikely]]
    {
        return;
    }
    std::memset(ptr_, 0, size_);
}
} // namespace nstl

#endif
