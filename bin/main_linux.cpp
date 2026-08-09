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
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
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
struct process
{
    process()
    {
        int fds[2];
        NSTL_THROW_EXCEPTION_IF(::pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0, std::runtime_error, "Pipe cannot be opened");
        read_fd = fds[0], write_fd = fds[1];
    }
    ~process()
    {
        this->reset();
    }
    process(const process&) = delete;
    process& operator=(const process&) = delete;
    process(process&& other_) noexcept
    {
        this->swap(other_);
    }
    process& operator=(process&& other_) noexcept
    {
        if (this != &other_) {
            this->reset();
            this->swap(other_);
        }
        return *this;
    }


    void swap(process& other_)
    {
        std::swap(read_fd, other_.read_fd);
        std::swap(write_fd, other_.write_fd);
    }

    void reset()
    {
        if (0 <= read_fd)
        {
            ::close(read_fd);
            read_fd = -1;
        }
        if (0 <= write_fd)
        {
            ::close(write_fd);
            write_fd = -1;
        }
    }

    bool validPid() const
    {
        return 0 < pid;
    }

    bool valid() const
    {
        return this->validPid() || 0 <= read_fd;
    }

    void close_read(int epfd)
    {
        if (read_fd < 0)
        {
            return;
        }
        epoll_event ev_pipe{};
        ev_pipe.events = EPOLLIN | EPOLLET;
        ev_pipe.data.fd = read_fd;
        NSTL_THROW_EXCEPTION_IF(::epoll_ctl(epfd, EPOLL_CTL_DEL, read_fd, &ev_pipe), std::runtime_error, "epoll_ctl failed");
        ::close(read_fd);
        read_fd = -1;
    }

    pid_t pid{0};
    int read_fd{-1};
    int write_fd{-1};
};

std::optional<int> start_processes(int argc_, char** argv_, char** envv_, std::vector<process>& tgt_)
{
    std::vector<char*> arguments;
    arguments.reserve(argc_ + 1);
    std::copy(argv_, argv_ + argc_, std::back_inserter(arguments));
    arguments.push_back(nullptr);

    const auto env_var_big = std::format("NSTL_PROCESSES_NUMBER={}", tgt_.size());

    std::vector<char*> envvars;
    for (auto ptr = envv_; *ptr != nullptr; ++ptr)
    {
        envvars.push_back(*ptr);
    }
    envvars.push_back(const_cast<char*>(env_var_big.c_str()));

    const auto base_size = envvars.size();

    for (std::uint32_t idx = 0; idx < tgt_.size(); ++idx)
    {
        auto& p_item = tgt_[idx];

        envvars.resize(base_size);

        const auto env_var_idx = std::format("NSTL_PROCESSES_ID={}", idx);
        envvars.push_back(const_cast<char*>(env_var_idx.c_str()));
        envvars.push_back(nullptr);

        const auto pid = ::fork();
        NSTL_THROW_EXCEPTION_IF(pid < 0, std::runtime_error, "::fork failed");
        // new child app
        if (pid == 0)
        {
            NSTL_THROW_EXCEPTION_IF(dup2(p_item.write_fd, STDOUT_FILENO) < 0, std::runtime_error, "duplicate handle failed");
            if (::execve(argv_[0], arguments.data(), envvars.data()) == -1)
            {
                std::cout << errno << " error number" << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            p_item.pid = pid;
        }
    }

    return std::nullopt;
}

void read_signal(int sfd, bool& raised, int& exit_code, std::vector<process>& processes)
{
    // Drain the signalfd. SIGCHLD coalesces, so the number of
    // records here says nothing about how many children exited.
    signalfd_siginfo ssi;
    constexpr size_t ssi_size = sizeof(ssi);
    for (auto size = ::read(sfd, &ssi, ssi_size); 0 < size; size = ::read(sfd, &ssi, ssi_size))
    {
        if (size != ssi_size)
        {
            return;
        }
        switch (ssi.ssi_signo)
        {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            for (const auto& process : processes)
            {
                if (0 < process.pid)
                {
                    ::kill(process.pid, ssi.ssi_signo);
                }
            }
            raised = true;
            continue;
        case SIGCHLD:
            // Reap every child that's now waitable
            while (true)
            {
                int status = 0;
                const auto pid = ::waitpid(-1, &status, WNOHANG);
                if (0 < pid)
                {
                    const auto pid_itr = std::ranges::find_if(processes, [pid] (const process& proc){ return proc.pid == pid; });
                    if (pid_itr == processes.end())
                    {
                        continue;
                    }
                    pid_itr->pid = 0;

                    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);

                    if (code != EXIT_SUCCESS && !raised)
                    {
                        exit_code = code;
                        for (const auto& item_pid : processes)
                        {
                            if (0 < item_pid.pid)
                            {
                                ::kill(item_pid.pid, SIGTERM);
                            }
                        }
                        raised = true;
                    }
                }
                else if (pid == 0)
                {
                    break;
                }
                // from here: pid is negative
                else if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECHILD)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            break;
        default:
            continue;
        }
    }
}

