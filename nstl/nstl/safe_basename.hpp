#ifndef __NSTL_SAFE_BASENAME
#define __NSTL_SAFE_BASENAME 1

#include <cstring>

namespace nstl
{
#ifdef _WIN32
constexpr char path_sep = '\\';
#else
constexpr char path_sep = '/';
#endif

// this is actually constexpr with gcc
inline const char* safe_basename(const char* filename_)
{
    if (filename_)
    {
        const char* ptr = std::strrchr(filename_, path_sep);
        return ptr ? ptr + 1 : filename_;
    }
    return nullptr;
}
}

#endif
