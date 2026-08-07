#include <nstl/parse.hpp>
#include <nstl/macros.hpp>
#include <nstl/scope_exit.hpp>

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <Windows.h>

#include <algorithm>
#include <format>
#include <memory>
#include <vector>

namespace
{
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

void opt_close_handle(HANDLE handle_)
{
    if (handle_)
    {
        ::CloseHandle(handle_);
    }
}
} // namespace

int main(int argc_, char** argv_, char** envv_)
{
    NSTL_THROW_EXCEPTION_IF(argc_ < 3, std::invalid_argument, "At least 3 args expected");
    const auto threads = nstl::parse_view<std::uint32_t>(argv_[1]);
    NSTL_THROW_EXCEPTION_IF(threads == 0, std::runtime_error, "at least one thread must be set");

    int exit_code = EXIT_SUCCESS;

    std::vector<HANDLE> pids, tids;
    pids.reserve(threads);
    tids.reserve(threads);

    const auto cleanup = nstl::on_scope_exit(
        [&tids, &pids]()
        {
            std::ranges::for_each(pids, opt_close_handle);
            std::ranges::for_each(tids, opt_close_handle);
        });

    for (std::uint32_t idx = 0; idx < threads; ++idx)
    {
        const auto cmdline = command_line(argc_ - 2, argv_ + 2);
        const auto envvars = env_vars(envv_, idx, threads);

        STARTUPINFOA sa_info;
        std::memset(&sa_info, 0, sizeof(STARTUPINFOA));
        PROCESS_INFORMATION pi;
        std::memset(&pi, 0, sizeof(PROCESS_INFORMATION));
        sa_info.cb = sizeof(STARTUPINFOA);

        if (::CreateProcessA(nullptr, cmdline.get(), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, envvars.get(),
                             nullptr, &sa_info, &pi))
        {
            tids.push_back(pi.hThread);
            pids.push_back(pi.hProcess);
        }
    }

    while (!pids.empty())
    {
        const auto retval = ::WaitForMultipleObjects(static_cast<DWORD>(pids.size()), pids.data(), FALSE, INFINITE);
        if (WAIT_OBJECT_0 <= retval && retval < WAIT_OBJECT_0 + pids.size())
        {
            const auto current_idx = retval - WAIT_OBJECT_0;

            const auto pid_itr = pids.begin() + current_idx;
            const auto tid_itr = tids.begin() + current_idx;

            DWORD current_exit_code = EXIT_FAILURE;
            ::GetExitCodeProcess(*pid_itr, &current_exit_code);

            ::CloseHandle(std::exchange(*pid_itr, nullptr));
            ::CloseHandle(std::exchange(*tid_itr, nullptr));

            pids.erase(pid_itr);
            tids.erase(tid_itr);

            if (current_exit_code != EXIT_SUCCESS)
            {
                exit_code = current_exit_code;
                for (auto pitr = pids.begin(), titr = tids.begin(); pitr != pids.end() && titr != tids.end();
                     ++pitr, ++titr)
                {

                    ::TerminateProcess(*pitr, exit_code);
                    ::CloseHandle(std::exchange(*pitr, nullptr));
                    ::CloseHandle(std::exchange(*titr, nullptr));
                }

                tids.clear();
                pids.clear();
            }
        }
        else
        {
            NSTL_THROW_EXCEPTION_IF(WAIT_ABANDONED_0 <= retval && retval < WAIT_ABANDONED_0 + pids.size(),
                                    std::runtime_error, "Process abandoned (not expected at all)");
            NSTL_THROW_EXCEPTION_IF(retval == WAIT_TIMEOUT, std::runtime_error,
                                    "Process wait timed out (no much sense as we wait INFINITE)");
            NSTL_THROW_EXCEPTION_IF(retval == WAIT_FAILED, std::runtime_error, "WaitForMultipleObjects FAILED");
            NSTL_THROW_EXCEPTION(std::runtime_error, retval << " return value doesn't make a sense");
        }
    }
    return exit_code;
}
