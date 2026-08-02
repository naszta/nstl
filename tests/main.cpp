#include <nstl/logging.hpp>

#include <gtest/gtest.h>

#include <cstdlib>

int main(int argc_, char** argv_)
{
    ::testing::InitGoogleTest(&argc_, argv_);
    nstl::log::Logger logger;
    NSTL_INFO("Log working");
    return RUN_ALL_TESTS();
}
