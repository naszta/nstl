#include "file_trigger.hpp"
#include "exception.hpp"
#include "handle_raii.hpp"

#include <sys/event.h>
#include <sys/types.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cstdint>
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

void set_close_on_exec(int fd)
{
    NSTL2_THROW_EXCEPTION_IF(::fcntl(fd, F_SETFD, FD_CLOEXEC) < 0, "fcntl F_SETFD failed with FD_CLOEXEC: " << fd);
}
} // namespace

class file_trigger_apple : public file_trigger
{
    const FileIntRaii _hndl;
    const data_cb _cb;
    const bool _sow{ true };
    const size_t _buffer_size{0};
    FileIntRaii _kq;
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

    void _worker()
    {
        std::array<struct kevent, 2> changes{};
        EV_SET(&changes[0], static_cast<uintptr_t>(static_cast<int>(_hndl)), EVFILT_VNODE, EV_ADD | EV_CLEAR,
               NOTE_WRITE | NOTE_EXTEND, 0, nullptr);
        EV_SET(&changes[1], static_cast<uintptr_t>(static_cast<int>(_exit_read)), EVFILT_READ, EV_ADD, 0, 0, nullptr);
        NSTL2_THROW_EXCEPTION_IF(::kevent(static_cast<int>(_kq), changes.data(), static_cast<int>(changes.size()), nullptr, 0, nullptr) < 0,
                                 "kevent failed to register watches");

        constexpr int event_size = 16;
        std::array<struct kevent, event_size> events{};

        while (true)
        {
            const int n = ::kevent(static_cast<int>(_kq), nullptr, 0, events.data(), event_size, nullptr);
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                NSTL2_THROW_EXCEPTION("kevent wait error n = " << n);
            }
            bool exit_requested = false;
            for (int idx = 0; idx < n; ++idx)
            {
                const auto ident = static_cast<int>(events[idx].ident);
                if (ident == static_cast<int>(_hndl) && events[idx].filter == EVFILT_VNODE)
                {
                    this->_read_data(static_cast<int>(_hndl), _cb, true);
                }
                else if (ident == static_cast<int>(_exit_read))
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
    file_trigger_apple(FileIntRaii handle_, data_cb cb_, const bool sow_, const size_t buffer_size_)
        : _hndl{ std::move(handle_) }, _cb{ std::move(cb_) }, _sow{ sow_ }, _buffer_size{ buffer_size_ }
    {
        _kq.reset(::kqueue());
        NSTL2_THROW_EXCEPTION_IF(!_kq, "kqueue failed");
        set_close_on_exec(static_cast<int>(_kq));

        _buffer.resize(_buffer_size);
        _read_data(static_cast<int>(_hndl), _cb, _sow);

        std::array<int, 2> exit_fds{ -1, -1 };
        NSTL2_THROW_EXCEPTION_IF(::pipe(exit_fds.data()) < 0, "Exit pipe cannot be created");
        _exit_read.reset(exit_fds[0]);
        _exit_write.reset(exit_fds[1]);
        set_non_blocking(static_cast<int>(_exit_read));
        set_non_blocking(static_cast<int>(_exit_write));
        set_close_on_exec(static_cast<int>(_exit_read));
        set_close_on_exec(static_cast<int>(_exit_write));

        _runner = std::thread{ &file_trigger_apple::_worker, this };
    }

    ~file_trigger_apple() override { this->stop(); }

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
    return std::make_shared<file_trigger_apple>(std::move(hndl), std::move(cb_), sow_, buffer_size_);
}

std::shared_ptr<file_trigger> file_trigger::factory(native_handle handle_, data_cb cb_, const bool sow_, const size_t buffer_size_)
{
    FileIntRaii hndl{ handle_ };
    NSTL2_THROW_EXCEPTION_IF(!hndl, "Invalid handle passed through");
    NSTL2_THROW_EXCEPTION_IF(buffer_size_ == 0, "Buffer must not be empty");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "Callback must be valid");
    set_non_blocking(static_cast<int>(hndl));
    return std::make_shared<file_trigger_apple>(std::move(hndl), std::move(cb_), sow_, buffer_size_);
}

file_trigger::file_trigger() = default;
file_trigger::~file_trigger() = default;

nstl::file_trigger::native_handle open_native(const std::filesystem::path& path_)
{
    return ::open(path_.c_str(), O_RDONLY | O_NONBLOCK);
}
} // namespace nstl
