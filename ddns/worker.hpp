#pragma once

#include "config.hpp"

#include <oneapi/tbb/concurrent_hash_map.h>

#include <boost/container/flat_set.hpp>

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
    using time_point = std::chrono::steady_clock::time_point;

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
    using named_addresses = tbb::concurrent_hash_map<std::string, names_item>;

    std::atomic_bool _enabled{ true };
    configs _configs;
    rule_applier _applier;
    named_addresses _cache;
    time_point _regular_log;

    void _check_items();
    void _logging();

public:
    explicit worker(configs configs_, bool dry_run_ = false);
    worker(configs configs_, rule_applier applier_);

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
