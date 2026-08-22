#include "file_trigger.hpp"
#include "exception.hpp"
#include "handle_raii.hpp"

#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <climits>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace nstl
{

namespace
{
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
    const size_t _buffer_size{0};
    FileIntRaii _inotify_fd;
    FileIntRaii _exit_read;
    FileIntRaii _exit_write;
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

    void _drain_inotify()
    {
        std::array<char, sizeof(inotify_event) + NAME_MAX + 1> buf;
        while (::read(static_cast<int>(_inotify_fd), buf.data(), buf.size()) > 0)
        {
        }
    }

    void _worker()
    {
        const FileIntRaii epfd{ ::epoll_create1(EPOLL_CLOEXEC) };
        // register the inotify fd watching the target file:
        epoll_event ev_target{};
        ev_target.events = EPOLLIN;
        ev_target.data.fd = static_cast<int>(_inotify_fd);
        NSTL2_THROW_EXCEPTION_IF(
            ::epoll_ctl(static_cast<int>(epfd), EPOLL_CTL_ADD, static_cast<int>(_inotify_fd), &ev_target) < 0,
            "epoll_ctl failed (target)");
        // register exit pipe:
        epoll_event ev_exit{};
        ev_exit.events = EPOLLIN;
        ev_exit.data.fd = static_cast<int>(_exit_read);
        NSTL2_THROW_EXCEPTION_IF(
            ::epoll_ctl(static_cast<int>(epfd), EPOLL_CTL_ADD, static_cast<int>(_exit_read), &ev_exit) < 0,
            "epoll_ctl failed (exit)");

        constexpr int event_size = 16;
        std::array<epoll_event, event_size> events;

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
            bool exit_requested = false;
            for (int idx = 0; idx < n; ++idx)
            {
                const int tgt_fd = events[idx].data.fd;
                if (tgt_fd == _inotify_fd)
                {
                    this->_drain_inotify();
                    this->_read_data(static_cast<int>(_hndl), _cb, true);
                }
                else if (tgt_fd == _exit_read)
                {
                    exit_requested = true;
                }
            }
            if (exit_requested)
            {
                return;
            }
        }
    }

public:
    file_trigger_linux(FileIntRaii handle_, data_cb cb_, const bool sow_, const size_t buffer_size_)
        : _hndl{ std::move(handle_) }, _cb{ std::move(cb_) }, _sow{ sow_ }, _buffer_size{ buffer_size_ }
    {
        _inotify_fd.reset(::inotify_init1(IN_NONBLOCK | IN_CLOEXEC));
        NSTL2_THROW_EXCEPTION_IF(!_inotify_fd, "inotify_init1 failed");

        const std::string proc_path = "/proc/self/fd/" + std::to_string(static_cast<int>(_hndl));
        NSTL2_THROW_EXCEPTION_IF(::inotify_add_watch(static_cast<int>(_inotify_fd), proc_path.c_str(), IN_MODIFY) < 0,
                                 "inotify_add_watch failed");

        _buffer.resize(_buffer_size);
        _read_data(static_cast<int>(_hndl), _cb, _sow);

        std::array<int, 2> exit_fds{ -1, -1 };
        NSTL2_THROW_EXCEPTION_IF(::pipe2(exit_fds.data(), O_NONBLOCK | O_CLOEXEC) < 0, "Exit pipe cannot be created");
        _exit_read.reset(exit_fds[0]);
        _exit_write.reset(exit_fds[1]);

        _runner = std::thread{ &file_trigger_linux::_worker, this };
    }

    ~file_trigger_linux() override { this->stop(); }

    void stop() override
    {
        if (_running.exchange(false))
        {
            constexpr char exit_signal = 1;
            const auto written = ::write(static_cast<int>(_exit_write), &exit_signal, sizeof(exit_signal));
            NSTL2_THROW_EXCEPTION_IF(written < 0 || static_cast<size_t>(written) != sizeof(exit_signal),
                                     "Failed to write file");
            _runner.join();
        }
    }
};

std::shared_ptr<file_trigger> file_trigger::factory(const std::filesystem::path& file_, data_cb cb_, const bool sow_, const size_t buffer_size_)
{
    FileIntRaii hndl{ open_native(file_) };
    NSTL2_THROW_EXCEPTION_IF(!hndl, file_ << " cannot be opened for read");
    NSTL2_THROW_EXCEPTION_IF(buffer_size_ == 0, "Buffer must not be empty");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "Callback must be valid");
    return std::make_shared<file_trigger_linux>(std::move(hndl), std::move(cb_), sow_, buffer_size_);
}

std::shared_ptr<file_trigger> file_trigger::factory(native_handle handle_, data_cb cb_, const bool sow_, const size_t buffer_size_)
{
    FileIntRaii hndl{ handle_ };
    NSTL2_THROW_EXCEPTION_IF(!hndl, "Invalid handle passed through");
    NSTL2_THROW_EXCEPTION_IF(buffer_size_ == 0, "Buffer must not be empty");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "Callback must be valid");
    set_non_blocking(static_cast<int>(hndl));
    return std::make_shared<file_trigger_linux>(std::move(hndl), std::move(cb_), sow_, buffer_size_);
}

file_trigger::file_trigger() = default;
file_trigger::~file_trigger() = default;

nstl::file_trigger::native_handle open_native(const std::filesystem::path& path_)
{
    return ::open(path_.c_str(), O_RDONLY | O_NONBLOCK);
}
} // namespace nstl
