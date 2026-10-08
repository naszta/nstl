#ifndef _NSTL_LOGGER
#define _NSTL_LOGGER 1

#include <nstl/logging.hpp>

namespace nstl::log
{
class LoggerImpl;

class LoggerAPI
{
protected:
    const level::values _level;

public:
    explicit LoggerAPI(level::values level_);
    virtual ~LoggerAPI();
    LoggerAPI(const LoggerAPI&) = delete;
    LoggerAPI& operator=(const LoggerAPI&) = delete;

    virtual void push(level::values level, std::string&& line_) = 0;
    virtual bool stop() = 0;
    virtual void throttleSize(const std::ptrdiff_t size_) = 0;

    virtual size_t size() const = 0;
    virtual level::values getLevel() const { return _level; }
    virtual bool is_cout_logger() const { return false; }
};

class Logger
{
    std::shared_ptr<LoggerAPI> _log;

public:
    explicit Logger(level::values level = level::Info);
    explicit Logger(const std::filesystem::path& tgt_, level::values level = level::Info);
    explicit Logger(std::ostream& os_, level::values level = level::Info);
    explicit Logger(std::shared_ptr<LoggerAPI> log_);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool throttleSize(std::ptrdiff_t size_);
    size_t size() const;
    void reset();
    level::values getLevel(level::values def = level::Info) const;
};
} // namespace nstl::log

#endif
