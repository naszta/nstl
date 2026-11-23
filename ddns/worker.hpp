#pragma once

#include "config.hpp"

#include <oneapi/tbb/concurrent_hash_map.h>

#include <boost/container/flat_set.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

namespace nstl::net
{
struct dns_config;
}

namespace nstl::ddns
{
using ip_addresses = boost::container::flat_set<std::string, std::less<>>;

using rule_applier =
    std::function<bool(const std::string& name_, const std::string& table_, const ip_addresses& addresses_)>;

class worker
{
    using time_point = std::chrono::steady_clock::time_point;

    enum class State
    {
        Ready,
        Pending,
    };
    struct names_item
    {
        void check_item(const std::string& name_, const config& cfg_,
                        const rule_applier& apply_, observer_ptr<const net::dns_config> dns_cfg_ = nullptr);
        ip_addresses addresses;
        ip_addresses other;
        State state{ State::Pending };
    };
    using named_addresses = tbb::concurrent_hash_map<std::string, names_item>;

    std::atomic_bool _enabled{ true };
    configs _configs;
    rule_applier _applier;
    named_addresses _cache;
    time_point _regular_log;
    std::shared_ptr<const net::dns_config> _cfg;

    void _check_items();
    void _logging();

public:
    worker(configs configs_, custom_dns cdns_, bool dry_run_ = false);
    worker(configs configs_, custom_dns cdns_, rule_applier applier_);

    template <class Rep, class Period> void run(std::chrono::duration<Rep, Period> dur_)
    {
        _enabled = true;
        while (_enabled)
        {
            this->_check_items();
            this->_logging();
            std::this_thread::sleep_for(dur_);
        }
    }

    bool stop();
};

} // namespace nstl::ddns
