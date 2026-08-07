#include <nstl/parse.hpp>
#include <nstl/macros.hpp>
#include <nstl/scope_exit.hpp>

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <sys/signalfd.h>
#include <sys/epoll.h>
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

    sigset_t mask, orig;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, SIGCHLD);
    NSTL_THROW_EXCEPTION_IF(::sigprocmask(SIG_BLOCK, &mask, &orig) < 0, std::runtime_error, "sigprocmask failed");

    const int sfd = ::signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    NSTL_THROW_EXCEPTION_IF(sfd < 0, std::runtime_error, "signalfd failed");
    const auto sfd_cleanup = nstl::on_scope_exit([sfd]() { ::close(sfd); });

    const int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    NSTL_THROW_EXCEPTION_IF(epfd < 0, std::runtime_error, "epoll_create1 failed");
    const auto epfd_cleanup = nstl::on_scope_exit([epfd]() { ::close(epfd); });

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    NSTL_THROW_EXCEPTION_IF(::epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev) < 0, std::runtime_error, "epoll_ctl failed");

    auto proc_variant = start_processes(argc_ - 2, argv_ + 2, envv_, threads);
    if (const auto int_ptr = std::get_if<int>(&proc_variant))
    {
        return *int_ptr;
    }
    auto& pids = std::get<std::vector<pid_t>>(proc_variant);

    constexpr int event_size = 16;
    std::array<epoll_event, event_size> events;

    bool raised = false;

    while (!pids.empty())
    {
        const int n = epoll_wait(epfd, events.data(), event_size, -1);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            NSTL_THROW_EXCEPTION(std::runtime_error, "epoll_wait error n = " << n);
        }
        for (int idx = 0; idx < n && !pids.empty(); ++idx)
        {
            // event not in scope
            if (events[idx].data.fd != sfd)
            {
                continue;
            }
            // Drain the signalfd. SIGCHLD coalesces, so the number of
            // records here says nothing about how many children exited.
            signalfd_siginfo ssi;
            constexpr size_t ssi_size = sizeof(ssi);
            while (::read(sfd, &ssi, ssi_size) == ssi_size)
            {
            }

            // Reap every child that's now waitable
            while (true)
            {
                int status = 0;
                const auto pid = ::waitpid(-1, &status, WNOHANG);
                if (0 < pid)
                {
                    const auto pid_itr = std::ranges::find(pids, pid);
                    if (pid_itr == pids.end())
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
                            if (item_pid != pid)
                            {
                                ::kill(item_pid, SIGTERM);
                            }
                        }
                        raised = true;
                    }
                }
                else if (pid == 0)
                {
                    break;
                }
                else if (errno != EINTR)
                {
                    break;
                }
            }
        }
    }

    return exit_code;
}