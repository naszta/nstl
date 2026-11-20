#pragma once

#include "config.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace nstl::ddns
{
using ip_addresses = boost::container::flat_set<std::string, std::less<>>;

using rule_applier =
    std::function<bool(const std::string& name_, const std::string& table_, const ip_addresses& addresses_)>;

class worker
{
    enum class State
    {
        Ready,
        Pending,
    };
    struct names_item
    {
        void check_item(const std::string& name_, const config& cfg_, const rule_applier& apply_);
        ip_addresses addresses;
        ip_addresses other;
        State state{ State::Pending };
    };
    using named_addresses = boost::container::flat_map<std::string, names_item, std::less<>>;

    std::atomic_bool _enabled{ true };
    configs _configs;
    rule_applier _applier;
    named_addresses _cache;

    void _check_items();

public:
    explicit worker(configs configs_, bool dry_run_ = false);
    worker(configs configs_, rule_applier applier_);

    template <class Rep, class Period> void run(std::chrono::duration<Rep, Period> dur_)
    {
        _enabled = true;
        while (_enabled)
        {
            this->_check_items();
            std::this_thread::sleep_for(dur_);
        }
    }

    bool stop();
};

} // namespace nstl::ddns
