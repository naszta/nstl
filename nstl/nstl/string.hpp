#ifndef _NSTL_STRING
#define _NSTL_STRING 1

#include <concepts>
#include <cctype>
#include <cwctype>
#include <string_view>
#include <type_traits>

namespace nstl
{
namespace detail
{
template <class CharT> bool is_space_ch(const CharT ch_)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
    {
        return std::iswspace(static_cast<std::wint_t>(ch_)) != 0;
    }
    else
    {
        return std::isspace(static_cast<std::make_unsigned_t<CharT>>(ch_)) != 0;
    }
}
} // namespace detail

template <class CharT, class TraitsT, class FuncT>
    requires std::invocable<FuncT, std::basic_string_view<CharT, TraitsT>>
size_t split_view_func(const std::basic_string_view<CharT, TraitsT> view_, const CharT delim_, FuncT func,
                       const bool skip_empty_ = true)
{
    using str_view = typename std::basic_string_view<CharT, TraitsT>;
    using size_type = typename str_view::size_type;

    size_type prev = 0;
    size_t retval = 0;

    for (auto pos = view_.find(delim_); pos != str_view::npos; prev = pos + 1, pos = view_.find(delim_, prev))
    {
        const auto item = pos == prev ? str_view{} : str_view{ view_.data() + prev, pos - prev };
        if (skip_empty_ && item.empty())
        {
            continue;
        }
        func(item);
        ++retval;
    }

    const auto item = prev < view_.size() ? view_.substr(prev) : str_view{};
    if (skip_empty_ && item.empty())
    {
        return retval;
    }
    func(item);
    ++retval;
    return retval;
}

template <class CharT, class TraitsT>
std::basic_string_view<CharT, TraitsT> right_trim_view(std::basic_string_view<CharT, TraitsT> view_)
{
    while (!view_.empty() && detail::is_space_ch(view_.back()))
    {
        view_ = view_.substr(0, view_.size() - 1);
    }

    return view_;
}

template <class CharT, class TraitsT>
std::basic_string_view<CharT, TraitsT> left_trim_view(std::basic_string_view<CharT, TraitsT> view_)
{
    while (!view_.empty() && detail::is_space_ch(view_.front()))
    {
        view_ = view_.substr(1);
    }

    return view_;
}

template <class CharT, class TraitsT>
std::basic_string_view<CharT, TraitsT> trim_view(std::basic_string_view<CharT, TraitsT> view_)
{
    return right_trim_view(left_trim_view(view_));
}
} // namespace nstl

#endif
