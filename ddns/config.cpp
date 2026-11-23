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
    this->range = 0;
    this->enabled = true;
}

bool config::valid() const { return !this->table.empty() && !this->ddns_names.empty(); }

std::pair<configs, custom_dns> parse_file(const std::filesystem::path& filepath_)
{
    std::ifstream file{ filepath_ };
    const auto payload = nlohmann::json::parse(file);
    return parse_payload(payload);
}

std::pair<configs, custom_dns> parse_payload(std::string_view payload_)
{
    const auto payload = nlohmann::json::parse(payload_);
    return parse_payload(payload);
}

namespace
{

struct ip_push_visitor
{
    custom_dns& target;

    explicit ip_push_visitor(custom_dns& target_) : target{ target_ } {}

    void operator()(const std::monostate&) const { NSTL2_THROW_EXCEPTION("dns contains invalid IP address"); }

    void operator()(const net::ipv4_addr& ip_) const { target.push_back(ip_); }

    void operator()(const net::ipv6_addr& ip_) const { target.push_back(ip_); }
};
} // namespace

std::pair<configs, custom_dns> parse_payload(const nlohmann::json& src_)
{
    NSTL2_THROW_EXCEPTION_IF(!src_.is_object(), "json expected to be object");
    custom_dns servers;
    configs items;
    if (const auto ditr = src_.find("dns"); ditr != src_.end())
    {
        NSTL2_THROW_EXCEPTION_IF(!ditr->is_array(), "dns expected to be array");
        const ip_push_visitor visitor{ servers };

        for (auto sitr = ditr->cbegin(); sitr != ditr->cend(); ++sitr)
        {
            NSTL2_THROW_EXCEPTION_IF(!sitr->is_string(), "dns items expected to be string");
            const auto ip_opt = net::parse_ip_address(sitr->get_ref<const std::string&>());
            std::visit(visitor, ip_opt);
        }
    }

    if (const auto iitr = src_.find("items"); iitr != src_.end())
    {
        NSTL2_THROW_EXCEPTION_IF(!iitr->is_object(), "items expected to be object");
        items.reserve(iitr->size());
        for (auto itr = iitr->begin(); itr != iitr->end(); ++itr)
        {
            const auto helper = items.try_emplace(itr.key(), itr.value());
            NSTL2_THROW_EXCEPTION_IF(!helper.second, itr.key() << " duplicated");
            NSTL2_THROW_EXCEPTION_IF(!helper.first->second.valid(), itr.key() << " is not valid");
        }
    }
    return std::make_pair(std::move(items), std::move(servers));
}

namespace
{
struct ip_visitor
{
    std::uint8_t _range;

    explicit ip_visitor(std::uint8_t range_) : _range{ range_ } {}

    // mo mask
    std::string operator()(const net::ipv4_addr& ip_, const std::monostate&) const { return net::to_string(ip_); }
    std::string operator()(const net::ipv6_addr& ip_, const std::monostate&) const { return net::to_string(ip_); }
    // proper mask
    std::string operator()(const net::ipv4_addr& ip_, const net::ipv4_addr& mask_) const
    {
        net::ipv4_addr result;
        net::and_arrays(ip_, mask_, result);
        std::ostringstream oss;
        net::to_stream(oss, result);
        oss << '/' << static_cast<int>(_range);
        return oss.str();
    }
    std::string operator()(const net::ipv6_addr& ip_, const net::ipv6_addr& mask_) const
    {
        net::ipv6_addr result;
        net::and_arrays(ip_, mask_, result);
        std::ostringstream oss;
        net::to_stream(oss, result);
        oss << '/' << static_cast<int>(_range);
        return oss.str();
    }
    // we have a bug
    std::string operator()(const net::ipv6_addr&, const net::ipv4_addr&) const
    {
        NSTL2_THROW_EXCEPTION("IPv4 mask must not be applied on IPv6 address: we have a bug");
    }
    std::string operator()(const net::ipv4_addr&, const net::ipv6_addr&) const
    {
        NSTL2_THROW_EXCEPTION("IPv6 mask must not be applied on IPv4 address: we have a bug");
    }
};
} // namespace

std::string config::apply_mask(const net::ip_addr_gen& ip_) const
{
    return std::visit(ip_visitor{ this->range }, ip_, mask);
}

} // namespace nstl::ddns
