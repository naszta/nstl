#include "config.hpp"

#include <nstl/exception.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

namespace nstl::ddns
{

config::config() = default;

config::config(const nlohmann::json& src_)
{
    NSTL2_THROW_EXCEPTION_IF(!src_.is_object(), "json expected to be object");
    // table
    if (const auto titr = src_.find("table"); titr != src_.end())
    {
        NSTL2_THROW_EXCEPTION_IF(!titr->is_string(), "table expected to be string");
        this->table = titr->get<nlohmann::json::string_t>();
    }
    // ip class
    if (const auto citr = src_.find("type"); citr != src_.end())
    {
        if (citr->is_string())
        {
            const auto& value = citr->get_ref<const nlohmann::json::string_t&>();
            if (value == "ip4")
            {
                this->ip_class = nstl::net::IpClass::Ipv4;
            }
            else if (value == "ip6")
            {
                this->ip_class = nstl::net::IpClass::Ipv6;
            }
            else
            {
                NSTL2_THROW_EXCEPTION(value << " is invalid IP class value");
            }
        }
        else if (citr->is_number_unsigned())
        {
            const auto value = citr->get<nlohmann::json::number_unsigned_t>();
            if (value == 4)
            {
                this->ip_class = net::IpClass::Ipv4;
            }
            else if (value == 6)
            {
                this->ip_class = net::IpClass::Ipv6;
            }
            else
            {
                NSTL2_THROW_EXCEPTION(value << " is invalid IP class value");
            }
        }
        else
        {
            NSTL2_THROW_EXCEPTION("type expected to be string or integer");
        }
    }
    // ddns_names
    if (const auto ditr = src_.find("ddns"); ditr != src_.end())
    {
        if (ditr->is_string())
        {
            this->ddns_names.reserve(1);
            this->ddns_names.push_back(ditr->get<nlohmann::json::string_t>());
        }
        else if (ditr->is_array())
        {
            this->ddns_names.reserve(ditr->size());
            for (const auto& item : *ditr)
            {
                NSTL2_THROW_EXCEPTION_IF(!item.is_string(), "all ddns items expected to be string");
                this->ddns_names.push_back(item.get<nlohmann::json::string_t>());
            }
        }
        else
        {
            NSTL2_THROW_EXCEPTION("ddns expected to be string or string array");
        }
    }
    // range
    if (const auto ritr = src_.find("range"); ritr != src_.end())
    {
        NSTL2_THROW_EXCEPTION_IF(!ritr->is_number_unsigned(), "range expected to be unsigned integer");
        const auto mask_val = ritr->get<nlohmann::json::number_unsigned_t>();
        switch (this->ip_class)
        {
        case net::IpClass::Ipv4:
            NSTL2_THROW_EXCEPTION_IF(net::ipv4_size * net::bytes_in_bits < mask_val,
                                     mask_val << " is not valid mask for IPv4");
            this->range = static_cast<std::uint8_t>(mask_val);
            this->mask = net::create_ipv4_mask(this->range);
            break;
        case net::IpClass::Ipv6:
            NSTL2_THROW_EXCEPTION_IF(net::ipv6_size * net::bytes_in_bits < mask_val,
                                     mask_val << " is not valid mask for IPv6");
            this->range = static_cast<std::uint8_t>(mask_val);
            this->mask = net::create_ipv6_mask(this->range);
            break;
        case net::IpClass::IpvAll:
        default:
            NSTL2_THROW_EXCEPTION("Invalid IP class");
        }
    }
    // enabled
    if (const auto eitr = src_.find("enabled"); eitr != src_.end())
    {
        NSTL2_THROW_EXCEPTION_IF(!eitr->is_boolean(), "enabled expected to be boolean");
        this->enabled = eitr->get<bool>();
    }
}

void config::reset()
{
    this->table.clear();
    this->ip_class = nstl::net::IpClass::Ipv4;
    this->ddns_names.clear();
    this->mask = std::monostate{};
}

bool config::valid() const { return !this->table.empty() && !this->ddns_names.empty(); }

configs parse_file(const std::filesystem::path& filepath_)
{
    std::ifstream file{ filepath_ };
    const auto payload = nlohmann::json::parse(file);
    return parse_payload(payload);
}

configs parse_payload(std::string_view payload_)
{
    const auto payload = nlohmann::json::parse(payload_);
    return parse_payload(payload);
}

configs parse_payload(const nlohmann::json& src_)
{
    NSTL2_THROW_EXCEPTION_IF(!src_.is_object(), "json expected to be object");
    configs retval;
    retval.reserve(src_.size());
    for (auto itr = src_.begin(); itr != src_.end(); ++itr)
    {
        const auto helper = retval.try_emplace(itr.key(), itr.value());
        NSTL2_THROW_EXCEPTION_IF(!helper.second, itr.key() << " duplicated");
        NSTL2_THROW_EXCEPTION_IF(!helper.first->second.valid(), itr.key() << " is not valid");
    }
    return retval;
}

namespace
{
struct ip_visitor
{
    const net::ip_addr_gen& _ip;
    std::uint8_t _range;

    ip_visitor(const net::ip_addr_gen& ip_, std::uint8_t range_) : _ip{ ip_ }, _range{ range_ } {}

    std::string operator()(const std::monostate&) const { return net::to_string(_ip); }
    std::string operator()(const net::ipv4_addr& mask_) const
    {
        const auto& address = std::get<net::ipv4_addr>(_ip);
        net::ipv4_addr result;
        net::and_arrays(address, mask_, result);
        std::ostringstream oss;
        net::to_stream(oss, result);
        oss << '/' << static_cast<int>(_range);
        return oss.str();
    }
    std::string operator()(const net::ipv6_addr& mask_) const
    {
        const auto& address = std::get<net::ipv6_addr>(_ip);
        net::ipv6_addr result;
        net::and_arrays(address, mask_, result);
        std::ostringstream oss;
        net::to_stream(oss, result);
        oss << '/' << static_cast<int>(_range);
        return oss.str();
    }
};
} // namespace

std::string config::apply_mask(const net::ip_addr_gen& ip_) const
{
    return std::visit(ip_visitor{ ip_, this->range }, mask);
}

} // namespace nstl::ddns
