#include <nstl/math.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

TEST(MathFloatEq, OppositeSignInfinityIsFalse)
{
    const nstl::math::float_eq<true> oper;
    EXPECT_FALSE(oper(std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(oper(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()));
}

TEST(MathFloatEq, FiniteVsInfiniteIsFalse)
{
    const nstl::math::float_eq<true> oper;
    EXPECT_FALSE(oper(1.0, std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(oper(std::numeric_limits<double>::infinity(), 1.0));
}

TEST(MathFloatEq, WithinDefaultEpsIsTrue)
{
    const nstl::math::float_eq<true> oper;
    EXPECT_TRUE(oper(1.0, 1.0 + 1e-10));
    EXPECT_FALSE(oper(1.0, 1.0 + 1e-6));
}

TEST(MathFloatEq, CustomEps)
{
    const nstl::math::float_eq<true> oper;
    EXPECT_TRUE(oper.eq(1.0, 1.1, 0.2));
    EXPECT_FALSE(oper.eq(1.0, 1.1, 0.05));
}

TEST(MathSafeLlround, Float)
{
    EXPECT_EQ(nstl::math::safe_llround(1.5f), 2);
    EXPECT_EQ(nstl::math::safe_llround(-1.5f), -2);
    EXPECT_THROW(nstl::math::safe_llround(std::numeric_limits<float>::quiet_NaN()), std::exception);
    EXPECT_THROW(nstl::math::safe_llround(std::numeric_limits<float>::infinity()), std::exception);
}

TEST(MathSafeLlround, Double)
{
    EXPECT_EQ(nstl::math::safe_llround(2.4), 2);
    EXPECT_EQ(nstl::math::safe_llround(-2.4), -2);
    EXPECT_THROW(nstl::math::safe_llround(std::numeric_limits<double>::quiet_NaN()), std::exception);
    EXPECT_THROW(nstl::math::safe_llround(std::numeric_limits<double>::infinity()), std::exception);
}

TEST(MathSafeLlround, LongDouble)
{
    EXPECT_EQ(nstl::math::safe_llround(3.6L), 4);
    EXPECT_THROW(nstl::math::safe_llround(std::numeric_limits<long double>::quiet_NaN()), std::exception);
}

TEST(MathRound, OverflowThrowsOutOfRange)
{
    // values large enough to overflow the 32-bit target but still representable as long long,
    // so this exercises round_val_func's own range check rather than safe_llround's conversion guard.
    EXPECT_THROW(nstl::math::round_val<std::int32_t>(5e9), std::out_of_range);
    EXPECT_THROW(nstl::math::round_val<std::int32_t>(-5e9), std::out_of_range);
    EXPECT_THROW(nstl::math::round_val<std::uint32_t>(5e9), std::out_of_range);
}

TEST(MathRound, UnrepresentableValueThrowsRuntimeError)
{
    EXPECT_THROW(nstl::math::round_val<std::int32_t>(1e300), std::runtime_error);
    EXPECT_THROW(nstl::math::round_val<std::int32_t>(-1e300), std::runtime_error);
}

TEST(MathRound, NegativeToUnsignedThrowsInvalidArgument)
{
    EXPECT_THROW(nstl::math::round_val<std::uint32_t>(-0.6), std::invalid_argument);
}

TEST(MathRound, RoundValFunc)
{
    const auto value = nstl::math::round_val_func<std::int32_t>(3.2, [](double val_) { return val_ * 2.0; });
    EXPECT_EQ(value, 6);
}
