#include "worker.hpp"
#include "nft.hpp"

#include <nstl/dns_tools.hpp>
#include <nstl/exception.hpp>
#include <nstl/logging.hpp>
#include <nstl/range_print.hpp>

#include <format>
#include <sstream>

namespace nstl::ddns
{
namespace
{
std::shared_ptr<nft_base> nft_factory(const bool dry_run_)
{
    if (dry_run_)
    {
        return std::make_shared<nft_logonly>();
    }
    else
    {
        return std::make_shared<nft>();
    }
}

rule_applier applier_factory(const bool dry_run_)
{
    auto ptr = nft_factory(dry_run_);
    NSTL2_THROW_EXCEPTION_IF(!ptr->valid(), "NFT target is not valid");
    return [ptr = std::move(ptr)](const std::string& name_, const std::string& table_, const ip_addresses& addresses_)
    {
        const auto flush_cmd = std::format("flush set {} {}", table_, name_);
        if (!ptr->execute(flush_cmd.c_str()))
        {
            return false;
        }
        if (addresses_.empty())
        {
            return true;
        }

        std::ostringstream ip_command;
        ip_command << "add element " << table_ << ' ' << name_ << " { " << range_print(addresses_, ',') << " }";
        const auto ips_cmd = ip_command.str();

        return ptr->execute(ips_cmd.c_str());
    };
}
} // namespace

worker::worker(configs configs_, custom_dns cdns_, const bool dry_run_)
    : worker{ std::move(configs_), std::move(cdns_), applier_factory(dry_run_) }
{
}

worker::worker(configs configs_, custom_dns cdns_, rule_applier applier_)
    : _configs{ std::move(configs_) }
    , _applier{ std::move(applier_) }
    , _cache{64}
    , _regular_log{ std::chrono::steady_clock::now() }
{
    if (!cdns_.empty())
    {
        _cfg = std::make_shared<net::dns_config>(std::move(cdns_));
    }

    NSTL2_THROW_EXCEPTION_IF(!_applier, "rule applier is empty");
}

void worker::names_item::check_item(const std::string& name_, const config& cfg_, const rule_applier& apply_,
                                    observer_ptr<const net::dns_config> dns_cfg_)
{
    try
    {
        other.clear();
        for (const auto& name : cfg_.ddns_names)
        {
            if (const auto ip_addresses = nstl::net::ips_name(name, cfg_.ip_class, dns_cfg_))
            {
                for (const auto& item : ip_addresses.value())
                {
                    other.insert(cfg_.apply_mask(item));
                }
            }
        }
    }
    catch (const std::exception& exc)
    {
        state = State::Pending;
        NSTL_ERROR("DDNS failed to process: " << exc.what());
        return;
    }

    const bool has_change = other != addresses;
    if (!has_change && state == State::Ready)
    {
        NSTL_DEBUG(name_ << ": addresses has't been changed [" << range_print(addresses, ',') << "]");
        return;
    }
    if (has_change)
    {
        state = State::Pending;
        other.swap(addresses);
    }
    NSTL_INFO(name_ << ": addresses updated from [" << range_print(other, ',') << "] to ["
                    << range_print(addresses, ',') << "]");
    if (apply_(name_, cfg_.table, addresses))
    {
        state = State::Ready;
    }
}

void worker::_check_items()
{
    for (const auto& [name, config] : _configs)
    {
        if (config.enabled)
        {
            named_addresses::accessor accessor;
            _cache.insert(accessor, name);
            accessor->second.check_item(name, config, _applier, _cfg);
        }
    }
}

void worker::_logging()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - _regular_log < std::chrono::hours{ 1 }) [[likely]]
    {
        return;
    }
    _regular_log = now;
    for (const auto& [name, _] : _configs)
    {
        named_addresses::const_accessor accessor;
        if (_cache.find(accessor, name))
        {
            NSTL_INFO("Current state: " << name << " values: [" << range_print(accessor->second.addresses, ", ") << ']');
        }
    }
}

bool worker::stop() { return _enabled.exchange(false); }
} // namespace nstl::ddns
