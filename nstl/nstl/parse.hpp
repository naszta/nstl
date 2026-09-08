#ifndef _NSTL_PARSE_H
#define _NSTL_PARSE_H 1

#include <nstl/macros.hpp>

#include <algorithm>
#include <array>
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
auto from_chars(const char* beg_, const char* end_, Type& tgt_) -> std::from_chars_result
{
    if constexpr (std::is_same_v<bool, Type>)
    {
        const auto size = end_ - beg_;
        if (size < 0) [[unlikely]]
        {
            throw std::invalid_argument{ "end_ is less than beg_" };
        }

        constexpr const std::array<std::string_view, 2> true_values{ "1", "true" };
        constexpr const std::array<std::string_view, 2> false_values{ "0", "false" };
        const std::string_view value{ beg_, static_cast<size_t>(size) };

        if (const auto titr = std::ranges::find_if(true_values, [&value](const std::string_view item)
                                                   { return value.starts_with(item); });
            titr != true_values.end())
        {
            tgt_ = true;
            return std::from_chars_result{ .ptr = beg_ + titr->size(), .ec = std::errc{} };
        }
        if (const auto fitr = std::ranges::find_if(false_values, [&value](const std::string_view item)
                                                   { return value.starts_with(item); });
            fitr != false_values.end())
        {
            tgt_ = false;
            return std::from_chars_result{ .ptr = beg_ + fitr->size(), .ec = std::errc{} };
        }
        return std::from_chars_result{ .ptr = beg_, .ec = std::errc::invalid_argument };
    }
    else
    {
        return std::from_chars(beg_, end_, tgt_);
    }
}

template <class Type>
    requires(arithmetic<Type>)
Type parse_view(const std::string_view view_)
{
    NSTL_THROW_EXCEPTION_IF(view_.empty(), std::invalid_argument, "view is empty");
    Type value{};
    const auto endptr = view_.data() + view_.size();
    const std::from_chars_result res = nstl::from_chars(view_.data(), endptr, value);

    NSTL_THROW_EXCEPTION_IF(res.ptr != endptr, std::invalid_argument, "Not the whole view is a number: " << view_);
    NSTL_THROW_EXCEPTION_IF(res.ec != std::errc{}, std::invalid_argument, "Failed to parse the view: " << view_);

    return value;
}
} // namespace nstl

#endif
