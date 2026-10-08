#include "logging.hpp"
#include "exception.hpp"
#include "string.hpp"

#include <cstdlib>

#include <algorithm>
#include <array>
#include <atomic>
#include <format>
#include <iterator>
#include <limits>
#include <iostream>
#include <thread>

namespace nstl::log
{
namespace
{
std::atomic<level::int_type> current_level{ static_cast<level::int_type>(level::Info) };

constexpr std::array<std::string_view, 5> log_levels{ "DEBUG", "INFO", "WARNING", "ERROR", "TERMINATE" };
} // namespace

level::values level::parseLevel(const std::string_view view_)
{
    if (const auto itr = std::ranges::find(log_levels, view_); itr != log_levels.cend()) [[likely]]
    {
        return static_cast<values>(std::distance(log_levels.cbegin(), itr));
    }
    NSTL2_THROW_EXCEPTION(view_ << " level is not known");
}

std::string_view level::name(const values level_)
{
    NSTL2_THROW_EXCEPTION_IF(level_ < 0 || log_levels.size() <= static_cast<std::uint32_t>(level_), "Invalid level");
    return log_levels[static_cast<int_type>(level_)];
}

std::ostream& level::toStream(std::ostream& os_, const values level_)
{
    os_ << level::name(level_);
    return os_;
}

level::values level::setLevel(const values level)
{
    return static_cast<level::values>(current_level.exchange(static_cast<level::int_type>(level)));
}

level::values level::getLevel() { return static_cast<level::values>(current_level.load(std::memory_order::relaxed)); }

bool level::isLevelActive(const level::values level) { return getLevel() <= level; }

LogFunc& logger()
{
    static LogFunc instance;
    return instance;
}

const date::time_zone* LogTimeZone::_parse_zone(const std::string_view zone_) const
{
    constexpr std::string_view utc_view{ "utc" };
    if (zone_.empty())
    {
        return date::current_zone();
    }
    else if (iequal(zone_, utc_view))
    {
        return nullptr;
    }
    else
    {
        return date::locate_zone(zone_);
    }
}

LogTimeZone& LogTimeZone::tz_instance()
{
    static LogTimeZone item;
    return item;
}

LogTimeZone::LogTimeZone()
{
    if (const char* zone_name = std::getenv("LOG_TZ"); zone_name)
    {
        _name.emplace(zone_name);
        _zone = _parse_zone(_name.value());
    }
}
LogTimeZone::~LogTimeZone() = default;

void LogTimeZone::setZone(std::optional<std::string> zone_)
{
    std::lock_guard lg{ _lock };
    _zone = zone_ ? _parse_zone(*zone_) : nullptr;
    _name = std::move(zone_);
}

std::ostringstream& LogTimeZone::printStamp(std::ostringstream& oss_) const
{
    const auto utc_now = std::chrono::system_clock::now();
    std::shared_lock sl{ _lock };
    if (_zone)
    {
        const date::zoned_time zd{ _zone, utc_now };
#ifdef NSTL_USING_HH_DATE
        date::to_stream(oss_, "%FT%T%z", zd);
#else
        std::format_to(std::ostream_iterator<char>{ oss_ }, "{:%FT%T%z}", zd);
#endif
    }
    else
    {
#ifdef NSTL_USING_HH_DATE
        date::to_stream(oss_, "%FT%TZ", utc_now);
#else
        std::format_to(std::ostream_iterator<char>{ oss_ }, "{:%FT%TZ}", utc_now);
#endif
    }
    return oss_;
}

std::ostream& operator<<(std::ostream& os_, const LogTimeZone& zone_)
{
    if (zone_._name)
    {
        if (zone_._name->empty())
        {
            os_ << "current";
        }
        else
        {
            os_ << zone_._name.value();
        }
    }
    else
    {
        os_ << "UTC";
    }
    return os_;
}

LoggerFormatter::LoggerFormatter(const LogTimeZone& tz_, const level::values level_, const std::string_view file_,
                                 const int line_)
    : _level{ level_ }
{
    tz_.printStamp(_oss) << delimiter;
    _oss << file_ << ':' << line_ << delimiter;
    _oss << std::this_thread::get_id() << delimiter;
    level::toStream(_oss, _level);
    _oss << delimiter;
}

LoggerFormatter::~LoggerFormatter() = default;

void LoggerFormatter::operator()() const
{
    if (const auto tgt = logger()) [[likely]]
    {
        tgt(_level, this->logLine());
    }
}

std::ostringstream& LoggerFormatter::target() { return _oss; }

std::string_view LoggerFormatter::logLine() const { return _oss.view(); }
} // namespace nstl::log
