#include "backtrace.hpp"
#include "exception.hpp"
#include "global_init.hpp"
#include "safe_basename.hpp"
#include "scope_exit.hpp"

#include <backtrace.h>

#include <algorithm>
#include <iostream>
#include <ostream>

namespace nstl::bt
{
namespace
{
backtrace_state* bt_state{ nullptr };

void bt_error_cb(void* /*data*/, const char* msg, int errnum)
{
    std::cerr << "BT error (" << errnum << "): " << (msg ? msg : "") << std::endl;
}

int pc_cb(void* data, uintptr_t pc, const char* filename, int lineno, const char* function)
{
    auto osptr = reinterpret_cast<std::ostream*>(data);
    NSTL2_THROW_EXCEPTION_IF(osptr == nullptr, "Data pass through failed");
    auto& oss = *osptr;
    const auto fn_view = safe_basename_view(filename ? std::string_view{ filename } : std::string_view{});
    const auto func_view = function ? std::string_view{ function } : std::string_view{};
    if (!fn_view.empty())
    {
        oss << fn_view << ':' << lineno;
    }
    oss << "(" << std::hex << pc << ")";
    if (!func_view.empty())
    {
        oss << " - " << func_view;
    }
    oss << "\n";
    return 0;
}

int frame_cb(void* data, uintptr_t pc)
{
    backtrace_pcinfo(bt_state, pc, pc_cb, bt_error_cb, data);
    return 0;
}
} // namespace

void backtrace_init()
{
    if (!bt_state)
    {
        bt_state = ::backtrace_create_state(nullptr, 1, bt_error_cb, nullptr);
    }
}

std::ostream& pr_backtrace(std::ostream& os_, const std::string_view func_, const std::string_view file_,
                           const int line_)
{
    os_ << "Backtrace";
    if (!func_.empty())
    {
        os_ << " from ";
        if (!file_.empty())
        {
            os_ << file_ << ':' << line_;
        }
        os_ << " -> " << func_;
    }
    os_ << ":\n";

    if (!bt_state) [[unlikely]]
    {
        return os_;
    }
    backtrace_simple(bt_state, 1, frame_cb, bt_error_cb, &os_);

    return os_;
}
} // namespace nstl::bt
