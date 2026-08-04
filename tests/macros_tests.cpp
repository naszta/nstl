#include <nstl/macros.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

TEST(Macros, Coverage)
{
    const std::string_view content{ "Example exception" };
    bool no_exception = true;
	try
	{
        NSTL_THROW_EXCEPTION(std::runtime_error, content);
	}
	catch (const std::exception& exc_)
	{
        no_exception = false;
        const std::string_view exc_view{exc_.what()};
        EXPECT_TRUE(exc_view.ends_with(content));
	}
    EXPECT_FALSE(no_exception) << " no exception raised, come on!";
}
