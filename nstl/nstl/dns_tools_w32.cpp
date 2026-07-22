#include "nstl/dns_tools.hpp"
#include "nstl/macros.hpp"
#include "nstl/scope_exit.hpp"

#include <Windows.h>
#include <windns.h>

#include <stdexcept>

namespace nstl::net
{
std::optional<std::vector<mx_srv>> mx_name(const char* name_)
{
	THROW_EXCEPTION_IF(!name_, std::invalid_argument, "name_ cannot be nullptr");
	DNS_RECORD* results = nullptr;
	const auto cleanup = on_scope_exit([&results]() {
		if (results) {
			DnsRecordListFree(results, DnsFreeRecordListDeep);
		}
	});

	const DNS_STATUS status = ::DnsQuery_UTF8(name_, DNS_TYPE_MX, DNS_QUERY_STANDARD, nullptr, &results, nullptr);
	if (status)
	{
		return std::nullopt;
	}

	std::optional<std::vector<mx_srv>> retval;

	for (auto ptr = results; ptr; ptr = ptr->pNext) {
		if (ptr->wType != DNS_TYPE_MX) {
			continue;
		}
		const auto& item = ptr->Data.MX;
		mx_srv value{ .address = std::string{item.pNameExchange}, .priority = item.wPreference};

		if (!retval.has_value()) {
			retval.emplace();
		}
		retval->push_back(std::move(value));
	}

	return retval;
}

std::optional<std::vector<std::string>> txt_name(const char* name_)
{
	THROW_EXCEPTION_IF(!name_, std::invalid_argument, "name_ cannot be nullptr");
	DNS_RECORD* results = nullptr;
	const auto cleanup = on_scope_exit([&results]() {
		if (results) {
			DnsRecordListFree(results, DnsFreeRecordListDeep);
		}
		});

	const DNS_STATUS status = ::DnsQuery_UTF8(name_, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, nullptr, &results, nullptr);
	if (status)
	{
		return std::nullopt;
	}

	std::optional<std::vector<std::string>> retval;
	retval.emplace();

	for (auto ptr = results; ptr; ptr = ptr->pNext) {
		if (ptr->wType != DNS_TYPE_TEXT) {
			continue;
		}
		const auto& item = ptr->Data.TXT;
		std::string target;

		for (DWORD idx = 0; idx < item.dwStringCount; ++idx) {
			target.append(item.pStringArray[idx]);
		}

		if (!retval.has_value()) {
			retval.emplace();
		}
		retval->push_back(std::move(target));
	}

	return retval;
}

std::optional<std::vector<std::string>> c_name(const char* name_)
{
	THROW_EXCEPTION_IF(!name_, std::invalid_argument, "name_ cannot be nullptr");
	DNS_RECORD* results = nullptr;
	const auto cleanup = on_scope_exit([&results]() {
		if (results) {
			DnsRecordListFree(results, DnsFreeRecordListDeep);
		}
		});

	const DNS_STATUS status = ::DnsQuery_UTF8(name_, DNS_TYPE_CNAME, DNS_QUERY_STANDARD, nullptr, &results, nullptr);
	if (status)
	{
		return std::nullopt;
	}

	std::optional<std::vector<std::string>> retval;

	for (auto ptr = results; ptr; ptr = ptr->pNext) {
		if (ptr->wType != DNS_TYPE_CNAME) {
			continue;
		}
		const auto& item = ptr->Data.CNAME;

		if (!retval.has_value()) {
			retval.emplace();
		}
		retval->emplace_back(item.pNameHost);
	}

	return retval;
}
}