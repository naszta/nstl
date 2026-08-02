#include "dns_tools.hpp"
#include "exception.hpp"

#include <Windows.h>
#include <windns.h>

#include <stdexcept>

namespace nstl::net
{
namespace
{
struct DnsRecordDeleter
{
    void operator()(DNS_RECORD* ptr) const
    {
        if (ptr)
        {
            DnsRecordListFree(ptr, DnsFreeRecordListDeep);
        }
    }
};

std::unique_ptr<DNS_RECORD, DnsRecordDeleter> get_results(const char* name_, WORD type_, DWORD options_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
    DNS_RECORD* results = nullptr;
    const DNS_STATUS status = ::DnsQuery_UTF8(name_, type_, options_, nullptr, &results, nullptr);
    std::unique_ptr<DNS_RECORD, DnsRecordDeleter> retval{ results };
    if (status)
    {
        return nullptr;
    }
    return retval;
}
} // namespace

std::optional<std::vector<mx_srv>> mx_name(const char* name_)
{
    const auto results = get_results(name_, DNS_TYPE_MX, DNS_QUERY_STANDARD);
    std::optional<std::vector<mx_srv>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_MX)
        {
            continue;
        }
        const auto& item = ptr->Data.MX;
        mx_srv value{ .address = std::string{ item.pNameExchange }, .priority = item.wPreference };

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->push_back(std::move(value));
    }

    return retval;
}

std::optional<std::vector<std::string>> txt_name(const char* name_)
{
    const auto results = get_results(name_, DNS_TYPE_TEXT, DNS_QUERY_STANDARD);
    std::optional<std::vector<std::string>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_TEXT)
        {
            continue;
        }
        const auto& item = ptr->Data.TXT;
        std::string target;

        for (DWORD idx = 0; idx < item.dwStringCount; ++idx)
        {
            target.append(item.pStringArray[idx]);
        }

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->push_back(std::move(target));
    }

    return retval;
}

std::optional<std::vector<std::string>> c_name(const char* name_)
{
    const auto results = get_results(name_, DNS_TYPE_CNAME, DNS_QUERY_STANDARD);

    std::optional<std::vector<std::string>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_CNAME)
        {
            continue;
        }
        const auto& item = ptr->Data.CNAME;

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->emplace_back(item.pNameHost);
    }

    return retval;
}
} // namespace nstl::net