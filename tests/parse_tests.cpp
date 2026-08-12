#include <gtest/gtest.h>

#include <nstl/math.hpp>
#include <nstl/parse.hpp>

#include <cstdint>
#include <cmath>
#include <limits>

TEST(ParseView, Bool)
{
    EXPECT_TRUE(nstl::parse_view<bool>("true"));
    EXPECT_TRUE(nstl::parse_view<bool>("1"));
    EXPECT_FALSE(nstl::parse_view<bool>("false"));
    EXPECT_FALSE(nstl::parse_view<bool>("0"));
    EXPECT_THROW(nstl::parse_view<bool>(""), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<bool>("true_false"), std::invalid_argument);
}

using SignedIntegers = ::testing::Types<std::int16_t, std::int32_t, std::int64_t>;
using UnsignedIntegers = ::testing::Types<std::uint16_t, std::uint32_t, std::uint64_t>;
#ifdef __APPLE__
using FloatingPoints = ::testing::Types<float, double>;
#else
using FloatingPoints = ::testing::Types<float, double, long double>;
#endif

template <typename Type> struct IntTests : public ::testing::Test
{
    using value_type = Type;
};

template <typename Type> struct UnsignedIntTests : public ::testing::Test
{
    using value_type = Type;
};

template <typename Type> struct FloatTests : public ::testing::Test
{
    using value_type = Type;
};

TYPED_TEST_SUITE(IntTests, SignedIntegers);
TYPED_TEST_SUITE(UnsignedIntTests, UnsignedIntegers);
TYPED_TEST_SUITE(FloatTests, FloatingPoints);

TYPED_TEST(IntTests, ParseTest)
{
    using value_type = typename TestFixture::value_type;

    EXPECT_EQ(nstl::parse_view<value_type>("12"), 12);
    EXPECT_EQ(nstl::parse_view<value_type>("-12"), -12);
    EXPECT_THROW(nstl::parse_view<value_type>("2rule"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>(""), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("true_false"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("123456789012345678901234567890"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("12.3"), std::invalid_argument);
}

TYPED_TEST(UnsignedIntTests, ParseTest)
{
    using value_type = typename TestFixture::value_type;

    EXPECT_EQ(nstl::parse_view<value_type>("12"), 12);
    EXPECT_THROW(nstl::parse_view<value_type>("-12"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("2rule"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>(""), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("true_false"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("123456789012345678901234567890"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("12.3"), std::invalid_argument);
}

TYPED_TEST(FloatTests, ParseTest)
{
    using value_type = typename TestFixture::value_type;

    const nstl::math::float_eq<true> oper;

    EXPECT_TRUE(oper(nstl::parse_view<value_type>("12"), value_type{ 12.0 }));
    EXPECT_TRUE(oper(nstl::parse_view<value_type>("-12"), value_type{ -12.0 }));
    EXPECT_TRUE(oper(nstl::parse_view<value_type>("12.3"), value_type{ 12.3 }));
    const auto nan_val = nstl::parse_view<value_type>("nan");
    EXPECT_TRUE(std::isnan(nan_val));
    const auto inf_val = nstl::parse_view<value_type>("inf");
    EXPECT_TRUE(std::isinf(inf_val));
    EXPECT_THROW(nstl::parse_view<value_type>("2rule"), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>(""), std::invalid_argument);
    EXPECT_THROW(nstl::parse_view<value_type>("true_false"), std::invalid_argument);
}

TYPED_TEST(FloatTests, TrueCase)
{
    using value_type = typename TestFixture::value_type;

    const nstl::math::float_eq<true> oper;
    EXPECT_TRUE(oper(value_type{ 3.1415 }, value_type{ 3.1415 }));
    EXPECT_TRUE(oper(value_type{ 0.0 }, value_type{ -0.0 }));
    EXPECT_TRUE(oper(std::numeric_limits<value_type>::infinity(), std::numeric_limits<value_type>::infinity()));
    EXPECT_TRUE(oper(std::numeric_limits<value_type>::quiet_NaN(), std::numeric_limits<value_type>::quiet_NaN()));
    EXPECT_TRUE(
        oper(std::numeric_limits<value_type>::signaling_NaN(), std::numeric_limits<value_type>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<value_type>::quiet_NaN(), std::numeric_limits<value_type>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<value_type>::signaling_NaN(), std::numeric_limits<value_type>::quiet_NaN()));
}

TYPED_TEST(FloatTests, FalseCase)
{
    using value_type = typename TestFixture::value_type;

    const nstl::math::float_eq<false> oper;

    EXPECT_TRUE(oper(value_type{ 3.1415 }, value_type{ 3.1415 }));
    EXPECT_TRUE(oper(value_type{ 0.0 }, value_type{ -0.0 }));
    EXPECT_TRUE(oper(std::numeric_limits<value_type>::infinity(), std::numeric_limits<value_type>::infinity()));
    EXPECT_FALSE(oper(std::numeric_limits<value_type>::quiet_NaN(), std::numeric_limits<value_type>::quiet_NaN()));
    EXPECT_FALSE(
        oper(std::numeric_limits<value_type>::signaling_NaN(), std::numeric_limits<value_type>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<value_type>::quiet_NaN(), std::numeric_limits<value_type>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<value_type>::signaling_NaN(), std::numeric_limits<value_type>::quiet_NaN()));
}

TEST(MathTest, MixedTrue)
{
    const nstl::math::float_eq<true> oper;
    EXPECT_TRUE(oper(float{ 3.1415 }, double{ 3.1415 }));
    EXPECT_TRUE(oper(float{ 0.0 }, double{ -0.0 }));
    EXPECT_TRUE(oper(double{ 3.1415 }, float{ 3.1415 }));
    EXPECT_TRUE(oper(double{ 0.0 }, float{ -0.0 }));

    EXPECT_TRUE(oper(std::numeric_limits<float>::infinity(), std::numeric_limits<double>::infinity()));
    EXPECT_TRUE(oper(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<double>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<double>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<double>::quiet_NaN()));

    EXPECT_TRUE(oper(std::numeric_limits<double>::infinity(), std::numeric_limits<float>::infinity()));
    EXPECT_TRUE(oper(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<double>::signaling_NaN(), std::numeric_limits<float>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<float>::signaling_NaN()));
    EXPECT_TRUE(oper(std::numeric_limits<double>::signaling_NaN(), std::numeric_limits<float>::quiet_NaN()));
}

TEST(MathTest, MixedFalse)
{
    const nstl::math::float_eq<false> oper;
    EXPECT_TRUE(oper(float{ 3.1415 }, double{ 3.1415 }));
    EXPECT_TRUE(oper(float{ 0.0 }, double{ -0.0 }));
    EXPECT_TRUE(oper(double{ 3.1415 }, float{ 3.1415 }));
    EXPECT_TRUE(oper(double{ 0.0 }, float{ -0.0 }));

    EXPECT_TRUE(oper(std::numeric_limits<float>::infinity(), std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(oper(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<double>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<double>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<double>::quiet_NaN()));

    EXPECT_TRUE(oper(std::numeric_limits<double>::infinity(), std::numeric_limits<float>::infinity()));
    EXPECT_FALSE(oper(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<double>::signaling_NaN(), std::numeric_limits<float>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<float>::signaling_NaN()));
    EXPECT_FALSE(oper(std::numeric_limits<double>::signaling_NaN(), std::numeric_limits<float>::quiet_NaN()));
}
