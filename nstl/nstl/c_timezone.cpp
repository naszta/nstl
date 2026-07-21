#include "nstl/c_timezone.hpp"

#include <stdexcept>

#include <time.h>

namespace nstl
{
#ifdef _WIN32
namespace
{
    struct ::tm* localtime_r(const ::time_t *src, struct ::tm *tgt)
    {
        return ::localtime_s(tgt, src) == 0 ? tgt : nullptr;
    }

    struct ::tm* gmtime_r(const ::time_t *src, struct ::tm *tgt)
    {
        return ::gmtime_s(tgt, src) == 0 ? tgt : nullptr;
    }

    ::time_t timegm(struct ::tm* src)
    {
        return ::_mkgmtime(src);
    }
}
#endif

std::chrono::sys_info c_timezone::_get_sys_info(const std::chrono::sys_seconds& sys_secs_, const std::chrono::local_seconds& local_secs_, const int is_dst_) const
{
    return std::chrono::sys_info{
        .begin = sys_secs_,
        .end = sys_secs_ + std::chrono::seconds{1},
        .offset = local_secs_.time_since_epoch() - sys_secs_.time_since_epoch(),
        .save = std::chrono::hours{ is_dst_ < 0 ? 0 : is_dst_ },
        .abbrev = std::string{},
    };
}

std::pair<std::chrono::local_seconds, int> c_timezone::to_local_common(const std::chrono::sys_seconds& utc_seconds_) const
{
    const ::time_t utc_time_t = utc_seconds_.time_since_epoch().count();

    struct tm localtm;
    if (!localtime_r(&utc_time_t, &localtm)) [[unlikely]] {
        throw std::runtime_error{ "date cannot be calculated" };
    }
    const auto ret_isdst = localtm.tm_isdst;
    localtm.tm_isdst = 0;
    const auto local_time_t = timegm(&localtm);
    if (local_time_t == -1) [[unlikely]] {
        throw std::runtime_error{ "epoch cannot be calculated" };
    }

    return std::make_pair(std::chrono::local_seconds{std::chrono::seconds{local_time_t}}, ret_isdst);
}

std::pair<std::chrono::sys_seconds, int> c_timezone::to_sys_common(const std::chrono::local_seconds& local_seconds_) const
{
    const ::time_t local_time_t = local_seconds_.time_since_epoch().count();

    struct tm localtm;
    if (!gmtime_r(&local_time_t, &localtm)) [[unlikely]] {
        throw std::runtime_error{ "date cannot be calculated" };
    }
    localtm.tm_isdst = -1;
    const auto utc_time_t = mktime(&localtm);
    if (utc_time_t == -1) [[unlikely]] {
        throw std::runtime_error{ "epoch cannot be calculated" };
    }

    return std::make_pair(std::chrono::sys_seconds{std::chrono::seconds{utc_time_t}}, localtm.tm_isdst);
}
}
