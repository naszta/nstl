#include "dns_tools.hpp"
#include "exception.hpp"

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windns.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nstl::net
{
namespace
{
struct DnsRecordDeleter
{
    void operator()(DNS_RECORDW* ptr) const { ::DnsFree(ptr, DnsFreeRecordList); }
};
using DnsRecordPtr = std::unique_ptr<DNS_RECORDW, DnsRecordDeleter>;

std::string to_utf8_view(const std::wstring_view view_)
{
    const auto size =
        ::WideCharToMultiByte(CP_UTF8, 0, view_.data(), static_cast<int>(view_.size()), nullptr, 0, nullptr, nullptr);
    if (size < 1)
    {
        return std::string{};
    }
    std::string retval(static_cast<size_t>(size), '\0');
    NSTL2_THROW_EXCEPTION_IF(::WideCharToMultiByte(CP_UTF8, 0, view_.data(), static_cast<int>(view_.size()),
                                                   retval.data(), size, nullptr, nullptr) <= 0,
                             "wstring_view failed to convert to string");
    return retval;
}

std::string to_utf8(const wchar_t* text_) { return text_ && text_[0] != L'\0' ? to_utf8_view(text_) : std::string{}; }

std::wstring to_wide_view(const std::string_view view_)
{
    const auto size = ::MultiByteToWideChar(CP_UTF8, 0, view_.data(), static_cast<int>(view_.size()), nullptr, 0);
    if (size < 1)
    {
        return std::wstring{};
    }
    std::wstring retval(static_cast<size_t>(size), L'\0');
    NSTL2_THROW_EXCEPTION_IF(
        ::MultiByteToWideChar(CP_UTF8, 0, view_.data(), static_cast<int>(view_.size()), retval.data(), size) <= 0,
        "string_view failed to convert to wstring");
    return retval;
}

std::wstring to_wide(const char* text_) { return text_ && text_[0] != '\0' ? to_wide_view(text_) : std::wstring{}; }

using DnsAddrArrayPtr = std::unique_ptr<DNS_ADDR_ARRAY, nstl::free_deleter>;

class sockaddr_writer
{
    using max_sockaddr_buf = CHAR[DNS_ADDR_MAX_SOCKADDR_LENGTH];

    max_sockaddr_buf& _tgt;
    std::uint16_t _port;

    template <class SockAddr> void _write(const SockAddr& sa_) const
    {
        std::memset(_tgt, 0, sizeof(max_sockaddr_buf));
        std::memcpy(_tgt, &sa_, sizeof(SockAddr));
    }

public:
    explicit sockaddr_writer(max_sockaddr_buf& tgt_, std::uint16_t port_ = 0)
        : _tgt{ tgt_ }, _port{ port_ }
    {
        static_assert(sizeof(max_sockaddr_buf) >= sizeof(SOCKADDR_IN6), "MaxSa too small for SOCKADDR_IN6");
        static_assert(sizeof(ipv4_addr{}) == sizeof(IN_ADDR), "ipv4_addr size mismatch");
        static_assert(sizeof(ipv6_addr{}) == sizeof(IN6_ADDR), "ipv6_addr size mismatch");
    }

    void operator()(const ipv4_addr& ip_) const
    {
        SOCKADDR_IN sa;
        std::memset(&sa, 0, sizeof(SOCKADDR_IN));
        sa.sin_family = AF_INET;
        sa.sin_port = htons(_port);
        std::memcpy(&sa.sin_addr, ip_.data(), ip_.size());
        _write(sa);
    }

    void operator()(const ipv6_addr& ip_) const
    {
        SOCKADDR_IN6 sa6;
        std::memset(&sa6, 0, sizeof(SOCKADDR_IN6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = htons(_port);
        std::memcpy(&sa6.sin6_addr, ip_.data(), ip_.size());
        _write(sa6);
    }
};

DnsAddrArrayPtr make_addr_array(const std::vector<ip_addr_gen>& servers_)
{
    const size_t bytes = sizeof(DNS_ADDR_ARRAY) + (servers_.size() - 1) * sizeof(DNS_ADDR);
    DnsAddrArrayPtr arr{ static_cast<DNS_ADDR_ARRAY*>(std::calloc(1, bytes)) };
    if (!arr) [[unlikely]]
    {
        throw std::bad_alloc{};
    }
    arr->MaxCount = arr->AddrCount = static_cast<DWORD>(servers_.size());
    arr->Family = AF_UNSPEC;
    for (size_t i = 0; i < servers_.size(); ++i)
    {
        std::visit(sockaddr_writer{ arr->AddrArray[i].MaxSa }, servers_[i]);
    }
    return arr;
}

DnsRecordPtr get_results_ex(const wchar_t* name_, WORD type_, observer_ptr<const dns_config> config_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "name_ cannot be nullptr");
    DNS_QUERY_REQUEST request;
    std::memset(&request, 0, sizeof(DNS_QUERY_REQUEST));
    request.Version = DNS_QUERY_REQUEST_VERSION1;
    request.QueryName = name_;
    request.QueryType = type_;
    request.QueryOptions = DNS_QUERY_STANDARD;
    if (type_ == DNS_TYPE_SVCB || type_ == DNS_TYPE_HTTPS)
    {
        request.QueryOptions |= DNS_QUERY_PARSE_ALL_RECORDS;
    }

    DnsAddrArrayPtr servers;
    if (config_ && !config_->dns_servers.empty())
    {
        servers = make_addr_array(config_->dns_servers);
        request.pDnsServerList = servers.get();
    }

    DNS_QUERY_RESULT result;
    std::memset(&result, 0, sizeof(DNS_QUERY_RESULT));
    result.Version = DNS_QUERY_RESULTS_VERSION1;
    const DNS_STATUS status = ::DnsQueryEx(&request, &result, nullptr);
    DnsRecordPtr retval{ reinterpret_cast<DNS_RECORDW*>(std::exchange(result.pQueryRecords, nullptr)) };
    if (status)
    {
        return nullptr;
    }
    return retval;
}

DnsRecordPtr get_results_ex(const std::string_view name_, WORD type_, observer_ptr<const dns_config> cfg_)
{
    NSTL2_THROW_EXCEPTION_IF(name_.empty(), "empty / nullptr cannot be resolved");
    const auto wide_name = to_wide_view(name_);
    return get_results_ex(wide_name.c_str(), type_, cfg_);
}
} // namespace

std::optional<std::vector<ipv4_addr>> ip_a_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_A, cfg_);

    std::optional<std::vector<ipv4_addr>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_A)
        {
            continue;
        }
        const auto& item = ptr->Data.A;
        if (!retval.has_value())
        {
            retval.emplace();
        }
        auto& tgt = retval->emplace_back();
        std::memcpy(tgt.data(), &item.IpAddress, tgt.size());
    }

    return retval;
}

