#ifndef _NSTL_SAFE_BASENAME
#define _NSTL_SAFE_BASENAME 1

#include <string_view>

namespace nstl
{
#ifdef _WIN32
constexpr char path_sep = '\\';
#else
constexpr char path_sep = '/';
#endif

template <class CharT, class Traits = std::char_traits<CharT>>
constexpr std::basic_string_view<CharT, Traits>
safe_basename_view_t(const std::basic_string_view<CharT, Traits> filename_)
{
    const auto pos = filename_.rfind(static_cast<CharT>(path_sep));
    return pos == std::basic_string_view<CharT, Traits>::npos ? filename_ : filename_.substr(pos + 1);
}

constexpr std::string_view safe_basename_view(const std::string_view filename_) { return safe_basename_view_t(filename_); }
constexpr std::wstring_view safe_basename_view(const std::wstring_view filename_)
{
    return safe_basename_view_t(filename_);
}
} // namespace nstl

#endif
