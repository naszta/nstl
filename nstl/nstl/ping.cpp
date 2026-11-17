#include "ping.hpp"

#include <nstl/dns_tools.hpp>
#include <nstl/exception.hpp>

namespace
{
nstl::net::ip_addr_gen resolve_first(const std::string_view fqdn_)
{
    const auto ips = nstl::net::ips_name(fqdn_);
    NSTL2_THROW_EXCEPTION_IF(!ips.has_value() || ips->empty(), "Unable to resolve " << fqdn_);
    return ips->front();
}
} // namespace

namespace nstl::net
{
Ping::Ping(const std::string_view fqdn_) : Ping{ resolve_first(fqdn_) } {}
} // namespace nstl::net
