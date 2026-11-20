#ifndef _NSTL_SPF_TOOLS
#define _NSTL_SPF_TOOLS 1

#include <nstl/ip_tools.hpp>

#include <functional>
#include <string_view>
#include <vector>

namespace nstl::net::spf
{
using rules_type = std::vector<ip_range_gen>;
using rule_cb = std::function<void (ip_range_gen range_)>;

void parse_spf(std::string_view domain_, std::string_view txt_, const rule_cb& cb_);
void parse_item(std::string_view domain_, std::string_view item_, const rule_cb& cb_);
}

#endif
