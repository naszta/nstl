#include "global_init.hpp"
#include "exception.hpp"

#ifdef NSTL_USING_CURL
#include <curl/curl.h>
#elif defined(_WIN32)
#include <ws2tcpip.h>
#endif

namespace nstl
{
global_init::global_init(const bool curl_init_) : _curl_init{curl_init_}
{
#if defined(NSTL_USING_CURL)
    if (this->_curl_init)
    {
        const auto result = ::curl_global_init(CURL_GLOBAL_DEFAULT);
        NSTL2_THROW_EXCEPTION_IF(result != CURLE_OK, "curl_global_init failed: " << ::curl_easy_strerror(result));
    }
#elif defined(_WIN32)
    WSADATA wsaData;
    NSTL2_THROW_EXCEPTION_IF(::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0, "TCP/IP init failed");
#endif
}

global_init::~global_init()
{
#if defined(NSTL_USING_CURL)
    if (this->_curl_init)
    {
        ::curl_global_cleanup();
    }
#elif defined(_WIN32)
    ::WSACleanup();
#endif
}
} // namespace nstl
