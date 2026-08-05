#include <nstl/exception.hpp>

#include <gtest/gtest.h>

#include <sstream>

TEST(Exception, StringCtor)
{
    const nstl::exception exc{ std::string{ "boom" }, "some_file.cpp", 42 };
    EXPECT_STREQ(exc.what(), "boom");
    EXPECT_EQ(exc.file(), "some_file.cpp");
    EXPECT_EQ(exc.line(), 42);
}

TEST(Exception, CharPtrCtor)
{
    const nstl::exception exc{ "boom", "some_file.cpp", 7 };
    EXPECT_STREQ(exc.what(), "boom");
    EXPECT_EQ(exc.file(), "some_file.cpp");
    EXPECT_EQ(exc.line(), 7);
}

TEST(Exception, DefaultFileAndLine)
{
    const nstl::exception exc{ "boom" };
    EXPECT_TRUE(exc.file().empty());
    EXPECT_EQ(exc.line(), 0);
}

TEST(Exception, StreamOperator)
{
    const nstl::exception exc{ "boom" };
    std::ostringstream oss;
    oss << exc;
    EXPECT_EQ(oss.str(), "exception: boom");
}

TEST(Exception, NestedStreamOperator)
{
    std::ostringstream oss;
    try
    {
        try
        {
            throw nstl::exception{ "inner" };
        }
        catch (...)
        {
            std::throw_with_nested(nstl::exception{ "outer" });
        }
    }
    catch (const nstl::exception& exc_)
    {
        oss << exc_;
    }
    EXPECT_EQ(oss.str(), "exception: outer\n exception: inner");
}

TEST(Exception, Macro)
{
    bool threw = false;
    try
    {
        NSTL2_THROW_EXCEPTION("detail " << 123);
    }
    catch (const nstl::exception& exc_)
    {
        threw = true;
        EXPECT_GT(exc_.line(), 0);
        EXPECT_EQ(exc_.file(), "exception_tests.cpp");
        std::ostringstream expected;
        expected << "exception_tests.cpp:" << exc_.line() << " - detail 123";
        EXPECT_EQ(exc_.what(), expected.str());
    }
    EXPECT_TRUE(threw);
}

TEST(Exception, MacroIfTrue)
{
    // NSTL2_THROW_EXCEPTION_IF expands to a brace-init with unparenthesized commas, which the
    // preprocessor would otherwise mis-split as extra arguments to EXPECT_THROW's own macro. Wrapping
    // it in an immediately-invoked lambda inside one extra pair of parens keeps everything nested at
    // paren-depth >= 1, so the outer macro's argument scan sees a single argument.
    EXPECT_THROW(([]() { NSTL2_THROW_EXCEPTION_IF(true, "should throw"); }()), nstl::exception);
}

TEST(Exception, MacroIfFalse)
{
    EXPECT_NO_THROW(([]() { NSTL2_THROW_EXCEPTION_IF(false, "should not throw"); }()));
}

TEST(Exception, NestedMacro)
{
    bool healthy = true;
    try
    {
        try
        {
            NSTL2_THROW_EXCEPTION("inner detail");
        }
        catch (...)
        {
            NSTL2_NESTED_THROW_EXCEPTION("outer detail");
        }
    }
    catch (const nstl::exception& exc_)
    {
        healthy = false;
        std::ostringstream oss;
        oss << exc_;
        const std::string printed = oss.str();
        EXPECT_NE(printed.find("outer detail"), std::string::npos);
        EXPECT_NE(printed.find("inner detail"), std::string::npos);
        EXPECT_NE(printed.find('\n'), std::string::npos);
    }
    EXPECT_FALSE(healthy) << "expected an exception to propagate";
}
