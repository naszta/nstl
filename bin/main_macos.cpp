#include <nstl/parse.hpp>
#include <nstl/macros.hpp>
#include <nstl/scope_exit.hpp>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cerrno>

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <sys/event.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

namespace
{
std::variant<std::vector<pid_t>, int> start_processes(int argc_, char** argv_, char** envv_,
                                                      const std::uint32_t threads_)
{
    const auto env_var_big = std::format("NSTL_PROCESSES_NUMBER={}", threads_);

    std::vector<char*> envvars;
    for (auto ptr = envv_; *ptr != nullptr; ++ptr)
    {
        envvars.push_back(*ptr);
    }
    envvars.push_back(const_cast<char*>(env_var_big.c_str()));

    std::vector<pid_t> pids;
    pids.reserve(threads_);

    for (std::uint32_t idx = 0; idx < threads_; ++idx)
    {
        const auto pid = ::fork();
        NSTL_THROW_EXCEPTION_IF(pid < 0, std::runtime_error, "::fork failed");
        // new child app
        if (pid == 0)
        {
            std::vector<char*> args;
            args.reserve(argc_ + 1);
            std::copy(argv_, argv_ + argc_, std::back_inserter(args));
            args.push_back(nullptr);

            const auto env_var_idx = std::format("NSTL_PROCESSES_ID={}", idx);
            envvars.push_back(const_cast<char*>(env_var_idx.c_str()));
            envvars.push_back(nullptr);
            return ::execve(args[0], args.data(), envvars.data());
        }
        else
        {
            pids.push_back(pid);
        }
    }

    return pids;
}
} // namespace

int main(int argc_, char** argv_, char** envv_)
{
    NSTL_THROW_EXCEPTION_IF(argc_ < 3, std::invalid_argument, "At least 3 args expected");
    const auto threads = nstl::parse_view<std::uint32_t>(argv_[1]);
    NSTL_THROW_EXCEPTION_IF(threads == 0, std::runtime_error, "at least one thread must be set");

    int exit_code = EXIT_SUCCESS;

    const int kq = ::kqueue();
    NSTL_THROW_EXCEPTION_IF(kq < 0, std::runtime_error, "kqueue failed");
    const auto kq_cleanup = nstl::on_scope_exit([kq]() { ::close(kq); });

    NSTL_THROW_EXCEPTION_IF(::fcntl(kq, F_SETFD, FD_CLOEXEC) < 0, std::runtime_error, "fcntl FD_CLOEXEC failed");

    auto proc_variant = start_processes(argc_ - 2, argv_ + 2, envv_, threads);
    if (const auto int_ptr = std::get_if<int>(&proc_variant))
    {
        return *int_ptr;
    }
    auto& pids = std::get<std::vector<pid_t>>(proc_variant);

    // Register a NOTE_EXIT watch for every child. Because no child has been
    // reaped yet, each PID is guaranteed to still exist (running or zombie),
    // so EV_ADD cannot fail with ESRCH. NOTE_EXIT auto-deletes its knote when
    // the process exits, so no manual EV_DELETE is required.
    {
        std::vector<struct kevent> changes;
        changes.reserve(pids.size());
        for (const auto pid : pids)
        {
            struct kevent kev;
            EV_SET(&kev, static_cast<uintptr_t>(pid), EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, nullptr);
            changes.push_back(kev);
        }
        NSTL_THROW_EXCEPTION_IF(
            ::kevent(kq, changes.data(), static_cast<int>(changes.size()), nullptr, 0, nullptr) < 0,
            std::runtime_error, "kevent registration failed");
    }

    constexpr int event_size = 16;
    std::array<struct kevent, event_size> events;

    bool raised = false;

    while (!pids.empty())
    {
        const int n = ::kevent(kq, nullptr, 0, events.data(), event_size, nullptr);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            NSTL_THROW_EXCEPTION(std::runtime_error, "kevent error n = " << n);
        }
        for (int idx = 0; idx < n && !pids.empty(); ++idx)
        {
            const auto& e = events[idx];
            // Only a process filter reporting NOTE_EXIT is of interest here.
            if (e.filter != EVFILT_PROC || (e.fflags & NOTE_EXIT) == 0)
            {
                continue;
            }

            const auto pid = static_cast<pid_t>(e.ident);
            const auto pid_itr = std::ranges::find(pids, pid);
            if (pid_itr == pids.end())
            {
                continue;
            }

            int status = 0;
            if (::waitpid(pid, &status, 0) != pid)
            {
                continue;
            }
            pids.erase(pid_itr);

            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            if (code != EXIT_SUCCESS && !raised)
            {
                exit_code = code;
                for (const auto item_pid : pids)
                {
                    ::kill(item_pid, SIGTERM);
                }
                raised = true;
            }
        }
    }

    return exit_code;
}
