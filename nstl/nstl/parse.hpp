#ifndef _NSTL_PARSE_H
#define _NSTL_PARSE_H 1

#include <nstl/macros.hpp>

#include <charconv>
#include <type_traits>
#include <stdexcept>
#include <string_view>

namespace nstl
{
template <class Type>
concept arithmetic = std::is_arithmetic_v<Type>;

template <class Type>
    requires(arithmetic<Type>)
Type parse_view(const std::string_view view_)
{
    NSTL_THROW_EXCEPTION_IF(view_.empty(), std::invalid_argument, "view is empty");
    if constexpr (std::is_same_v<bool, Type>)
    {
        if (view_ == "true" || view_ == "1")
        {
            return true;
        }
        else if (view_ == "false" || view_ == "0")
        {
            return false;
        }
        NSTL_THROW_EXCEPTION(std::invalid_argument, "view is not boolean: " << view_);
    }
    else
    {
        Type value{};
        const auto endptr = view_.data() + view_.size();
        const std::from_chars_result res = std::from_chars(view_.data(), endptr, value);

        NSTL_THROW_EXCEPTION_IF(res.ptr != endptr, std::invalid_argument, "Not the whole view is a number: " << view_);
        NSTL_THROW_EXCEPTION_IF(res.ec != std::errc{}, std::invalid_argument, "Failed to parse the view: " << view_);

        return value;
    }
}
} // namespace nstl

#endif
