#include "global_init.hpp"
#include "exception.hpp"
#include "backtrace.hpp"

#include <atomic>

#include <curl/curl.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/signalfd.h>
#include <signal.h>
#endif

namespace nstl
{
namespace
{
std::atomic_bool init_done{false};
std::atomic_int sfd_global{-1};

#ifdef __linux__
int openSignalFile(const bool signal_init_)
{
    if (!signal_init_)
    {
        return -1;
    }
    sigset_t mask, orig;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, SIGINT);
    ::sigaddset(&mask, SIGTERM);
    ::sigaddset(&mask, SIGQUIT);
    NSTL2_THROW_EXCEPTION_IF(::sigprocmask(SIG_BLOCK, &mask, &orig) < 0, "sigprocmask failed");
    const int sfd = ::signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    NSTL2_THROW_EXCEPTION_IF(sfd < 0, "signalfd failed");
    return sfd;
}
#else
int openSignalFile(bool /* signal_init_ */) { return -1; }
#endif
} // namespace

int global_init::getSignalFile() { return sfd_global; }

global_init::global_init(bool signal_init_, const bool curl_init_) : _curl_init{ curl_init_ }
{
    NSTL2_THROW_EXCEPTION_IF(init_done.exchange(true), "global_init did run once");

    sfd_global.store(openSignalFile(signal_init_));

    bt::backtrace_init();

    if (this->_curl_init)
    {
        const auto result = ::curl_global_init(CURL_GLOBAL_DEFAULT);
        NSTL2_THROW_EXCEPTION_IF(result != CURLE_OK, "curl_global_init failed: " << ::curl_easy_strerror(result));
    }
}

global_init::~global_init()
{
    if (this->_curl_init)
    {
        ::curl_global_cleanup();
    }

#ifdef __linux__
    if (const int sfd = sfd_global.exchange(-1); 0 <= sfd)
    {
        ::close(sfd);
    }
#endif
}
} // namespace nstl
