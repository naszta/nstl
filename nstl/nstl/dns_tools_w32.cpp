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

DnsRecordPtr get_results_ex(const wchar_t* name_, WORD type_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
    DNS_QUERY_REQUEST3 request;
    std::memset(&request, 0, sizeof(DNS_QUERY_REQUEST3));
    request.Version = DNS_QUERY_REQUEST_VERSION3;
    request.QueryName = name_;
    request.QueryType = type_;
    request.QueryOptions = DNS_QUERY_STANDARD | DNS_QUERY_PARSE_ALL_RECORDS;

    DNS_QUERY_RESULT result;
    std::memset(&result, 0, sizeof(DNS_QUERY_RESULT));
    result.Version = DNS_QUERY_RESULTS_VERSION1;
    const DNS_STATUS status = ::DnsQueryEx(reinterpret_cast<PDNS_QUERY_REQUEST>(&request), &result, nullptr);
    DnsRecordPtr retval{ std::exchange(result.pQueryRecords, nullptr) };
    if (status)
    {
        return nullptr;
    }
    return retval;
}

DnsRecordPtr get_results_ex(const char* name_, WORD type_)
{
    const auto dns_name = name_ ? std::string_view{ name_ } : std::string_view{};
    NSTL2_THROW_EXCEPTION_IF(dns_name.empty(), "empty / nullptr cannot be resolved");
    const auto size = ::MultiByteToWideChar(CP_UTF8, 0, dns_name.data(), static_cast<int>(dns_name.size()), NULL, 0);
    NSTL2_THROW_EXCEPTION_IF(size <= 0, dns_name << " cannot be converted to wchar_t");
    std::vector<wchar_t> buffer;
    buffer.resize(size + 1);
    const auto written =
        ::MultiByteToWideChar(CP_UTF8, 0, dns_name.data(), static_cast<int>(dns_name.size()), buffer.data(), size);
    NSTL2_THROW_EXCEPTION_IF(written <= 0, dns_name << " cannot be converted to wchar_t");
    buffer[written] = L'\0';
    return get_results_ex(buffer.data(), type_);
}

DnsRecordPtr get_results(const char* name_, WORD type_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ is nullptr");
    PDNS_RECORDA result = nullptr;
    const DNS_STATUS status = ::DnsQuery_UTF8(name_, type_, DNS_QUERY_STANDARD, nullptr, &result, nullptr);
    DnsRecordPtr retval{ std::exchange(result, nullptr) };
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

namespace
{
enum KeyValue
{
    Mandatory = 0,
    Alpn = 1,
    NoDefaultAlpn = 2,
    Port = 3,
    Ipv4Hint = 4,
    Ech = 5,
    Ipv6Hint = 6,
    DohPath = 7,
};

std::optional<svcb_param> convert_param(const DNS_SVCB_PARAM& param_)
{
    std::optional<svcb_param> retval;
    switch (param_.wSvcParamKey)
    {
    case Mandatory:
    {
        std::vector<std::uint16_t> keys;
        keys.reserve(param_.pMandatory->cMandatoryKeys);
        for (WORD idx = 0; idx < param_.pMandatory->cMandatoryKeys; ++idx)
        {
            keys.push_back(param_.pMandatory->rgwMandatoryKeys[idx]);
        }
        retval.emplace(std::move(keys));
        return retval;
    }
    case Alpn:
    {
        std::vector<std::string> alpns;
        alpns.reserve(param_.pAlpn->cIds);
        for (WORD idx = 0; idx < param_.pAlpn->cIds; ++idx)
        {
            const auto& alpn = param_.pAlpn->rgIds[idx];
            alpns.emplace_back(reinterpret_cast<const char*>(alpn.pbId), alpn.cBytes);
        }

        retval.emplace(std::move(alpns));
        return retval;
    }
    case NoDefaultAlpn:
        retval.emplace(std::monostate{});
        return retval;
    case Port:
        retval.emplace(param_.wPort);
        return retval;
    case DohPath:
    {
        std::string doh{param_.pszDohPath};
        retval.emplace(std::move(doh));
        return retval;
    }
    case Ipv4Hint:
    {
        std::vector<std::uint32_t> ipv4s;
        ipv4s.reserve(param_.pIpv4Hints->cIps);
        for (WORD idx = 0; idx < param_.pIpv4Hints->cIps; ++idx)
        {
            ipv4s.push_back(param_.pIpv4Hints->rgIps[idx]);
        }
        retval.emplace(std::move(ipv4s));
        return retval;
    }
    case Ipv6Hint:
    {
        std::vector<std::array<std::uint8_t, 16>> ipv6s;
        ipv6s.reserve(param_.pIpv6Hints->cIps);
        for (WORD idx = 0; idx < param_.pIpv6Hints->cIps; ++idx)
        {
            const IP6_ADDRESS& ipv6 = param_.pIpv6Hints->rgIps[idx];
            auto& target = ipv6s.emplace_back();
            std::memcpy(target.data(), ipv6.IP6Byte, target.size());
        }
        retval.emplace(std::move(ipv6s));
        return retval;
    }
    default:
        return retval;
    }
}

gen_svcb convert_data(const DNS_SVCB_DATA& data_)
{
    gen_svcb data;
    data.address = data_.pszTargetName;
    data.priority = data_.wSvcPriority;

    data.params.reserve(data_.cSvcParams);

    for (WORD idx = 0; idx < data_.cSvcParams; ++idx)
    {
        if (auto param = convert_param(data_.pSvcParams[idx]))
        {
            data.params.push_back(std::move(param.value()));
        }
    }

    return data;
}

WORD get_type_id(const SvcbType type_)
{
    switch (type_)
    {
    case SvcbType::Svcb:
        return DNS_TYPE_SVCB;
    case SvcbType::Https:
        return DNS_TYPE_HTTPS;
    default:
        NSTL2_THROW_EXCEPTION("invalid svcb type");
    }
}
}

std::optional<std::vector<gen_svcb>> svcb_name(const char* name_, const SvcbType type_)
{
    const auto dns_type = get_type_id(type_);
    const auto results = get_results_ex(name_, dns_type);

    std::optional<std::vector<gen_svcb>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != dns_type)
        {
            continue;
        }
        auto value = convert_data(ptr->Data.Svcb);
        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->emplace_back(std::move(value));
    }

    return retval;
}
} // namespace nstl::net