std::optional<std::vector<ipv6_addr>> ip_aaaa_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_AAAA, cfg_);

    std::optional<std::vector<ipv6_addr>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_AAAA)
        {
            continue;
        }
        const auto& item = ptr->Data.AAAA;
        if (!retval.has_value())
        {
            retval.emplace();
        }
        auto& tgt = retval->emplace_back();
        std::memcpy(tgt.data(), &item.Ip6Address.IP6Byte[0], tgt.size());
    }

    return retval;
}

std::optional<std::vector<mx_srv>> mx_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_MX, cfg_);

    std::optional<std::vector<mx_srv>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_MX)
        {
            continue;
        }
        const auto& item = ptr->Data.MX;
        mx_srv value{ .address = to_utf8(item.pNameExchange), .priority = item.wPreference };

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->push_back(std::move(value));
    }

    return retval;
}

std::optional<std::vector<std::string>> txt_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_TEXT, cfg_);

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
            target.append(to_utf8(item.pStringArray[idx]));
        }

        if (!retval.has_value())
        {
            retval.emplace();
        }
        retval->push_back(std::move(target));
    }

    return retval;
}

std::optional<std::vector<std::string>> c_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_CNAME, cfg_);

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
        retval->emplace_back(to_utf8(item.pNameHost));
    }

    return retval;
}

