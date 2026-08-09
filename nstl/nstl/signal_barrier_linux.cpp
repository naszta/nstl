#include "signal_barrier.hpp"
#include "global_init.hpp"
#include "exception.hpp"
#include "scope_exit.hpp"

#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>


constexpr int max_events = 16;

namespace nstl
{
SignalBarrier::SignalBarrier(std::chrono::nanoseconds /* period_ */)
    : _sfd{ global_init::getSignalFile() }
{
    NSTL2_THROW_EXCEPTION_IF(_sfd < 0,
                             "signal file is not open: global_init should be initialized with signal enabled");
}

SignalBarrier::~SignalBarrier() = default;

namespace
{
void epoll_add(int epfd, int fd)
{
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    NSTL2_THROW_EXCEPTION_IF(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add failed");
}

void epoll_remove(int epfd, int fd)
{
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(epoll_event));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    NSTL2_THROW_EXCEPTION_IF(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev) == -1, "epoll add failed");
}

int create_timer_file(const std::chrono::nanoseconds& to_)
{
    const int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    NSTL2_THROW_EXCEPTION_IF(tfd == -1, "timerfd_create");

    struct itimerspec its;
    const auto seconds = std::chrono::floor<std::chrono::seconds>(to_);
    const auto nanos = to_ - seconds;

    its.it_value.tv_sec = seconds.count();
    its.it_value.tv_nsec = nanos.count();
    its.it_interval.tv_sec = seconds.count();
    its.it_interval.tv_nsec = nanos.count();

    NSTL2_THROW_EXCEPTION_IF(timerfd_settime(tfd, 0, &its, NULL) == -1, "timerfd_settime failed");

    return tfd;
}

int read_signal(int fd)
{
    struct signalfd_siginfo si;
    while (true)
    {
        const auto size = ::read(fd, &si, sizeof(si));
        if (size == -1)
        {
            if (errno == EAGAIN)
            {
                return 0;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            NSTL2_THROW_EXCEPTION("read(signalfd) failed");
        }
        NSTL2_THROW_EXCEPTION_IF(size != sizeof(si), "short read on signalfd: " << size << " bytes");
        switch (si.ssi_signo)
        {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            return si.ssi_signo;
        default:
            continue; // should be dead code, waiting for the next signal anyway
        }
    }
}

std::uint64_t read_timer(int fd)
{
    while (true)
    {
        std::uint64_t expirations = 0;
        const auto size = ::read(fd, &expirations, sizeof(expirations));
        if (size == -1)
        {
            if (errno == EAGAIN)
            {
                return 0;
            }
            if (errno == EINTR)
            {
                continue;
            }
            NSTL2_THROW_EXCEPTION("read(timer) failed");
        }
        NSTL2_THROW_EXCEPTION_IF(size != sizeof(expirations), "short read on timerfd: " << size << " bytes");
        return expirations;
    }
}
} // namespace

SigVal SignalBarrier::wait()
{
    const int epfd = epoll_create1(EPOLL_CLOEXEC);
    NSTL2_THROW_EXCEPTION_IF(epfd == -1, "epoll_create1 failed");
    const auto epfd_clean = on_scope_exit([epfd]() { ::close(epfd); });

    epoll_add(epfd, _sfd);
    const auto sfd_remove = on_scope_exit([this, epfd](){
        epoll_remove(epfd, _sfd);
    });

    struct epoll_event events[max_events];

    while (true)
    {
        const int ready = epoll_wait(epfd, events, max_events, -1);
        if (ready == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            NSTL2_THROW_EXCEPTION("epoll_wait failed");
        }
        for (int idx = 0; idx < ready; ++idx)
        {
            if (events[idx].data.fd == _sfd)
            {
                if (auto retval = read_signal(_sfd); 0 < retval)
                {
                    return from_signal_conv(retval);
                }
            }
        }
    }
}

std::optional<SigVal> SignalBarrier::wait_for(const std::chrono::nanoseconds& to_)
{
    NSTL2_THROW_EXCEPTION_IF(to_ <= std::chrono::nanoseconds::zero(), "timeout must be greater than zero");

    const auto timer_fd = create_timer_file(to_);
    const auto timer_fd_clean = on_scope_exit([timer_fd]() { ::close(timer_fd); });

    const int epfd = epoll_create1(EPOLL_CLOEXEC);
    NSTL2_THROW_EXCEPTION_IF(epfd == -1, "epoll_create1 failed");
    const auto epfd_clean = on_scope_exit([epfd]() { ::close(epfd); });

    epoll_add(epfd, _sfd);
    const auto sfd_remove = on_scope_exit([this, epfd](){
        epoll_remove(epfd, _sfd);
    });
    epoll_add(epfd, timer_fd);
    const auto timer_fd_remove = on_scope_exit([timer_fd, epfd](){
        epoll_remove(epfd, timer_fd);
    });

    struct epoll_event events[max_events];

    while (true)
    {
        const int ready = epoll_wait(epfd, events, max_events, -1);
        if (ready == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            NSTL2_THROW_EXCEPTION("epoll_wait failed");
        }
        for (int idx = 0; idx < ready; ++idx)
        {
            const auto fd = events[idx].data.fd;

            if (fd == _sfd)
            {
                if (auto retval = read_signal(fd); 0 < retval)
                {
                    return from_signal_conv(retval);
                }
            }
            else if (fd == timer_fd)
            {
                if (0 < read_timer(fd))
                {
                    return std::nullopt;
                }
            }
        }
    }
}
} // namespace nstl
