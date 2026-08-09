#include "signal_barrier.hpp"
#include "exception.hpp"

#include <csignal>
#include <ostream>

namespace nstl
{

SigVal from_signal_conv(const int value_)
{
    switch (value_)
    {
    case SIGINT:
        return SigVal::SigInt;
    case SIGTERM:
        return SigVal::SigTerm;
#ifdef SIGQUIT
    case SIGQUIT:
        return SigVal::SigQuit;
#endif
    default:
        NSTL2_THROW_EXCEPTION(value_ << " signal cannot be converted to SigVal");
    }
}

int to_signal_conv(SigVal value_) { return static_cast<int>(value_); }

std::ostream& operator<<(std::ostream& os_, const SigVal sig_)
{
    switch (sig_) {
    case SigVal::SigInt:
        os_ << "SIGINT";
        return os_;
    case SigVal::SigTerm:
        os_ << "SIGTERM";
        return os_;
    case SigVal::SigQuit:
        os_ << "SIGQUIT";
        return os_;
    case SigVal::SigLogoff:
        os_ << "SIGLOGOFF";
        return os_;
    case SigVal::SigShutdown:
        os_ << "SIGSHUTDOWN";
        return os_;
    case SigVal::Unknown:
    default:
        os_ << "Unknown";
        return os_;
    }
}
}
