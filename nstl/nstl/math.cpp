#include "math.hpp"

#include <nstl/compiler.hpp>
#include <cfenv>

NSTL_FENV_ACCESS_ON

namespace nstl::math
{
template <class Type>
    requires(floting_point<Type>)
long long safe_llround_t(Type val_)
{
    if (math_errhandling & MATH_ERREXCEPT)
    {
        std::feclearexcept(FE_INVALID);
        const auto value = std::llround(val_);
        NSTL_THROW_EXCEPTION_IF(std::fetestexcept(FE_INVALID), std::runtime_error,
                            val_ << " cannot be converted to integer");
        return value;
    }
    else if (std::isfinite(val_))
    {
        return std::llround(val_);
    }
    NSTL_THROW_EXCEPTION(std::runtime_error, val_ << " cannot be converted to integer");
}

long long safe_llround(float val_) { return safe_llround_t(val_); }
long long safe_llround(double val_) { return safe_llround_t(val_); }
long long safe_llround(long double val_) { return safe_llround_t(val_); }
} // namespace nstl::math
