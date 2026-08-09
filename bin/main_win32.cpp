#include <nstl/parse.hpp>
#include <nstl/macros.hpp>
#include <nstl/scope_exit.hpp>

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <Windows.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_map>

namespace
{
constexpr std::uint32_t running_true = std::numeric_limits<std::uint32_t>::max();
std::atomic_uint32_t app_running{ running_true };

// Windows style signal handling
BOOL WINAPI ConsoleHandler(DWORD signal)
{
    switch (signal)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        app_running.store(signal);
        return TRUE;
    default:
        return FALSE;
    }
}

struct free_deleter
{
    void operator()(void* ptr_) const { std::free(ptr_); }
};

std::unique_ptr<char, free_deleter> command_line(int argc_, char** argv_)
{
    std::ostringstream oss;
    oss << argv_[0];
    for (int i = 1; i < argc_; ++i)
    {
        oss << ' ' << argv_[i];
    }
    const auto line_view = oss.view();
    char* ptr = reinterpret_cast<char*>(std::malloc(line_view.size() + 1));
    if (ptr == nullptr) [[unlikely]]
    {
        throw std::bad_alloc{};
    }
    std::unique_ptr<char, free_deleter> retval{ std::exchange(ptr, nullptr) };
    std::strncpy(retval.get(), line_view.data(), line_view.size());
    *(retval.get() + line_view.size()) = '\0';
    return retval;
}

size_t secure_strlen(const char* ptr_) { return ptr_ ? std::strlen(ptr_) : 0; }

std::unique_ptr<void, free_deleter> env_vars(char** envv_, const std::uint32_t idx_, const std::uint32_t threads_)
{
    std::vector<char> buffer;
    buffer.reserve(1024);

    for (auto item_size = secure_strlen(*envv_); 0 < item_size; ++envv_, item_size = secure_strlen(*envv_))
    {
        std::copy(*envv_, *envv_ + item_size, std::back_inserter(buffer));
        buffer.push_back('\0');
    }

    const auto env_var_big = std::format("NSTL_PROCESSES_NUMBER={}", threads_);
    std::copy(env_var_big.cbegin(), env_var_big.cend(), std::back_inserter(buffer));
    buffer.push_back('\0');

    const auto env_var_idx = std::format("NSTL_PROCESSES_ID={}", idx_);
    std::copy(env_var_idx.cbegin(), env_var_idx.cend(), std::back_inserter(buffer));
    buffer.push_back('\0');

    buffer.push_back('\0');
    void* ptr = reinterpret_cast<char*>(std::malloc(buffer.size()));
    if (ptr == nullptr) [[unlikely]]
    {
        throw std::bad_alloc{};
    }
    std::unique_ptr<void, free_deleter> retval{ std::exchange(ptr, nullptr) };
    std::memcpy(retval.get(), buffer.data(), buffer.size());
    return retval;
}

void opt_close_handle(HANDLE& handle_)
{
    if (handle_)
    {
        ::CloseHandle(handle_);
        handle_ = nullptr;
    }
}

void CALLBACK TimerRoutine(void* data_, BOOLEAN TimerOrWaitFired);

struct Data
{
    explicit Data(std::uint32_t threads_)
    {
        pids.reserve(threads_);
        tids.reserve(threads_);
    }

    void push_back(HANDLE hpid, HANDLE htid, DWORD pid)
    {
        pids.emplace_back(hpid);
        tids.emplace_back(htid);
        handle_to_pid.emplace(hpid, pid);
        wait_for.push_back(hpid);
    }

    PHANDLE data()
    {
        return wait_for.data();
    }

    DWORD size() const { return static_cast<DWORD>(wait_for.size()); }

    ~Data()
    {
        while (pids.size() < wait_for.size())
        {
            opt_close_handle(wait_for.back());
            wait_for.pop_back();
        }
        std::ranges::for_each(pids, opt_close_handle);
        std::ranges::for_each(tids, opt_close_handle);
        opt_close_handle(timer_hndlr);
    }

    bool empty() const { return pids.empty(); }

    std::optional<DWORD> remove_idx(const DWORD idx_)
    {
        const auto hnd_itr = wait_for.begin() + idx_;
        const auto pid_itr = pids.begin() + idx_;
        // tid itr is off: it is an event
        if (pid_itr == pids.end())
        {
            wait_for.erase(hnd_itr);
            return std::nullopt;
        }

        DWORD current_exit_code = EXIT_FAILURE;
        ::GetExitCodeProcess(*pid_itr, &current_exit_code);

        handle_to_pid.erase(*pid_itr);

        const auto tid_itr = tids.begin() + idx_;

        ::CloseHandle(std::exchange(*pid_itr, nullptr));
        ::CloseHandle(std::exchange(*tid_itr, nullptr));

        pids.erase(pid_itr);
        tids.erase(tid_itr);
        wait_for.erase(hnd_itr);
        return current_exit_code;
    }

