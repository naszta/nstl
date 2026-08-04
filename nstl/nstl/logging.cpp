#include "logging.hpp"
#include "exception.hpp"

#include <cstdlib>

#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>
#include <format>
#include <iterator>
#include <limits>
#include <iostream>

#include <oneapi/tbb/concurrent_queue.h>

namespace nstl::log
{
namespace
{
std::atomic<LogLevel::LogInt> current_level{ static_cast<LogLevel::LogInt>(LogLevel::Info) };

constexpr std::array<std::string_view, 5> log_levels{ "DEBUG", "INFO", "WARNING", "ERROR", "TERMINATE" };
} // namespace

LogLevel::LogEnum LogLevel::parseLevel(const std::string_view view_)
{
    if (const auto itr = std::ranges::find(log_levels, view_); itr != log_levels.cend()) [[likely]]
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

LogLevel::LogEnum LogLevel::setLevel(const LogEnum level)
{
    return static_cast<LogLevel::LogEnum>(current_level.exchange(static_cast<LogLevel::LogInt>(level)));
}

LogLevel::LogEnum LogLevel::getLevel()
{
    return static_cast<LogLevel::LogEnum>(current_level.load(std::memory_order::relaxed));
}

bool LogLevel::isLevelActive(const LogEnum level) { return getLevel() <= level; }

LogFunc& logger()
{
    static LogFunc instance;
    return instance;
}

class LoggerImpl final : public std::enable_shared_from_this<LoggerImpl>
{
    const LogLevel::LogEnum _level;
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
    LoggerImpl(const std::filesystem::path& logfile_, const LogLevel::LogEnum level)
        : _level{ level }, _ofs{ logfile_, std::ios_base::out | std::ios_base::binary | std::ios_base::app },
          _tgt{ _ofs }, _runner{ &LoggerImpl::worker, this }
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

    explicit LoggerImpl(std::ostream& os_, const LogLevel::LogEnum level)
        : _level{ level }, _tgt{ os_ }, _runner{ &LoggerImpl::worker, this }
    {
    }

    ~LoggerImpl() { this->stop(); }

    void push(const LogLevel::LogEnum level, std::string&& line_)
    {
        if (_throttle_size.load(std::memory_order::relaxed) < _queue.size()) [[unlikely]]
        {
            return;
        }
        _queue.emplace(std::move(line_));

        if (level == LogLevel::Terminate) [[unlikely]]
        {
            this->stop();
            std::abort();
        }
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

    LogLevel::LogEnum getLevel() const { return _level; }
};

namespace
{
class log_stack
{
    mutable std::shared_mutex _lock;
    std::vector<std::shared_ptr<LoggerImpl>> _items;

    log_stack() = default;

    bool _set_functor()
    {
        if (_items.empty())
        {
            return false;
        }
        const auto& ptr = _items.back();

        std::weak_ptr<LoggerImpl> wptr{ ptr };
        logger() = [wptr = std::move(wptr)](const LogLevel::LogEnum level, const std::string_view line)
        {
            if (const auto ptr = wptr.lock())
            {
                ptr->push(level, std::string{ line.data(), line.size() });
            }
        };
        LogLevel::setLevel(ptr->getLevel());
        return true;
    }

public:
    ~log_stack() = default;
    log_stack(const log_stack&) = delete;
    log_stack& operator=(const log_stack&) = delete;

    static log_stack& instance()
    {
        static log_stack item;
        return item;
    }

    bool push_back(std::shared_ptr<LoggerImpl> log)
    {
        std::lock_guard lg{ _lock };
        _items.emplace_back(std::move(log));
        return this->_set_functor();
    }

    bool erase(const std::shared_ptr<LoggerImpl>& log)
    {
        std::lock_guard lg{ _lock };
        if (!_items.empty() && _items.back() == log)
        {
            _items.pop_back();
            return this->_set_functor();
        }
        else
        {
            _items.erase(std::remove(_items.begin(), _items.end(), log), _items.end());
            return false;
        }
    }
};

std::atomic_bool active_cout_logger{ false };
} // namespace

Logger::Logger(std::shared_ptr<LoggerImpl> log, bool cout_logger) : _cout_logger{ cout_logger }, _log{ std::move(log) }
{
    NSTL2_THROW_EXCEPTION_IF(_cout_logger && active_cout_logger.exchange(true), "cout logger is already active!");
    log_stack::instance().push_back(_log);
}

Logger::Logger(const LogLevel::LogEnum level) : Logger{ std::make_shared<LoggerImpl>(std::cout, level), true } {}

Logger::Logger(const std::filesystem::path& tgt_, const LogLevel::LogEnum level)
    : Logger{ std::make_shared<LoggerImpl>(tgt_, level), false }
{
}

Logger::Logger(std::ostream& os_, const LogLevel::LogEnum level)
    : Logger{ std::make_shared<LoggerImpl>(os_, level), false }
{
}

Logger::~Logger() { this->reset(); }

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

LogLevel::LogEnum Logger::getLevel(const LogLevel::LogEnum def) const { return _log ? _log->getLevel() : def; }

void Logger::reset()
{
    std::shared_ptr<LoggerImpl> log;
    log.swap(_log);
    if (!log)
    {
        return;
    }
    if (_cout_logger)
    {
        active_cout_logger.store(false);
    }
    log_stack::instance().erase(log);
}

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
