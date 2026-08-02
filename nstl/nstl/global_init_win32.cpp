#include "global_init.hpp"
#include "exception.hpp"

#include <ws2tcpip.h>

namespace nstl
{
namespace
{
struct W32Init
{
    W32Init()
    {
        WSADATA wsaData;
        NSTL2_THROW_EXCEPTION_IF(::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0, "TCP/IP init failed");
    }
    ~W32Init() { ::WSACleanup(); }
};
} // namespace
void global_init() { static W32Init instance; }
} // namespace nstl
