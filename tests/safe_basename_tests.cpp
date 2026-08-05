#include <nstl/safe_basename.hpp>

#include <gtest/gtest.h>

TEST(SafeBaseName, View)
{
    constexpr auto name = ::nstl::safe_basename_view(__FILE__);
    static_assert(!name.empty());
    static_assert(!name.empty());
    constexpr std::string_view expected_value{ "safe_basename_tests.cpp" };
    static_assert(name.compare(expected_value) == 0);
    constexpr auto basic = ::nstl::safe_basename_view("safe_basename_tests.cpp");
    static_assert(basic.compare(expected_value) == 0);
    constexpr auto empty = ::nstl::safe_basename_view(std::string_view{});
    static_assert(empty.empty());
}

TEST(SafeBaseName, WView)
{
#ifdef _WIN32
    constexpr auto name = ::nstl::safe_basename_view(L"D:\\dir\\path\\safe_basename_tests.cpp");
#else
    constexpr auto name = ::nstl::safe_basename_view(L"/dir/path/safe_basename_tests.cpp");
#endif
    static_assert(!name.empty());
    constexpr std::wstring_view expected_value{ L"safe_basename_tests.cpp" };
    static_assert(name.compare(expected_value) == 0);
    constexpr auto basic = ::nstl::safe_basename_view(L"safe_basename_tests.cpp");
    static_assert(basic.compare(expected_value) == 0);
    constexpr auto empty = ::nstl::safe_basename_view(std::wstring_view{});
    static_assert(empty.empty());
}
