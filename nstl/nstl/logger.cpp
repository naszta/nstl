#include "logger.hpp"
#include "exception.hpp"
#include "memory.hpp"

#include <oneapi/tbb/concurrent_queue.h>

#include <fstream>
#include <iostream>

namespace nstl::log
{
LoggerAPI::LoggerAPI(const level::values level_) : _level{ level_ } {}
LoggerAPI::~LoggerAPI() = default;

class OstreamLog : public LoggerAPI
{
    observer_ptr<std::ostream> _tgt;
    tbb::concurrent_bounded_queue<std::optional<std::string>> _queue;
    std::atomic_bool _running{ false };
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
                (*_tgt) << line.value() << std::endl;
            }
            else
            {
                return;
            }
        }
    }

protected:
    explicit OstreamLog(const level::values level_) : LoggerAPI{ level_ } {}
    void start(std::ostream& tgt_)
    {
        if (!_running.exchange(true))
        {
            _tgt = make_observer(&tgt_);
            _runner = std::thread{ &OstreamLog::worker, this };
        }
    }

public:
    OstreamLog(std::ostream& tgt_, const level::values level_) : OstreamLog{ level_ } { this->start(tgt_); }
    ~OstreamLog() { this->stop(); }

    void push(const level::values level_, std::string&& line_) override
    {
        if (_throttle_size.load(std::memory_order::relaxed) < _queue.size()) [[unlikely]]
        {
            return;
        }

        _queue.emplace(std::move(line_));

        if (level_ == level::Terminate) [[unlikely]]
        {
            this->stop();
            std::abort();
        }
    }

    bool stop() override
    {
        if (_running.exchange(false))
        {
            _queue.emplace(std::nullopt);
            _runner.join();
            _tgt = nullptr;
            return true;
        }
        return false;
    }

    void throttleSize(const std::ptrdiff_t size_) override { _throttle_size.store(size_); }

    size_t size() const override { return _queue.size(); }
};

class CoutLog final : public OstreamLog
{
public:
    explicit CoutLog(const level::values level_) : OstreamLog{ std::cout, level_ } {}
    bool is_cout_logger() const final { return true; }
};

class FileLog final : public OstreamLog
{
    std::ofstream _ofs;

public:
    FileLog(const std::filesystem::path& tgt_, const level::values level_) : OstreamLog{ level_ }
    {
        _ofs.open(tgt_, std::ios_base::app | std::ios_base::out | std::ios_base::binary);
        NSTL2_THROW_EXCEPTION_IF(!_ofs.is_open() || !_ofs.good(), tgt_ << " open for write failed");
        this->start(_ofs);
    }

    bool stop() override
    {
        if (OstreamLog::stop())
        {
            _ofs.close();
            return true;
        }
        return false;
    }

    ~FileLog() final { this->stop(); }
};

namespace
{
class log_stack
{
    mutable std::shared_mutex _lock;
    std::vector<std::shared_ptr<LoggerAPI>> _items;

    log_stack() = default;

    bool _set_functor()
    {
        if (_items.empty())
        {
            return false;
        }
        const auto& ptr = _items.back();

        std::weak_ptr<LoggerAPI> wptr{ ptr };
        ::nstl::log::logger() = [wptr = std::move(wptr)](const level::values level, const std::string_view line)
        {
            if (const auto ptr = wptr.lock())
            {
                ptr->push(level, std::string{ line.data(), line.size() });
            }
        };
        level::setLevel(ptr->getLevel());
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

    bool push_back(std::shared_ptr<LoggerAPI> log)
    {
        std::lock_guard lg{ _lock };
        _items.emplace_back(std::move(log));
        return this->_set_functor();
    }

    bool erase(const std::shared_ptr<LoggerAPI>& log)
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

Logger::Logger(std::shared_ptr<LoggerAPI> log) : _log{ std::move(log) }
{
    NSTL2_THROW_EXCEPTION_IF(!_log, "Logger API is nullptr");
    NSTL2_THROW_EXCEPTION_IF(_log->is_cout_logger() && active_cout_logger.exchange(true),
                             "cout logger is already active!");
    log_stack::instance().push_back(_log);
}

Logger::Logger(const level::values level) : Logger{ std::make_shared<CoutLog>(level) } {}

Logger::Logger(const std::filesystem::path& tgt_, const level::values level_)
    : Logger{ std::make_shared<FileLog>(tgt_, level_) }
{
}

Logger::Logger(std::ostream& os_, const level::values level) : Logger{ std::make_shared<OstreamLog>(os_, level) } {}

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

level::values Logger::getLevel(const level::values def) const { return _log ? _log->getLevel() : def; }

void Logger::reset()
{
    std::shared_ptr<LoggerAPI> log;
    log.swap(_log);
    if (!log)
    {
        return;
    }
    if (log->is_cout_logger())
    {
        active_cout_logger.store(false);
    }
    log_stack::instance().erase(log);
}
} // namespace nstl::log
