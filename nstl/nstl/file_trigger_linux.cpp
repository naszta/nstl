#include "file_trigger.hpp"
#include "exception.hpp"
#include "handle_raii.hpp"
#include "temp_dir.hpp"

#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <array>
#include <atomic>
#include <thread>
#include <utility>
#include <vector>

namespace nstl
{

namespace
{
constexpr std::string_view exit_value{ "exit: true\n" };

bool set_non_blocking(int fd)
{
    const auto flags = ::fcntl(fd, F_GETFL, 0);
    NSTL2_THROW_EXCEPTION_IF(flags < 0, "fcntl F_GETFL failed: " << fd);
    if (flags & O_NONBLOCK)
    {
        return false;
    }
    NSTL2_THROW_EXCEPTION_IF(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0,
                             "fcntl F_SETFL failed with O_NONBLOCK: " << fd);
    return true;
}
} // namespace

class file_trigger_linux : public file_trigger
{
    const FileIntRaii _hndl;
    const data_cb _cb;
    const bool _sow{ true };
    temp_file _sync_wo;
    FileIntRaii _sync_ro;
    std::vector<char> _buffer;

    std::atomic_bool _running{ true };
    std::thread _runner;

    bool _read_data(int fd, const data_cb& cb_, const bool exec_)
    {
        ssize_t size = 1;
        while (true)
        {
            size = ::read(fd, _buffer.data(), _buffer.size());
            if (size < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return true;
                }
                else if (errno == EINTR)
                {
                    continue;
                }
                NSTL2_THROW_EXCEPTION("Failed over reading handler");
            }
            else if (size == 0)
            {
                return false;
            }
            else if (exec_)
            {
                cb_(std::span<char>{ _buffer.begin(), static_cast<size_t>(size) });
            }
        }
        return true;
    }

    bool _is_exit(int fd)
    {
        bool retval = false;
        const auto content_check = [&retval](const std::span<char> content)
        {
            if (exit_value.size() <= content.size())
            {
                retval = exit_value.starts_with(std::string_view{ content.data(), content.size() });
            }
            else
            {
                retval = false;
            }
        };
        if (this->_read_data(fd, content_check, true))
        {
            return retval;
        }
        return true;
    }

    void _worker()
    {
        _read_data(static_cast<int>(_hndl), _cb, _sow);

        const FileIntRaii epfd{ ::epoll_create1(EPOLL_CLOEXEC) };
        // register the target file:
        epoll_event ev_target{};
        ev_target.events = EPOLLIN;
        ev_target.data.fd = static_cast<int>(_hndl);
        NSTL2_THROW_EXCEPTION_IF(
            ::epoll_ctl(static_cast<int>(epfd), EPOLL_CTL_ADD, static_cast<int>(_hndl), &ev_target) < 0,
            "epoll_ctl failed (target)");
        // register exit file:
        epoll_event ev_exit{};
        ev_exit.events = EPOLLIN;
        ev_exit.data.fd = static_cast<int>(_sync_ro);
        NSTL2_THROW_EXCEPTION_IF(
            ::epoll_ctl(static_cast<int>(epfd), EPOLL_CTL_ADD, static_cast<int>(_sync_ro), &ev_exit) < 0,
            "epoll_ctl failed (target)");

        constexpr int event_size = 16;
        std::array<epoll_event, event_size> events;

        _buffer.resize(1024 * 1024);

        while (true)
        {
            const int n = epoll_wait(static_cast<int>(epfd), events.data(), event_size, -1);
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                NSTL2_THROW_EXCEPTION("epoll_wait error n = " << n);
            }
            for (int idx = 0; idx < n; ++idx)
            {
                const int tgt_fd = events[idx].data.fd;
                if (tgt_fd == _hndl)
                {
                    if (!this->_read_data(tgt_fd, _cb, true))
                    {
                        return;
                    }
                }
                else if (tgt_fd == _sync_ro && this->_is_exit(tgt_fd))
                {
                    return;
                }
            }
        }
    }

public:
    file_trigger_linux(FileIntRaii handle_, data_cb cb_, const bool sow_)
        : _hndl{ std::move(handle_) }, _cb{ std::move(cb_) }, _sow{ sow_ }
    {
        NSTL2_THROW_EXCEPTION_IF(!_sync_wo.valid(), "Sync file cannot be created");
        _sync_ro.reset(::open(_sync_wo.path().c_str(), O_RDONLY, O_NONBLOCK));
        NSTL2_THROW_EXCEPTION_IF(!_sync_ro, "Sync file cannot be opened for read");
        _runner = std::thread{ &file_trigger_linux::_worker, this };
    }

    ~file_trigger_linux() override { this->stop(); }

    void stop() override
    {
        if (_running.exchange(false))
        {
            const auto written = ::write(static_cast<int>(_sync_wo), exit_value.data(), exit_value.size());
            NSTL2_THROW_EXCEPTION_IF(written < 0 || static_cast<size_t>(written) != exit_value.size(),
                                     "Failed to write file");
            _runner.join();
        }
    }
};

std::shared_ptr<file_trigger> file_trigger::factory(const std::filesystem::path& file_, data_cb cb_, const bool sow_)
{
    FileIntRaii hndl{ ::open(file_.c_str(), O_RDONLY, O_NONBLOCK) };
    NSTL2_THROW_EXCEPTION_IF(!hndl, file_ << " cannot be opened for read");
    return std::make_shared<file_trigger_linux>(std::move(hndl), std::move(cb_), sow_);
}

std::shared_ptr<file_trigger> file_trigger::factory(native_handle handle_, data_cb cb_, const bool sow_)
{
    FileIntRaii hndl{ handle_ };
    NSTL2_THROW_EXCEPTION_IF(!hndl, "Invalid handle passed through");
    set_non_blocking(static_cast<int>(hndl));
    return std::make_shared<file_trigger_linux>(std::move(hndl), std::move(cb_), sow_);
}

file_trigger::file_trigger() = default;
file_trigger::~file_trigger() = default;
} // namespace nstl
