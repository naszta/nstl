#include <nstl/logging.hpp>
#include <nstl/scope_exit.hpp>
#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

#include <fstream>
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

    const auto prev_level = nstl::log::LogLevel::setLevel(nstl::log::LogLevel::Debug);
    const auto target = std::make_shared<test_buffer>();
    std::weak_ptr<test_buffer> wptr{ target };

    nstl::log::LogFunc current{ [wptr = std::move(wptr)](const nstl::log::LogLevel::LogEnum level_,
                                                         const std::string_view line_)
                                {
                                    if (const auto ptr = wptr.lock())
                                    {
                                        ptr->emplace_back(level_, std::string{ line_.data(), line_.size() });
                                    }
                                } };

    std::swap(nstl::log::logger(), current);
    const auto cleanup = nstl::on_scope_exit(
        [&current, &prev_level]()
        {
            std::swap(nstl::log::logger(), current);
            nstl::log::LogLevel::setLevel(prev_level);
        });

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

TEST(Logging, NameInvalidLevelThrows)
{
    EXPECT_THROW(nstl::log::LogLevel::name(static_cast<nstl::log::LogLevel::LogEnum>(-1)), std::exception);
    EXPECT_THROW(nstl::log::LogLevel::name(static_cast<nstl::log::LogLevel::LogEnum>(99)), std::exception);
}

TEST(Logging, FileBackedLogger)
{
    const nstl::temp_dir dir;
    const auto log_path = dir / "test.log";
    const std::string_view expected{ "Testing file logger" };
    {
        nstl::log::Logger logger{ log_path };
        NSTL_INFO(expected);
        logger.reset();
    }

    std::ifstream ifs{ log_path };
    ASSERT_TRUE(ifs.good());
    std::string content{ std::istreambuf_iterator<char>{ ifs }, std::istreambuf_iterator<char>{} };
    EXPECT_TRUE(content.ends_with(std::string{ expected } + "\n")) << content;
}

TEST(Logging, FileBackedLoggerCannotOpenThrows)
{
    const nstl::temp_dir dir;
    const auto bad_path = dir / "no_such_subdir" / "test.log";
    EXPECT_THROW(nstl::log::Logger logger(bad_path), std::exception);
}

TEST(Logging, ThrottleSizeNegativeThrows)
{
    std::ostringstream target;
    nstl::log::Logger logger{ target };
    EXPECT_THROW(logger.throttleSize(-1), std::exception);
}

TEST(Logging, ThrottleSizeAndGetLevel)
{
    std::ostringstream target;
    nstl::log::Logger logger{ target, nstl::log::LogLevel::Warning };
    EXPECT_TRUE(logger.throttleSize(1024));
    EXPECT_EQ(logger.getLevel(), nstl::log::LogLevel::Warning);

    logger.reset();
    // after reset() the logger holds no LoggerImpl, so getLevel()/throttleSize() fall back to defaults.
    EXPECT_EQ(logger.getLevel(nstl::log::LogLevel::Error), nstl::log::LogLevel::Error);
    EXPECT_FALSE(logger.throttleSize(1024));
}

TEST(Logging, LogTimeZoneDefaultsToUtc)
{
    nstl::log::LogTimeZone tz;
    std::ostringstream oss;
    oss << tz;
    EXPECT_EQ(oss.str(), "UTC");
}

TEST(Logging, LogTimeZoneCurrent)
{
    nstl::log::LogTimeZone tz;
    tz.setZone(std::string{});
    std::ostringstream oss;
    oss << tz;
    EXPECT_EQ(oss.str(), "current");
}

TEST(Logging, LogTimeZoneNamed)
{
    nstl::log::LogTimeZone tz;
    tz.setZone(std::string{ "UTC" });
    std::ostringstream oss;
    oss << tz;
    EXPECT_EQ(oss.str(), "UTC");
}

TEST(Logging, LogTimeZoneInvalidNameThrows)
{
    nstl::log::LogTimeZone tz;
    EXPECT_THROW(tz.setZone(std::string{ "Not/AZone" }), std::exception);
}

TEST(Logging, LogTimeZonePrintStampNotEmpty)
{
    nstl::log::LogTimeZone tz;
    std::ostringstream oss;
    tz.printStamp(oss);
    EXPECT_FALSE(oss.str().empty());
}