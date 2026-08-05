#include "dns_tools.hpp"
#include "exception.hpp"

#include <Windows.h>
#include <windns.h>

#include <stdexcept>
#include <utility>

namespace nstl::net
{
namespace
{
struct DnsRecordDeleter
{
    void operator()(DNS_RECORD* ptr) const { ::DnsFree(ptr, DnsFreeRecordList); }
};
using DnsRecordPtr = std::unique_ptr<DNS_RECORD, DnsRecordDeleter>;

DnsRecordPtr get_results(const char* name_, WORD type_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
    DNS_RECORD* results = nullptr;
    const DNS_STATUS status = ::DnsQuery_UTF8(name_, type_, DNS_QUERY_STANDARD, nullptr, &results, nullptr);
    DnsRecordPtr retval{ std::exchange(results, nullptr) };
    if (status)
    {
        return nullptr;
    }
    return retval;
}
} // namespace

std::optional<std::vector<mx_srv>> mx_name(const char* name_)
{
    const auto results = get_results(name_, DNS_TYPE_MX);

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
    const auto results = get_results(name_, DNS_TYPE_TEXT);

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
    const auto results = get_results(name_, DNS_TYPE_CNAME);

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

std::optional<std::vector<gen_srv>> srv_name(const char* name_)
{
    const auto results = get_results(name_, DNS_TYPE_SRV);

    std::optional<std::vector<gen_srv>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_SRV)
        {
            continue;
        }
        const auto& item = ptr->Data.SRV;
        gen_srv value{ .address = std::string{ item.pNameTarget },
                       .port = item.wPort,
                       .priority = item.wPriority,
                       .weight = item.wWeight };

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->push_back(std::move(value));
    }

    return retval;
}
} // namespace nstl::net