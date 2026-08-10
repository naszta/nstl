#include <gtest/gtest.h>

#include <nstl/backtrace.hpp>
#include <nstl/logging.hpp>
#include <nstl/safe_basename.hpp>

#include <sstream>

TEST(Backtrace, Basic)
{
    std::ostringstream oss;
    nstl::bt::pr_backtrace(oss, __FUNCTION__, nstl::safe_basename_view(__FILE__), __LINE__);
    const auto bt = oss.view();
    EXPECT_FALSE(bt.empty());
    NSTL_INFO("Backtrace:\n" << bt);
}