    void sendCloseEvent(DWORD signal = CTRL_CLOSE_EVENT)
    {
        for (const auto& pid : pids)
        {
            if (const auto pid_itr = handle_to_pid.find(pid); pid_itr != handle_to_pid.end())
            {
                ::GenerateConsoleCtrlEvent(signal, pid_itr->second);
            }
        }
    }

    void createEvent()
    {
        constexpr auto event_timeout = std::chrono::duration<DWORD, std::milli>{ 1000 };
        NSTL_THROW_EXCEPTION_IF(event, std::runtime_error, "Event already created!");
        NSTL_THROW_EXCEPTION_IF(timer_hndlr, std::runtime_error, "Timer already created");

        event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        NSTL_THROW_EXCEPTION_IF(!event, std::runtime_error, "CreateEvent failed");
        if (!::CreateTimerQueueTimer(&timer_hndlr, NULL, reinterpret_cast<WAITORTIMERCALLBACK>(TimerRoutine), this,
                                     event_timeout.count(), 0, 0)) [[unlikely]]
        {
            NSTL_THROW_EXCEPTION(std::runtime_error, "CreateTimerQueueTimer failed");
        }
        wait_for.push_back(event);
    }

    std::unordered_map<HANDLE, DWORD> handle_to_pid;
    HANDLE timer_hndlr{ nullptr };
    HANDLE event{ nullptr };
    std::vector<HANDLE> pids;
    std::vector<HANDLE> tids;
    std::vector<HANDLE> wait_for;
    int exit_code{ EXIT_SUCCESS };
};


void CALLBACK TimerRoutine(void* data_, BOOLEAN /* TimerOrWaitFired */)
{
    NSTL_THROW_EXCEPTION_IF(!data_, std::runtime_error, "data is nullptr");
    Data* data = reinterpret_cast<Data*>(data_);

    for (auto pid : data->pids)
    {
        ::TerminateProcess(pid, data->exit_code);
    }

    if (data->event) [[likely]]
    {
        ::SetEvent(data->event);
    }
}

} // namespace

int main(int argc_, char** argv_, char** envv_)
{
    NSTL_THROW_EXCEPTION_IF(argc_ < 3, std::invalid_argument, "At least 3 args expected");
    const auto threads = nstl::parse_view<std::uint32_t>(argv_[1]);
    NSTL_THROW_EXCEPTION_IF(threads == 0, std::runtime_error, "at least one thread must be set");
    NSTL_THROW_EXCEPTION_IF(!::SetConsoleCtrlHandler(ConsoleHandler, TRUE), std::runtime_error , "SetConsoleCtrlHandler failed");

    Data app_data{threads};

    for (std::uint32_t idx = 0; idx < threads; ++idx)
    {
        const auto cmdline = command_line(argc_ - 2, argv_ + 2);
        const auto envvars = env_vars(envv_, idx, threads);

        STARTUPINFOA sa_info;
        std::memset(&sa_info, 0, sizeof(STARTUPINFOA));
        PROCESS_INFORMATION pi;
        std::memset(&pi, 0, sizeof(PROCESS_INFORMATION));
        sa_info.cb = sizeof(STARTUPINFOA);

        if (::CreateProcessA(nullptr, cmdline.get(), nullptr, nullptr, FALSE,
                             NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP, envvars.get(),
                             nullptr, &sa_info, &pi))
        {
            app_data.push_back(pi.hProcess, pi.hThread, pi.dwProcessId);
        }
    }

    bool send_close_event = true;

    while (!app_data.empty())
    {
        const auto retval = ::WaitForMultipleObjects(app_data.size(), app_data.data(), FALSE, 300);
        if (WAIT_OBJECT_0 <= retval && retval < WAIT_OBJECT_0 + app_data.size())
        {
            if (const auto current_exit_code = app_data.remove_idx(retval - WAIT_OBJECT_0))
            {
                if (current_exit_code != EXIT_SUCCESS && std::exchange(send_close_event, false))
                {
                    app_data.exit_code = current_exit_code.value();
                    app_data.sendCloseEvent();
                }
            }
        }
        else if (retval == WAIT_TIMEOUT)
        {
            const auto run_state = app_running.load();
            if (run_state != running_true && std::exchange(send_close_event, false))
            {
                app_data.sendCloseEvent(run_state);
                app_data.createEvent();
            }
        }
        else
        {
            NSTL_THROW_EXCEPTION_IF(WAIT_ABANDONED_0 <= retval && retval < WAIT_ABANDONED_0 + app_data.size(),
                                    std::runtime_error, "Process abandoned (not expected at all)");
            NSTL_THROW_EXCEPTION_IF(retval == WAIT_FAILED, std::runtime_error, "WaitForMultipleObjects FAILED");
            NSTL_THROW_EXCEPTION(std::runtime_error, retval << " return value doesn't make a sense");
        }
    }
    return app_data.exit_code;
}
