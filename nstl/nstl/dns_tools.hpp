#ifndef _NSTL_DNS_TOOLS
#define _NSTL_DNS_TOOLS 1

#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace nstl::net
{
	std::string hostname();
	std::optional<std::string> cannonical_name(const char* name_);

	struct mx_srv
	{
		std::string address;
		std::uint16_t priority{ 0 };
	};

	std::optional<std::vector<mx_srv>> mx_name(const char* name_);
	std::optional<std::vector<std::string>> txt_name(const char* name_);
	std::optional<std::vector<std::string>> c_name(const char* name_);
}

#endif
