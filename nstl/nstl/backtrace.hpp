#ifndef _NSTL_BACKTRACE
#define _NSTL_BACKTRACE 1

#include <iosfwd>
#include <string_view>

namespace nstl::bt
{
void backtrace_init();
std::ostream& pr_backtrace(std::ostream& os_,
    std::string_view func_ = std::string_view{},
    std::string_view file_ = std::string_view{},
    int line_ = 0);
}

#endif
