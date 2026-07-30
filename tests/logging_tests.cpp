#include <nstl/logging.hpp>

#include <gtest/gtest.h>

#include <thread>

TEST(Logging, Test)
{
    const std::string_view expected{ "Testing passed info\n" };
    std::ostringstream target;
    nstl::log::Logger logger{ target };

    std::thread test_thr{ []()
                          {
                              NSTL_DEBUG("Testing passed debug");
                              NSTL_INFO("Testing passed info");
                          } };
    std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
    test_thr.join();
    logger.reset();
    EXPECT_TRUE(target.view().ends_with(expected))
        << "Expected value: \"" << expected << "\"; actual: \"" << target.view() << "\"";
}

TEST(Logging, Multi)
{
    const std::string_view debug_log{ "Testing passed debug" };
    const std::string_view info_log{ "Testing passed info" };

    using test_buffer = std::vector<std::pair<nstl::log::LogLevel::LogEnum, std::string>>;

    nstl::log::LogLevel::setLevel(nstl::log::LogLevel::Debug);
    const auto target = std::make_shared<test_buffer>();
    std::weak_ptr<test_buffer> wptr{ target };

    nstl::log::logger() =
        [wptr = std::move(wptr)](const nstl::log::LogLevel::LogEnum level_, const std::string_view line_)
    {
        if (const auto ptr = wptr.lock())
        {
            ptr->emplace_back(level_, std::string{ line_.data(), line_.size() });
        }
    };

    NSTL_DEBUG(debug_log);
    NSTL_INFO(info_log);
    ASSERT_EQ(target->size(), 2U);
    EXPECT_TRUE(target->front().second.ends_with(debug_log));
    EXPECT_EQ(target->front().first, nstl::log::LogLevel::Debug);
    EXPECT_TRUE(target->back().second.ends_with(info_log));
    EXPECT_EQ(target->back().first, nstl::log::LogLevel::Info);
}

TEST(Logging, Levels)
{
    EXPECT_EQ("DEBUG", nstl::log::LogLevel::name(nstl::log::LogLevel::Debug));
    EXPECT_EQ("INFO", nstl::log::LogLevel::name(nstl::log::LogLevel::Info));
    EXPECT_EQ("WARNING", nstl::log::LogLevel::name(nstl::log::LogLevel::Warning));
    EXPECT_EQ("ERROR", nstl::log::LogLevel::name(nstl::log::LogLevel::Error));

    EXPECT_EQ(nstl::log::LogLevel::parseLevel("DEBUG"), nstl::log::LogLevel::Debug);
    EXPECT_EQ(nstl::log::LogLevel::parseLevel("INFO"), nstl::log::LogLevel::Info);
    EXPECT_EQ(nstl::log::LogLevel::parseLevel("WARNING"), nstl::log::LogLevel::Warning);
    EXPECT_EQ(nstl::log::LogLevel::parseLevel("ERROR"), nstl::log::LogLevel::Error);

    EXPECT_THROW(nstl::log::LogLevel::parseLevel("Non sense"), std::exception);
}