#include "nstl/dns_tools.hpp"
#include "nstl/macros.hpp"
#include "nstl/scope_exit.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <cstring>
#include <array>
#include <stdexcept>

namespace nstl::net
{
namespace
{
#ifdef _WIN32
	struct W32Init
	{
		W32Init()
		{
			WSADATA wsaData;
			NSTL_THROW_EXCEPTION_IF(::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0, std::runtime_error, "TCP/IP init failed");
		}
		~W32Init()
		{
			::WSACleanup();
		}
	};

	void net_init()
	{
		static W32Init instance;
	}
#else
	void net_init()
	{}
#endif
}

std::string hostname()
{
	// https://man7.org/linux/man-pages/man2/gethostname.2.html - SUSv2 guarantees that "Host names are limited to 255 bytes".
	// https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-gethostname - So if a buffer of 256 bytes is passed in the name parameter and the namelen parameter is set to 256, the buffer size will always be adequate.
	constexpr int max_host_size = 256;
	net_init();
	std::array<char, max_host_size> buffer;
	NSTL_THROW_EXCEPTION_IF(::gethostname(buffer.data(), max_host_size) != 0, std::invalid_argument,  "hostname cannot be resolved");
	return std::string{buffer.data()};
}

std::optional<std::string> cannonical_name(const char* name_)
{
	net_init();
	NSTL_THROW_EXCEPTION_IF(!name_, std::invalid_argument, "name_ cannot be nullptr");
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_CANONNAME;
	addrinfo* result = nullptr;
	const auto cleanup = on_scope_exit([&result]() {
		if (result) {
			::freeaddrinfo(result);
		}
	});

	NSTL_THROW_EXCEPTION_IF(::getaddrinfo(name_, nullptr, &hints, &result) != 0, std::runtime_error, name_ << " cannot be resolved");

	std::optional<std::string> retval;

	for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_canonname) {
			retval.emplace(ptr->ai_canonname);
			return retval;
		}
	}
	return retval;
}
}
