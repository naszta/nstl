#include "spf_tools.hpp"
#include "dns_tools.hpp"
#include "exception.hpp"
#include "string.hpp"

#include <regex>

namespace nstl::net::spf
{
namespace
{
void parse_redirect(const std::string_view /* domain_ */, const rule_cb& /*cb_*/) {}
}

void parse_spf(const std::string_view domain_, const std::string_view txt_, const rule_cb& cb_)
{
    constexpr std::string_view header{ "v=spf1" };
    constexpr std::string_view all{ "all" };
    constexpr std::string_view redirect_prefix{ "redirect=" };
    NSTL2_THROW_EXCEPTION_IF(!cb_, "empty callback");

	std::vector<std::string_view> items;
    split_view_func(txt_, ' ', [&](const std::string_view item_) { items.push_back(item_); }, true);

    NSTL2_THROW_EXCEPTION_IF(items.size() < 2, "Invalid txt record");
    NSTL2_THROW_EXCEPTION_IF(items.front() != header, "spf must be started with " << header);
    if (items.back().starts_with(redirect_prefix))
    {
        return parse_redirect(items.back().substr(redirect_prefix.size()), cb_);
    }

    NSTL2_THROW_EXCEPTION_IF(!items.back().ends_with(all), "spf should be ended with an all rule");

    const auto check_end = std::prev(items.cend());

    for (auto itr = std::next(items.cbegin()); itr != check_end; ++itr)
    {
        parse_item(domain_, *itr, cb_);
    }
}

namespace
{
using res_type = std::match_results<std::string_view::const_iterator>;
const std::regex sp_item_prefix{ R"(^([a-z0-9]+)(:([^\/]+))?(\/(\d+))?$)" };

/*/
std::string_view resolve_host(const std::string_view domain_, const res_type& result_)
{
    if (result_.size() < 4)
    {
        return domain_;
    }
    const auto& domres = result_[3];
    if (domres.first == domres.second)
    {
        return domain_;
    }

    return std::string_view{ domres.first, domres.second };
}
//*/
}

void parse_item(const std::string_view /* domain_ */, const std::string_view item_, const rule_cb& cb_)
{
    NSTL2_THROW_EXCEPTION_IF(!cb_, "empty callback");
    NSTL2_THROW_EXCEPTION_IF(item_.empty(), "item is empty");

    res_type target;
    NSTL2_THROW_EXCEPTION_IF(!std::regex_match(item_.begin(), item_.end(), target, sp_item_prefix),
                             item_ << " cannot be parsed");
    NSTL2_THROW_EXCEPTION_IF(target.size() < 2, item_ << " invalid regex result");
    const auto& item_prefix = target[1];

    if (item_prefix.compare("ip4") == 0)
    {
        NSTL2_THROW_EXCEPTION_IF(target.size() < 4, item_ << " is invalid");
        const auto range_view = 5 < target.size() ? std::string_view{ target[3].first, target[5].second }
                                                  : std::string_view{ target[3].first, target[3].second };
        auto range_opt = parse_ip4_range(range_view);
        NSTL2_THROW_EXCEPTION_IF(!range_opt, item_ << " parsing ipv4 range failed");
        cb_(std::move(*range_opt));
    }
    else if (item_prefix.compare("ip6") == 0)
    {
        NSTL2_THROW_EXCEPTION_IF(target.size() < 4, item_ << " is invalid");
        const auto range_view = 5 < target.size() ? std::string_view{ target[3].first, target[5].second }
                                                  : std::string_view{ target[3].first, target[3].second };
        auto range_opt = parse_ip6_range(range_view);
        NSTL2_THROW_EXCEPTION_IF(!range_opt, item_ << " parsing ipv6 range failed");
        cb_(std::move(*range_opt));
    }
    /*/
    else if (item_prefix.compare("mx") == 0)
    {
        const auto host = resolve_host(domain_, target);
        const auto results = nstl::net::mx_name(host);
    }
    else if (item_prefix.compare("a") == 0)
    {
        const auto host = resolve_host(domain_, target);
        const auto results = nstl::net::ips_name(host);

    }
    //*/

}
}