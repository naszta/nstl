#include "logging.hpp"
#include "exception.hpp"

#include <cstdlib>

#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>
#include <limits>
#include <iostream>

#include <oneapi/tbb/concurrent_queue.h>

#ifdef NSTL_USING_HH_DATE
#include <date/tz.h>
#endif

namespace nstl::log
{
namespace
{
using LogLevelAtom = std::atomic<LogLevel::LogInt>;

LogLevelAtom current_level{ static_cast<LogLevel::LogInt>(LogLevel::Info) };

constexpr std::array<std::string_view, 4> log_levels{ "DEBUG", "INFO", "WARNING", "ERROR" };
} // namespace

LogLevel::LogEnum LogLevel::parseLevel(const std::string_view view_)
{
    const auto itr = std::ranges::find(log_levels, view_);
    if (itr != log_levels.cend()) [[likely]]
    {
        return static_cast<LogEnum>(std::distance(log_levels.cbegin(), itr));
    }
    NSTL2_THROW_EXCEPTION(view_ << " level is not known");
}

std::string_view LogLevel::name(const LogEnum level_)
{
    NSTL2_THROW_EXCEPTION_IF(level_ < 0 || log_levels.size() <= static_cast<std::uint32_t>(level_), "Invalid level");
    return log_levels[static_cast<LogInt>(level_)];
}

std::ostream& LogLevel::toStream(std::ostream& os_, const LogEnum level_)
{
    os_ << LogLevel::name(level_);
    return os_;
}

void LogLevel::setLevel(const LogEnum level) { current_level.store(static_cast<LogLevel::LogInt>(level)); }

LogLevel::LogEnum LogLevel::getLevel() { return static_cast<LogLevel::LogEnum>(current_level.load()); }

bool LogLevel::isLevelActive(const LogEnum level) { return getLevel() <= level; }

LogFunc& logger()
{
    static LogFunc instance;
    return instance;
}

class LoggerImpl : public std::enable_shared_from_this<LoggerImpl>
{
    std::ofstream _ofs;
    std::ostream& _tgt;
    tbb::concurrent_bounded_queue<std::optional<std::string>> _queue;
    std::atomic_bool _running{ true };
    std::thread _runner;
    std::atomic_ptrdiff_t _throttle_size{ std::numeric_limits<std::ptrdiff_t>::max() };

    void worker()
    {
        std::optional<std::string> line;
        while (true)
        {
            _queue.pop(line);
            if (line.has_value()) [[likely]]
            {
                _tgt << line.value() << std::endl;
            }
            else
            {
                return;
            }
        }
    }

public:
    explicit LoggerImpl(const std::filesystem::path& logfile_)
        : _ofs{ logfile_, std::ios_base::out | std::ios_base::binary | std::ios_base::app }, _tgt{ _ofs },
          _runner{ &LoggerImpl::worker, this }
    {
        try
        {
            NSTL2_THROW_EXCEPTION_IF(!_ofs.good(), logfile_ << " cannot be opened for write");
        }
        catch (const std::exception&)
        {
            this->stop();
            throw;
        }
    }

    explicit LoggerImpl(std::ostream& os_) : _tgt{ os_ }, _runner{ &LoggerImpl::worker, this } {}

    ~LoggerImpl() { this->stop(); }

    void push(std::string&& line_)
    {
        if (_throttle_size.load(std::memory_order::relaxed) < _queue.size()) [[unlikely]]
        {
            return;
        }
        _queue.emplace(std::move(line_));
    }

    size_t size() const { return _queue.size(); }

    void stop()
    {
        if (_running.exchange(false))
        {
            _queue.emplace(std::nullopt);
            _runner.join();
        }
    }

    void throttleSize(const std::ptrdiff_t size_) { _throttle_size.store(size_); }
};

Logger::Logger(std::shared_ptr<LoggerImpl> log, const LogLevel::LogEnum level) : _log{ std::move(log) }
{
    LogLevel::setLevel(level);
    std::weak_ptr wptr{ _log };
    logger() = [wptr = std::move(wptr)](LogLevel::LogEnum, const std::string_view line)
    {
        if (auto lptr = wptr.lock()) [[likely]]
        {
            lptr->push(std::string{ line.data(), line.size() });
        }
    };
}

Logger::Logger(const LogLevel::LogEnum level) : Logger{ std::cout, level } {}

Logger::Logger(const std::filesystem::path& tgt_, const LogLevel::LogEnum level)
    : Logger{ std::make_shared<LoggerImpl>(tgt_), level }
{
}

Logger::Logger(std::ostream& os_, const LogLevel::LogEnum level) : Logger{ std::make_shared<LoggerImpl>(os_), level } {}

Logger::~Logger() = default;

size_t Logger::size() const { return _log ? _log->size() : 0; }

bool Logger::throttleSize(const std::ptrdiff_t size_)
{
    NSTL2_THROW_EXCEPTION_IF(size_ < 0, "negative log throttle size (" << size_ << ") doesn't make any sense");
    if (_log) [[likely]]
    {
        _log->throttleSize(size_);
        return true;
    }
    return false;
}

void Logger::reset() { _log.reset(); }

const date::time_zone* LogTimeZone::_parse_zone(const std::string_view zone_) const
{
    if (zone_.empty())
    {
        return date::current_zone();
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
        oss_ << std::format("{0:%F}T{0:%T%z}", zd);
#endif
    }
    else
    {
#ifdef NSTL_USING_HH_DATE
        date::to_stream(oss_, "%FT%TZ", utc_now);
#else
        oss_ << std::format("{0:%F}T{0:%T}Z", utc_now);
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

namespace
{
constexpr char delimiter = '|';
}

LoggerFormatter::LoggerFormatter(const LogTimeZone& tz_, const LogLevel::LogEnum level_, const std::string_view file_,
                                 const int line_)
    : _level{ level_ }
{
    tz_.printStamp(_oss) << delimiter;
    _oss << file_ << ':' << line_ << delimiter;
    _oss << std::this_thread::get_id() << delimiter;
    LogLevel::toStream(_oss, _level);
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
