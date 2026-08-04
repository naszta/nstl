#ifndef _NSTL_SAFE_BASENAME
#define _NSTL_SAFE_BASENAME 1

#include <cstring>
#include <string_view>

namespace nstl
{
#ifdef _WIN32
constexpr char path_sep = '\\';
constexpr wchar_t path_sep_w = L'\\';
#else
constexpr char path_sep = '/';
constexpr wchar_t path_sep_w = L'/';
#endif

constexpr std::string_view safe_basename_view(const std::string_view filename_)
{
    const auto pos = filename_.rfind(path_sep);
    return pos == std::string_view::npos ? filename_ : filename_.substr(pos + 1);
}

constexpr std::wstring_view safe_basename_view(const std::wstring_view filename_)
{
    const auto pos = filename_.rfind(path_sep_w);
    return pos == std::string_view::npos ? filename_ : filename_.substr(pos + 1);
}
} // namespace nstl

#endif
