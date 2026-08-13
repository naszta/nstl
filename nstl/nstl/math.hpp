#ifndef _NSTL_MATH
#define _NSTL_MATH 1

#include <nstl/macros.hpp>

#include <cmath>
#include <concepts>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace nstl::math
{
template <class Type>
concept floting_point = std::is_floating_point_v<Type>;

template <class Type>
concept integral = std::is_integral_v<Type>;

template <bool nan_eq> struct float_eq
{
    template <class Type>
        requires(floting_point<Type>)
    constexpr Type default_eps() const
    {
        return static_cast<Type>(1e-8);
    }

    template <class LType, class RType>
        requires(floting_point<LType> && floting_point<RType>)
    bool operator()(LType left_, RType right_) const
    {
        using CType = typename std::common_type_t<LType, RType>;
        return this->eq(left_, right_, default_eps<CType>());
    }

    template <class LType, class RType, class CType = std::common_type_t<LType, RType>>
        requires(floting_point<LType> && floting_point<RType> && floting_point<CType>)
    bool eq(LType left_, RType right_, CType eps_ = 1e-8) const
    {
        const auto lcl = std::fpclassify(left_);
        if (lcl != std::fpclassify(right_))
        {
            return false;
        }

        switch (lcl)
        {
        case FP_INFINITE:
            return std::signbit(left_) == std::signbit(right_);
        case FP_NAN:
            if constexpr (nan_eq)
            {
                return true;
            }
            else
            {
                return false;
            }
        case FP_ZERO:
            return true;
        default:
            return std::fabs(left_ - right_) < eps_;
        }
    }
};

long long safe_llround(float val_);
long long safe_llround(double val_);
long long safe_llround(long double val_);

template <class TgtT, class SrcT, class FuncT>
    requires(floting_point<SrcT> && integral<TgtT> && std::invocable<FuncT, SrcT>)
TgtT round_val_func(SrcT value_, FuncT&& func_)
{
    if constexpr (std::is_unsigned_v<TgtT>)
    {
        const auto value = safe_llround(func_(value_));
        NSTL_THROW_EXCEPTION_IF(value < 0, std::invalid_argument, "target is unsigned: value cannot be negative");
        NSTL_THROW_EXCEPTION_IF(std::numeric_limits<TgtT>::max() < value, std::out_of_range,
                                "resolve value is greater than the result");
        return static_cast<TgtT>(value);
    }
    else
    {
        const auto value = safe_llround(func_(value_));
        NSTL_THROW_EXCEPTION_IF(value < std::numeric_limits<TgtT>::min(), std::out_of_range,
                                "resolve value is less than the result's min");
        NSTL_THROW_EXCEPTION_IF(std::numeric_limits<TgtT>::max() < value, std::out_of_range,
                                "resolve value is greater than the result's max");
        return static_cast<TgtT>(value);
    }
}

template <class TgtT, class SrcT>
    requires(floting_point<SrcT> && integral<TgtT>)
TgtT round_val(SrcT value_)
{
    return round_val_func<TgtT>(value_, [](SrcT val_) { return val_; });
}

template <class TgtT, class SrcT>
    requires(floting_point<SrcT> && integral<TgtT>)
TgtT round_floor(SrcT value_)
{
    return round_val_func<TgtT>(value_, [](SrcT val_) { return val_ < 0.0 ? std::ceil(val_) : std::floor(val_); });
}

template <class TgtT, class SrcT>
    requires(floting_point<SrcT> && integral<TgtT>)
TgtT round_ceil(SrcT value_)
{
    return round_val_func<TgtT>(value_, [](SrcT val_) { return val_ < 0.0 ? std::floor(val_) : std::ceil(val_); });
}
} // namespace nstl::math

#endif