void read_pipe(process& process, std::span<char> buffer_, int epfd)
{
    while (0 <= process.read_fd)
    {
        const auto read_bytes = ::read(process.read_fd, buffer_.data(), buffer_.size());
        if (0 < read_bytes)
        {
            ::fwrite(buffer_.data(), 1, read_bytes, stdout);
            continue;
        }
        else if (read_bytes == 0)
        {
            process.close_read(epfd);
            return;
        }
        else if (errno == EINTR)
        {
            continue;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        NSTL_THROW_EXCEPTION(std::runtime_error, "read(pipe) failed");
    }
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
    ::sigaddset(&mask, SIGINT);
    ::sigaddset(&mask, SIGTERM);
    ::sigaddset(&mask, SIGQUIT);
    NSTL_THROW_EXCEPTION_IF(::sigprocmask(SIG_BLOCK, &mask, &orig) < 0, std::runtime_error, "sigprocmask failed");

    const int sfd = ::signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    NSTL_THROW_EXCEPTION_IF(sfd < 0, std::runtime_error, "signalfd failed");
    const auto sfd_cleanup = nstl::on_scope_exit([sfd]() { ::close(sfd); });
    // PIPE
    std::vector<process> processes;
    processes.resize(threads);

    // start
    const auto proc_opt = start_processes(argc_ - 2, argv_ + 2, envv_, processes);
    if (proc_opt)
    {
        return proc_opt.value();
    }

    const int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    NSTL_THROW_EXCEPTION_IF(epfd < 0, std::runtime_error, "epoll_create1 failed");
    const auto epfd_cleanup = nstl::on_scope_exit([epfd]() { ::close(epfd); });

    epoll_event ev_signal{};
    ev_signal.events = EPOLLIN;
    ev_signal.data.fd = sfd;
    NSTL_THROW_EXCEPTION_IF(::epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev_signal) < 0, std::runtime_error, "epoll_ctl failed");


    std::ranges::for_each(processes, [epfd](process& p_item) {
        epoll_event ev_pipe{};
        ev_pipe.events = EPOLLIN | EPOLLET;
        ev_pipe.data.fd = p_item.read_fd;
        NSTL_THROW_EXCEPTION_IF(::epoll_ctl(epfd, EPOLL_CTL_ADD, p_item.read_fd, &ev_pipe), std::runtime_error, "epoll_ctl failed");
    });

    constexpr int event_size = 16;
    std::array<epoll_event, event_size> events;

    bool raised = false;

    std::vector<char> buffer;
    buffer.resize(1024 * 1024);

    while (std::ranges::any_of(processes, [](const process& pitem){ return pitem.validPid(); }))
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
        for (int idx = 0; idx < n; ++idx)
        {
            auto tgt_fd = events[idx].data.fd;

            if (tgt_fd == sfd)
            {
                read_signal(tgt_fd, raised, exit_code, processes);
            }
            else if (const auto pitr = std::ranges::find_if(processes, [tgt_fd](const process& p_item) { return p_item.read_fd == tgt_fd; }); pitr != processes.end())
            {
                read_pipe(*pitr, std::span{buffer}, epfd);
            }
        }
    }

    std::ranges::for_each(processes,
        [&buffer, &epfd](process& p_item){
            read_pipe(p_item, std::span{buffer}, epfd);
        });

    return exit_code;
}