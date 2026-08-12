#ifndef _NSTL_LOGGING
#define _NSTL_LOGGING 1

#include <nstl/compiler.hpp>
#include <nstl/safe_basename.hpp>

#include <cstdint>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <sstream>
#include <string_view>
#include <type_traits>

#ifdef NSTL_USING_HH_DATE
NSTL_WRN_DATE_PUSH
#include <date/tz.h>
NSTL_WRN_DATE_POP
#else
namespace date = std::chrono;
#endif

namespace nstl::log
{
constexpr const char delimiter = '|';

class LogLevel
{
public:
    LogLevel() = delete;
    ~LogLevel() = delete;
    LogLevel(const LogLevel&) = delete;
    LogLevel& operator=(const LogLevel&) = delete;

    enum LogEnum : std::int16_t
    {
        Debug,
        Info,
        Warning,
        Error,
        Terminate,
    };

    using LogInt = std::underlying_type_t<LogEnum>;

    static LogEnum parseLevel(std::string_view view_);
    static std::ostream& toStream(std::ostream& os_, LogEnum level_);
    static std::string_view name(LogEnum level_);

    static LogEnum setLevel(LogEnum level);
    static LogEnum getLevel();
    static bool isLevelActive(LogEnum level);
};

using LogFunc = std::function<void(LogLevel::LogEnum level, std::string_view line)>;
LogFunc& logger();

class LoggerImpl;

class Logger
{
    const bool _cout_logger{ false };
    std::shared_ptr<LoggerImpl> _log;

    Logger(std::shared_ptr<LoggerImpl> log, bool cout_logger);

public:
    explicit Logger(LogLevel::LogEnum level = LogLevel::Info);
    explicit Logger(const std::filesystem::path& tgt_, LogLevel::LogEnum level = LogLevel::Info);
    explicit Logger(std::ostream& os_, LogLevel::LogEnum level = LogLevel::Info);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool throttleSize(std::ptrdiff_t size_);
    size_t size() const;
    void reset();
    LogLevel::LogEnum getLevel(LogLevel::LogEnum def = LogLevel::Info) const;
};

class LogTimeZone
{
    const date::time_zone* _zone{ nullptr };
    std::optional<std::string> _name;
    mutable std::shared_mutex _lock;

    inline const date::time_zone* _parse_zone(const std::string_view zone_) const;

public:
    static LogTimeZone& tz_instance();

    LogTimeZone();
    ~LogTimeZone();
    LogTimeZone(const LogTimeZone&) = delete;
    LogTimeZone& operator=(const LogTimeZone&) = delete;

    void setZone(std::optional<std::string> zone_);

    inline LogTimeZone& operator=(std::optional<std::string> zone_)
    {
        this->setZone(std::move(zone_));
        return *this;
    }
    std::ostringstream& printStamp(std::ostringstream& oss_) const;

    friend std::ostream& operator<<(std::ostream& os_, const LogTimeZone& zone_);
};

class LoggerFormatter
{
public:
    LoggerFormatter(const LogTimeZone& tz_, LogLevel::LogEnum level_, std::string_view file_, int line_);
    ~LoggerFormatter();
    LoggerFormatter(const LoggerFormatter&) = delete;
    LoggerFormatter& operator=(const LoggerFormatter&) = delete;

    void operator()() const;
    std::ostringstream& target();
    std::string_view logLine() const;

private:
    const LogLevel::LogEnum _level{ LogLevel::Debug };
    std::ostringstream _oss;
};
} // namespace nstl::log

#define NSTL_LOG_LEVEL_IMPL(level, details, help)                                                     \
    do                                                                                                \
    {                                                                                                 \
        if (::nstl::log::LogLevel::isLevelActive(level)) [[help]]                                     \
        {                                                                                             \
            ::nstl::log::LoggerFormatter __logger_{ ::nstl::log::LogTimeZone::tz_instance(), level,   \
                                                    ::nstl::safe_basename_view(__FILE__), __LINE__ }; \
            __logger_.target() << details;                                                            \
            __logger_();                                                                              \
        }                                                                                             \
    } while (false)

#define NSTL_LOG_LEVEL(level, details) NSTL_LOG_LEVEL_IMPL(level, details, likely)

#define NSTL_DEBUG(details) NSTL_LOG_LEVEL_IMPL(::nstl::log::LogLevel::Debug, details, unlikely)
#define NSTL_INFO(details) NSTL_LOG_LEVEL(::nstl::log::LogLevel::Info, details)
#define NSTL_WARNING(details) NSTL_LOG_LEVEL(::nstl::log::LogLevel::Warning, details)
#define NSTL_ERROR(details) NSTL_LOG_LEVEL(::nstl::log::LogLevel::Error, details)
#define NSTL_TERMINATE(details) NSTL_LOG_LEVEL(::nstl::log::LogLevel::Terminate, details)

#endif
