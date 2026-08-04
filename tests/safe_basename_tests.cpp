#include <nstl/safe_basename.hpp>

#include <gtest/gtest.h>

TEST(SafeBaseName, View)
{
    constexpr auto name = ::nstl::safe_basename_view(__FILE__);
    static_assert(!name.empty());
    EXPECT_EQ(name, "safe_basename_tests.cpp");
    EXPECT_EQ(::nstl::safe_basename_view(__FILE__), "safe_basename_tests.cpp");
    EXPECT_EQ(::nstl::safe_basename_view("safe_basename_tests.cpp"), "safe_basename_tests.cpp");
    EXPECT_EQ(::nstl::safe_basename_view(std::string_view{}), std::string_view{});
}

TEST(SafeBaseName, WView)
{
#ifdef _WIN32
    constexpr auto name = ::nstl::safe_basename_view(L"D:\\dir\\path\\safe_basename_tests.cpp");
#else
    constexpr auto name = ::nstl::safe_basename_view(L"/dir/path/safe_basename_tests.cpp");
#endif
    static_assert(!name.empty());
    EXPECT_EQ(name, L"safe_basename_tests.cpp");
    EXPECT_EQ(::nstl::safe_basename_view(L"safe_basename_tests.cpp"), L"safe_basename_tests.cpp");
    EXPECT_EQ(::nstl::safe_basename_view(std::wstring_view{}), std::wstring_view{});
}
