#include "file_trigger.hpp"
#include "exception.hpp"
#include "handle_raii.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

namespace nstl
{
namespace
{
// Regular disk files essentially never complete overlapped reads
// asynchronously: once we hit EOF we have no event to wait on, so we poll
// at this interval until either new data appears or stop() is requested.
constexpr std::chrono::duration<DWORD, std::milli> eof_poll_interval{100};

void set_overlapped_offset(OVERLAPPED& ov_, std::uint64_t offset_)
{
    ov_.Offset = static_cast<DWORD>(offset_ & 0xFFFFFFFFu);
    ov_.OffsetHigh = static_cast<DWORD>(offset_ >> 32);
}

std::uint64_t query_file_size(const HandleRaii& handle_)
{
    LARGE_INTEGER size{};
    NSTL2_THROW_EXCEPTION_IF(!::GetFileSizeEx(static_cast<HANDLE>(handle_), &size), "GetFileSizeEx failed: " << GetLastError());
    return static_cast<std::uint64_t>(size.QuadPart);
}
}

class file_trigger_win32 : public file_trigger
{
    const HandleRaii _file;
    const HandleRaii _exit_event;
    const data_cb _cb;
    const bool _sow{false};
    const std::uint64_t _sow_boundary{0};
    const size_t _buffer_size{0};

    std::atomic_bool _running{true};
    std::thread _runner;

    void _worker() const
    {
        const HandleRaii file_event{ ::CreateEvent(NULL, TRUE, FALSE, NULL) };
        NSTL2_THROW_EXCEPTION_IF(!file_event, "File event cannot be created");

        std::vector<char> buffer;
        buffer.resize(_buffer_size);

        const std::array<HANDLE, 2> handles{
            static_cast<HANDLE>(file_event), static_cast<HANDLE>(_exit_event)
        };

        std::uint64_t file_offset = 0;

        while (true)
        {
            OVERLAPPED ov;
            std::memset(&ov, 0, sizeof(OVERLAPPED));
            ov.hEvent = static_cast<HANDLE>(file_event);
            set_overlapped_offset(ov, file_offset);

            bool completed = false;
            bool eof = false;

            if (::ReadFile(static_cast<HANDLE>(_file), buffer.data(), static_cast<DWORD>(buffer.size()), NULL, &ov))
            {
                completed = true;
            }
            else
            {
                const DWORD err_val = GetLastError();
                if (err_val == ERROR_IO_PENDING)
                {
                    const auto response = ::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
                    if (response == WAIT_OBJECT_0 + 1)
                    {
                        // Cancel and drain the outstanding read before we tear down
                        // the buffer/event it references; otherwise closing the
                        // file handle can block on the still-pending I/O.
                        ::CancelIoEx(static_cast<HANDLE>(_file), &ov);
                        DWORD discarded = 0;
                        ::GetOverlappedResult(static_cast<HANDLE>(_file), &ov, &discarded, TRUE);
                        return;
                    }
                    NSTL2_THROW_EXCEPTION_IF(response == WAIT_TIMEOUT, "WAIT_TIMEOUT doesn't make any sense: INFINITE timeout was set");
                    NSTL2_THROW_EXCEPTION_IF(WAIT_ABANDONED_0 <= response && response < WAIT_ABANDONED_0 + handles.size(),
                                                response - WAIT_ABANDONED_0 << " handle abandoned");
                    NSTL2_THROW_EXCEPTION_IF(response == WAIT_FAILED, "WaitForMultipleObjects failed: " << GetLastError());
                    NSTL2_THROW_EXCEPTION_IF(response != WAIT_OBJECT_0, response << " unexpected output from WaitForMultipleObjects");
                    completed = true;
                }
                else
                {
                    NSTL2_THROW_EXCEPTION_IF(err_val != ERROR_HANDLE_EOF, "ReadFile failed: " << err_val);
                    eof = true;
                }
            }

            if (completed)
            {
                DWORD transferred = 0;
                if (::GetOverlappedResult(static_cast<HANDLE>(_file), &ov, &transferred, FALSE))
                {
                    const std::uint64_t chunk_start = file_offset;
                    file_offset += transferred;
                    if (transferred > 0)
                    {
                        auto deliver_from = chunk_start;
                        if (!_sow && chunk_start < _sow_boundary)
                        {
                            deliver_from = std::min(file_offset, _sow_boundary);
                        }
                        if (deliver_from < file_offset)
                        {
                            const auto skip = static_cast<size_t>(deliver_from - chunk_start);
                            const auto count = static_cast<size_t>(file_offset - deliver_from);
                            _cb(std::span<char>{ buffer.data() + skip, buffer.data() + skip + count });
                        }
                        continue; // more data may be waiting right away; keep draining
                    }
                    eof = true; // synchronous EOF is reported as a successful zero-byte read
                }
                else
                {
                    const DWORD err_val = GetLastError();
                    NSTL2_THROW_EXCEPTION_IF(err_val != ERROR_HANDLE_EOF, "GetOverlappedResult failed: " << err_val);
                    eof = true;
                }
            }

            if (eof)
            {
                const auto response = ::WaitForSingleObject(static_cast<HANDLE>(_exit_event), eof_poll_interval.count());
                if (response == WAIT_OBJECT_0)
                {
                    return;
                }
                NSTL2_THROW_EXCEPTION_IF(response == WAIT_FAILED, "WaitForSingleObject failed: " << GetLastError());
                NSTL2_THROW_EXCEPTION_IF(response != WAIT_TIMEOUT, response << " unexpected output from WaitForSingleObject");
            }
        }
    }

public:
    file_trigger_win32(HandleRaii hndl_, data_cb cb_, bool const sow_, const size_t buffer_size_, const std::uint64_t sow_boundary_)
        : _file{ std::move(hndl_) }, _exit_event{ ::CreateEvent(NULL, TRUE, FALSE, NULL) }, _cb{ std::move(cb_) },
          _sow{ sow_ }, _sow_boundary{ sow_boundary_ }, _buffer_size{ buffer_size_ }
    {
        NSTL2_THROW_EXCEPTION_IF(!_exit_event, "exit event cannot be created");
        _runner = std::thread{ &file_trigger_win32::_worker, this };
    }

