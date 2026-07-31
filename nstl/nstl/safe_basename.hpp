#ifndef _NSTL_SAFE_BASENAME
#define _NSTL_SAFE_BASENAME 1

#include <cstring>
#include <string_view>

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

constexpr std::string_view safe_basename_view(const std::string_view filename_)
{
    const auto pos = filename_.rfind(path_sep);
    return pos == std::string_view::npos ? filename_ : filename_.substr(pos + 1);
}
} // namespace nstl

#endif
