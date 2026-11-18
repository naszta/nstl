#pragma once

#include <nstl/dns_tools.hpp>

#include <nlohmann/json_fwd.hpp>

#include <boost/container/flat_map.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace nstl::ddns
{
struct config
{
    config();
    explicit config(const nlohmann::json& src_);
    void reset();
    bool valid() const;

    std::string apply_mask(const net::ip_addr_gen& ip_) const;

    bool enabled{ true };
    std::string table;
    nstl::net::IpClass ip_class{ nstl::net::IpClass::Ipv4 };
    std::vector<std::string> ddns_names;
    std::uint8_t range{ 0 };
    std::variant<std::monostate, net::ipv4_addr, net::ipv6_addr> mask;
};

using configs = boost::container::flat_map<std::string, config, std::less<>>;

configs parse_file(const std::filesystem::path& filepath_);
configs parse_payload(std::string_view payload_);
configs parse_payload(const nlohmann::json& src_);

} // namespace nstl::ddns