    ~file_trigger_win32() override { this->stop(); }

    void stop() override
    {
        if (_running.exchange(false))
        {
            ::SetEvent(static_cast<HANDLE>(_exit_event));
            if (_runner.joinable())
            {
                _runner.join();
            }
        }
    }
};

std::shared_ptr<file_trigger> file_trigger::factory(const std::filesystem::path& file_, data_cb cb_, const bool sow_,
                                                    const size_t buffer_size_)
{
    HandleRaii handler{::CreateFileW(file_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL)};
    NSTL2_THROW_EXCEPTION_IF(!handler, file_ << " cannot be opened to read");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "invalid callback");
    NSTL2_THROW_EXCEPTION_IF(buffer_size_ == 0, "Buffer must not be empty");
    const auto sow_boundary = query_file_size(handler);
    return std::make_shared<file_trigger_win32>(std::move(handler), std::move(cb_), sow_, buffer_size_, sow_boundary);
}

std::shared_ptr<file_trigger> file_trigger::factory(file_trigger::native_handle hndl_, data_cb cb_, const bool sow_,
                                                    const size_t buffer_size_)
{
    HandleRaii handler{hndl_};
    NSTL2_THROW_EXCEPTION_IF(!handler, "Invalid handler set");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "invalid callback");
    NSTL2_THROW_EXCEPTION_IF(buffer_size_ == 0, "Buffer must not be empty");
    const auto sow_boundary = query_file_size(handler);
    return std::make_shared<file_trigger_win32>(std::move(handler), std::move(cb_), sow_, buffer_size_, sow_boundary);
}

file_trigger::file_trigger() = default;
file_trigger::~file_trigger() = default;

nstl::file_trigger::native_handle open_native(const std::filesystem::path& path_)
{
    return ::CreateFileW(path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                         OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
}
}
