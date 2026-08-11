#ifndef _NSTL_MATH
#define _NSTL_MATH 1

#include <cmath>
#include <type_traits>

namespace nstl::math
{
template <class Type>
concept floting_point = std::is_floating_point_v<Type>;

template <bool nan_eq>
struct float_eq
{
    template <class LType, class RType>
    requires(floting_point<LType> && floting_point<RType>)
	bool operator()(LType left_, RType right_) const
	{
        return this->eq(left_, right_);
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
}

#endif