std::optional<std::vector<gen_srv>> srv_name(const std::string_view name_, observer_ptr<const dns_config> cfg_)
{
    const auto results = get_results_ex(name_, DNS_TYPE_SRV, cfg_);

    std::optional<std::vector<gen_srv>> retval;

    for (auto ptr = results.get(); ptr; ptr = ptr->pNext)
    {
        if (ptr->wType != DNS_TYPE_SRV)
        {
            continue;
        }
        const auto& item = ptr->Data.SRV;
        gen_srv value{
            .address = to_utf8(item.pNameTarget), .port = item.wPort, .priority = item.wPriority, .weight = item.wWeight
        };

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
        std::string doh{ param_.pszDohPath };
        retval.emplace(std::move(doh));
        return retval;
    }
    case Ipv4Hint:
    {
        std::vector<ipv4_addr> ipv4s;
        ipv4s.reserve(param_.pIpv4Hints->cIps);
        for (WORD idx = 0; idx < param_.pIpv4Hints->cIps; ++idx)
        {
            auto& target = ipv4s.emplace_back();
            std::memcpy(target.data(), &param_.pIpv4Hints->rgIps[idx], target.size());
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
} // namespace

std::optional<std::vector<gen_svcb>> svcb_name(const std::string_view name_, const SvcbType type_,
                                               observer_ptr<const dns_config> cfg_)
{
    const auto dns_type = get_type_id(type_);
    const auto results = get_results_ex(name_, dns_type, cfg_);

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

namespace
{
enum class Ipv6Classes
{
    Loopback,
    LinkLocal,
    ULA,
    Teredo,
    Global,
    Other,
};

Ipv6Classes classify(const in6_addr& a)
{
    static constexpr ipv6_addr loopback{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    const unsigned char* b = a.s6_addr;
    if (std::memcmp(b, loopback.data(), loopback.size()) == 0)
    {
        return Ipv6Classes::Loopback;
    }
    if (b[0] == 0xfe && (b[1] & 0xc0) == 0x80)
    {
        return Ipv6Classes::LinkLocal;
    }
    if ((b[0] & 0xfe) == 0xfc)
    {
        return Ipv6Classes::ULA;
    }
    if (b[0] == 0x20 && b[1] == 0x01 && b[2] == 0 && b[3] == 0)
    {
        return Ipv6Classes::Teredo;
    }
    if ((b[0] & 0xe0) == 0x20)
    {
        return Ipv6Classes::Global;
    }
    return Ipv6Classes::Other;
}
} // namespace

bool box_have_valid_ipv6()
{
    ULONG buff_size = 32768;

    std::vector<std::uint8_t> buffer;
    ULONG rc = ERROR_BUFFER_OVERFLOW;
    for (int tries = 0; rc == ERROR_BUFFER_OVERFLOW && tries < 3; ++tries)
    {
        buffer.resize(buff_size);
        rc = GetAdaptersAddresses(AF_INET6, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                  nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &buff_size);
    }
    if (rc == ERROR_NO_DATA)
    {
        return false;
    }
    NSTL2_THROW_EXCEPTION_IF(rc != NO_ERROR, "IPv6 interfaces cannot be resolved");

    for (auto* ad = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); ad; ad = ad->Next)
    {
        if (ad->OperStatus != IfOperStatusUp)
        {
            continue;
        }

        for (auto* ua = ad->FirstUnicastAddress; ua; ua = ua->Next)
        {
            const sockaddr* sa = ua->Address.lpSockaddr;
            if (!sa || sa->sa_family != AF_INET6)
            {
                continue;
            }

            const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(sa);
            const auto kind = classify(sin6->sin6_addr);
            if (kind == Ipv6Classes::Global)
            {
                return true;
            }
        }
    }

    return false;
}
} // namespace nstl::net