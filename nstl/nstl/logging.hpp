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

namespace level
{
enum values : std::int16_t
{
    Debug,
    Info,
    Warning,
    Error,
    Terminate,
};

using int_type = std::underlying_type_t<values>;

values parseLevel(std::string_view view_);
std::ostream& toStream(std::ostream& os_, values level_);
std::string_view name(values level_);
values setLevel(values level);
values getLevel();
bool isLevelActive(values level);
} // namespace level

using LogFunc = std::function<void(level::values level, std::string_view line)>;
LogFunc& logger();

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
    LoggerFormatter(const LogTimeZone& tz_, level::values level_, std::string_view file_, int line_);
    ~LoggerFormatter();
    LoggerFormatter(const LoggerFormatter&) = delete;
    LoggerFormatter& operator=(const LoggerFormatter&) = delete;

    void operator()() const;
    std::ostringstream& target();
    std::string_view logLine() const;

private:
    const level::values _level{ level::Debug };
    std::ostringstream _oss;
};
} // namespace nstl::log

#define NSTL_LOG_LEVEL_IMPL(loglevel, details, help)                                                   \
    do                                                                                                 \
    {                                                                                                  \
        if (::nstl::log::level::isLevelActive(loglevel)) [[help]]                                      \
        {                                                                                              \
            ::nstl::log::LoggerFormatter __logger_{ ::nstl::log::LogTimeZone::tz_instance(), loglevel, \
                                                    ::nstl::safe_basename_view(__FILE__), __LINE__ };  \
            __logger_.target() << details;                                                             \
            __logger_();                                                                               \
        }                                                                                              \
    } while (false)

#define NSTL_LOG_LEVEL(loglevel, details) NSTL_LOG_LEVEL_IMPL(loglevel, details, likely)

#define NSTL_DEBUG(details) NSTL_LOG_LEVEL_IMPL(::nstl::log::level::values::Debug, details, unlikely)
#define NSTL_INFO(details) NSTL_LOG_LEVEL(::nstl::log::level::values::Info, details)
#define NSTL_WARNING(details) NSTL_LOG_LEVEL(::nstl::log::level::values::Warning, details)
#define NSTL_ERROR(details) NSTL_LOG_LEVEL(::nstl::log::level::values::Error, details)
#define NSTL_TERMINATE(details) NSTL_LOG_LEVEL(::nstl::log::level::values::Terminate, details)

#endif
