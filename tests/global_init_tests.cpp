#include <nstl/global_init.hpp>

#include <gtest/gtest.h>

// nstl::global_init is a process-wide singleton: main.cpp already constructs one instance before
// RUN_ALL_TESTS() runs, so these tests only cover the paths that don't touch that shared state -
// the guard against a second construction (which throws before doing any real init work), and the
// read-only accessor for the signal fd it opened.

TEST(GlobalInit, SecondInstanceThrows)
{
    EXPECT_THROW((nstl::global_init{}), std::exception);
}

TEST(GlobalInit, GetSignalFileIsStableAcrossCalls)
{
    const int first = nstl::global_init::getSignalFile();
    const int second = nstl::global_init::getSignalFile();
    EXPECT_EQ(first, second);
}
