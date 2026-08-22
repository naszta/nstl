#include "math.hpp"

#include <nstl/compiler.hpp>
#include <cfenv>

NSTL_FENV_ACCESS_ON

#if (math_errhandling & MATH_ERREXCEPT) == 0
#error This code needs to use floating point exceptions
#endif

namespace nstl::math
{
template <class Type>
    requires(floting_point<Type>)
long long safe_llround_t(Type val_)
{
    std::feclearexcept(FE_INVALID);
    const auto value = std::llround(val_);
    NSTL_THROW_EXCEPTION_IF(std::fetestexcept(FE_INVALID), std::runtime_error,
                            val_ << " cannot be converted to integer");
    return value;
}

long long safe_llround(float val_) { return safe_llround_t(val_); }
long long safe_llround(double val_) { return safe_llround_t(val_); }
long long safe_llround(long double val_) { return safe_llround_t(val_); }
} // namespace nstl::math
