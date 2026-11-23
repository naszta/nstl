#include "worker.hpp"

#include <nstl/exception.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

using nstl::ddns::config;
using nstl::ddns::configs;
using nstl::ddns::custom_dns;
using nstl::ddns::ip_addresses;
using nstl::ddns::rule_applier;
using nstl::ddns::worker;

namespace
{
constexpr auto tick = 1ms;
constexpr auto wait_timeout = 10s;
// Time the worker keeps looping after the condition was met, so unexpected extra applier calls are observed.
constexpr auto settle_time = 100ms;

struct applier_call
{
    std::string name;
    std::string table;
    ip_addresses addresses;
};

class recording_applier
{
    mutable std::mutex _mutex;
    std::vector<applier_call> _calls;
    std::function<bool(std::size_t)> _result;

public:
    explicit recording_applier(std::function<bool(std::size_t)> result_ = [](std::size_t) { return true; })
        : _result{ std::move(result_) }
    {
    }

    rule_applier get()
    {
        return [this](const std::string& name_, const std::string& table_, const ip_addresses& addresses_)
        {
            std::size_t idx;
            {
                const std::lock_guard lock{ _mutex };
                idx = _calls.size();
                _calls.push_back({ name_, table_, addresses_ });
            }
            return _result(idx);
        };
    }

    std::vector<applier_call> calls() const
    {
        const std::lock_guard lock{ _mutex };
        return _calls;
    }

    std::size_t count() const
    {
        const std::lock_guard lock{ _mutex };
        return _calls.size();
    }
};

config make_config(std::string table_, std::vector<std::string> names_)
{
    config cfg;
    cfg.table = std::move(table_);
    cfg.ddns_names = std::move(names_);
    cfg.ip_class = nstl::net::IpClass::Ipv4;
    return cfg;
}

// Runs the worker on a background thread until `done_` returns true (or timeout), lets it loop for an
// additional settle period, then stops it. Returns whether `done_` was satisfied.
bool run_until(worker& worker_, const std::function<bool()>& done_)
{
    std::thread runner{ [&worker_] { worker_.run(tick); } };
    const auto deadline = std::chrono::steady_clock::now() + wait_timeout;
    bool satisfied = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (done_())
        {
            satisfied = true;
            break;
        }
        std::this_thread::sleep_for(tick);
    }
    std::this_thread::sleep_for(settle_time);
    // run() sets the enabled flag on entry; the settle period above guarantees it has started.
    worker_.stop();
    runner.join();
    return satisfied;
}
} // namespace

TEST(DdnsWorker, EmptyApplierThrows)
{
    EXPECT_THROW((worker{ configs{}, custom_dns{}, rule_applier{} }), nstl::exception);
}

TEST(DdnsWorker, DryRunConstructs) { EXPECT_NO_THROW((worker{ configs{}, custom_dns{}, true })); }

TEST(DdnsWorker, StopReturnsPreviousState)
{
    worker w{ configs{}, custom_dns{}, true };
    EXPECT_TRUE(w.stop());
    EXPECT_FALSE(w.stop());
}

TEST(DdnsWorker, EmptyConfigsRunAndStop)
{
    recording_applier rec;
    worker w{ configs{}, custom_dns{}, rec.get() };
    // nothing to wait for: the condition never holds, so this runs until the timeout
    std::thread runner{ [&w] { w.run(tick); } };
    std::this_thread::sleep_for(settle_time);
    w.stop();
    runner.join();
    EXPECT_EQ(rec.count(), 0u);
}

TEST(DdnsWorker, AppliesResolvedAddresses)
{
    recording_applier rec;
    configs cfgs;
    cfgs.emplace("home", make_config("inet filter", { "localhost" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= 1; }));
    const auto calls = rec.calls();
    // Addresses don't change between iterations, so after a successful apply nothing more happens.
    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].name, "home");
    EXPECT_EQ(calls[0].table, "inet filter");
    EXPECT_EQ(calls[0].addresses, ip_addresses{ "127.0.0.1" });
}

TEST(DdnsWorker, AppliesMaskedAddresses)
{
    recording_applier rec;
    configs cfgs;
    auto cfg = make_config("inet filter", { "localhost" });
    cfg.range = 8;
    cfg.mask = nstl::net::create_ipv4_mask(cfg.range);
    cfgs.emplace("home", std::move(cfg));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= 1; }));
    const auto calls = rec.calls();
    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].addresses, ip_addresses{ "127.0.0.0/8" });
}

TEST(DdnsWorker, DuplicateNamesDeduplicated)
{
    recording_applier rec;
    configs cfgs;
    cfgs.emplace("home", make_config("inet filter", { "localhost", "localhost" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= 1; }));
    const auto calls = rec.calls();
    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].addresses, ip_addresses{ "127.0.0.1" });
}

TEST(DdnsWorker, DisabledConfigSkipped)
{
    recording_applier rec;
    configs cfgs;
    auto disabled = make_config("inet filter", { "localhost" });
    disabled.enabled = false;
    cfgs.emplace("disabled", std::move(disabled));
    cfgs.emplace("enabled", make_config("inet other", { "localhost" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= 1; }));
    const auto calls = rec.calls();
    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].name, "enabled");
    EXPECT_EQ(calls[0].table, "inet other");
}

TEST(DdnsWorker, MultipleConfigsEachApplied)
{
    recording_applier rec;
    configs cfgs;
    cfgs.emplace("a", make_config("table_a", { "localhost" }));
    cfgs.emplace("b", make_config("table_b", { "localhost" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= 2; }));
    const auto calls = rec.calls();
    ASSERT_EQ(calls.size(), 2u);
    // configs is an ordered flat_map, iteration order is by key
    EXPECT_EQ(calls[0].name, "a");
    EXPECT_EQ(calls[0].table, "table_a");
    EXPECT_EQ(calls[1].name, "b");
    EXPECT_EQ(calls[1].table, "table_b");
}

TEST(DdnsWorker, FailedApplyIsRetried)
{
    constexpr std::size_t failures = 3;
    recording_applier rec{ [](std::size_t idx_) { return idx_ >= failures; } };
    configs cfgs;
    cfgs.emplace("home", make_config("inet filter", { "localhost" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    ASSERT_TRUE(run_until(w, [&] { return rec.count() >= failures + 1; }));
    const auto calls = rec.calls();
    // failures + the one successful apply, then no further calls
    ASSERT_EQ(calls.size(), failures + 1);
    for (const auto& call : calls)
    {
        EXPECT_EQ(call.name, "home");
        EXPECT_EQ(call.addresses, ip_addresses{ "127.0.0.1" });
    }
}

TEST(DdnsWorker, UnresolvableNameAppliesEmptySetAtMostOnce)
{
    recording_applier rec;
    configs cfgs;
    cfgs.emplace("home", make_config("inet filter", { "this-host-should-never-exist.invalid" }));
    worker w{ std::move(cfgs), custom_dns{}, rec.get() };

    // Depending on the resolver, an unknown host is reported either as "no address" (empty set gets
    // applied once to flush the nft set) or as a resolver error (nothing is applied). Either way the
    // applier must never see an address and must not be called repeatedly.
    run_until(w, [&] { return rec.count() >= 1; });
    const auto calls = rec.calls();
    ASSERT_LE(calls.size(), 1u);
    for (const auto& call : calls)
    {
        EXPECT_TRUE(call.addresses.empty());
    }
}
