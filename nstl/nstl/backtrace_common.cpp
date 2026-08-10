#include "backtrace.hpp"

#include <ostream>

namespace nstl::bt
{
void backtrace_init() {}

std::ostream& pr_backtrace(std::ostream& os_, const std::string_view func_, const std::string_view file_,
                           const int line_)
{
    if (!func_.empty())
    {
        os_ << "Backtrace ";
        if (!file_.empty())
        {
            os_ << file_ << ':' << line_;
        }
        os_ << " -> " << func_;
    }
    return os_;
}
} // namespace nstl::bt