#include "backtrace.hpp"
#include "safe_basename.hpp"
#include "scope_exit.hpp"

#include <array>
#include <ostream>

#include <Windows.h>
#include <dbghelp.h>

namespace nstl::bt
{
struct GlobalState
{
    HANDLE proc{ INVALID_HANDLE_VALUE };

    GlobalState() : proc{ ::GetCurrentProcess() }
    {
        ::SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        ::SymInitialize(proc, NULL, TRUE);
    }
    ~GlobalState() { ::SymCleanup(proc); }
};

void backtrace_init() { static const GlobalState instance; }

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

    HANDLE proc = ::GetCurrentProcess();

    constexpr DWORD addr_size = 64;
    constexpr DWORD syms_size = 256;

    std::array<void*, addr_size> addrs;
    const WORD n = CaptureStackBackTrace(1 /*skip this fn*/, addr_size - 2, addrs.data(), NULL);

    std::array<char, sizeof(SYMBOL_INFO) + syms_size> buffer;
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(buffer.data());
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = syms_size - 1;

    for (WORD idx = 0; idx < n; ++idx)
    {
        DWORD64 pc = reinterpret_cast<DWORD64>(addrs[idx]);
        DWORD64 disp = 0;
        const char* name = SymFromAddr(proc, pc, &disp, sym) ? sym->Name : "??";
        IMAGEHLP_LINE64 line = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
        DWORD lineDisp = 0;
        const auto name_view = name ? std::string_view{ name } : std::string_view{ "{{}}" };

        if (SymGetLineFromAddr64(proc, pc, &lineDisp, &line))
        {
            const auto fname = safe_basename_view(line.FileName);
            os_ << fname << ":" << line.LineNumber << " - " << name_view << "\n";
        }
        else
        {
            os_ << std::hex << pc << " - " << name_view << "\n";
        }
    }

    return os_;
}
} // namespace nstl::bt