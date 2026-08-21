#include "file_trigger.hpp"
#include "exception.hpp"
#include "handle_raii.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

namespace nstl
{
namespace
{
std::uint64_t get_file_offset(const OVERLAPPED& ov_, std::uint64_t& base_)
{
    std::uint64_t offset = ov_.OffsetHigh;
    offset <<= 32;
    offset += ov_.Offset;
    NSTL2_THROW_EXCEPTION_IF(offset < base_, "we must not move back by ReadFile");
    auto bytes_read = offset - base_;
    base_ = offset;
    return bytes_read;
}
}

class file_trigger_win32 : public file_trigger
{
    const HandleRaii _file;
    const HandleRaii _exit_event;
    const data_cb _cb;
    const bool _sow{false};

    std::atomic_bool _running{true};
    std::thread _runner;

    void _worker() const
    {
        const HandleRaii file_event{ ::CreateEvent(NULL, TRUE, FALSE, NULL) };
        NSTL2_THROW_EXCEPTION_IF(!file_event, "File event cannot be created");

        bool sow_in_progress = true;
        std::vector<char> buffer;
        buffer.resize(1024 * 1024);

        std::array<HANDLE, 2> handles{
            static_cast<HANDLE>(file_event), static_cast<HANDLE>(_exit_event)
        };

        OVERLAPPED ov;
        std::memset(&ov, 0, sizeof(OVERLAPPED));
        ov.hEvent = static_cast<HANDLE>(file_event);

        std::uint64_t file_offset = 0;

        while (true)
        {
            if (::ReadFile(static_cast<HANDLE>(_file), buffer.data(), static_cast<DWORD>(buffer.size()), NULL, &ov))
            {
                const auto data_read = get_file_offset(ov, file_offset);

                if (!sow_in_progress || _sow)
                {
                    _cb(std::span<char>{ buffer.data(), buffer.data() + data_read });
                }
            }
            else
            {
                const DWORD err_val = GetLastError();
                if (err_val != ERROR_IO_PENDING)
                {
                    NSTL2_THROW_EXCEPTION_IF(err_val != ERROR_HANDLE_EOF, "ReadFile failed: " << err_val);
                    // TODO: EOF
                }

            }

            const auto response = ::WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
            if (response == WAIT_OBJECT_0)
            {
                ::ResetEvent(static_cast<HANDLE>(file_event));
                continue;
            }
            else if (response == WAIT_OBJECT_0 + 1)
            {
                return;
            }
            NSTL2_THROW_EXCEPTION_IF(response == WAIT_TIMEOUT, "WAIT_TIMEOUT doesn't make any sense: INFINITE timeout was set");
            NSTL2_THROW_EXCEPTION_IF(WAIT_ABANDONED_0 < response && response < WAIT_ABANDONED_0 + handles.size(),
                                        response - WAIT_ABANDONED_0 << " handle abandoned");
            NSTL2_THROW_EXCEPTION_IF(response == WAIT_FAILED, "WaitForMultipleObjects failed: " << GetLastError());
            NSTL2_THROW_EXCEPTION(response << " unexpected output from WaitForMultipleObjects");
        }
    }

public:
    file_trigger_win32(HandleRaii hndl_, data_cb cb_, bool const sow_)
        : _file{ std::move(hndl_) },
          _exit_event{ ::CreateEvent(NULL, TRUE, FALSE, NULL) }, _cb{ std::move(cb_) }, _sow{sow_}
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


std::shared_ptr<file_trigger> file_trigger::factory(const std::filesystem::path& file_, data_cb cb_, const bool sow_)
{
    HandleRaii handler{::CreateFileW(file_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL)};
    NSTL2_THROW_EXCEPTION_IF(!handler, file_ << " cannot be opened to read");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "invalid callback");
    return std::make_shared<file_trigger_win32>(std::move(handler), std::move(cb_), sow_);
}

std::shared_ptr<file_trigger> file_trigger::factory(file_trigger::native_handle hndl_, data_cb cb_, const bool sow_)
{
    HandleRaii handler{hndl_};
    NSTL2_THROW_EXCEPTION_IF(!handler, "Invalid handler set");
    NSTL2_THROW_EXCEPTION_IF(!cb_, "invalid callback");
    return std::make_shared<file_trigger_win32>(std::move(handler), std::move(cb_), sow_);
}

file_trigger::file_trigger() = default;
file_trigger::~file_trigger() = default;
}
