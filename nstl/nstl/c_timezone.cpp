#include "c_timezone.hpp"
#include "exception.hpp"

#include <stdexcept>

#include <time.h>

namespace nstl
{
namespace time
{
struct std::tm* localtime_r(const std::time_t* src, struct std::tm* tgt)
{
#ifdef _WIN32
    return ::localtime_s(tgt, src) == 0 ? tgt : nullptr;
#else
    return ::localtime_r(src, tgt);
#endif
}

struct std::tm* gmtime_r(const std::time_t* src, struct std::tm* tgt)
{
#ifdef _WIN32
    return ::gmtime_s(tgt, src) == 0 ? tgt : nullptr;
#else
    return ::gmtime_r(src, tgt);
#endif
}

std::time_t timegm(struct std::tm* src)
{
#ifdef _WIN32
    return ::_mkgmtime(src);
#else
    return ::timegm(src);
#endif
}

std::time_t mktime(struct std::tm* src) { return ::mktime(src); }
} // namespace time

date::sys_info c_timezone::_get_sys_info(const date::sys_seconds& sys_secs_, const date::local_seconds& local_secs_,
                                         const int is_dst_)
{
    return date::sys_info{
        .begin = sys_secs_,
        .end = sys_secs_ + std::chrono::seconds{ 1 },
        .offset = local_secs_.time_since_epoch() - sys_secs_.time_since_epoch(),
        .save = std::chrono::hours{ is_dst_ < 0 ? 0 : is_dst_ },
        .abbrev = std::string{},
    };
}

std::pair<date::local_seconds, int> c_timezone::to_local_common(const date::sys_seconds& utc_seconds_)
{
    const ::time_t utc_time_t = utc_seconds_.time_since_epoch().count();

    struct tm localtm;
    NSTL2_THROW_EXCEPTION_IF(!time::localtime_r(&utc_time_t, &localtm), "date cannot be calculated");
    const auto ret_isdst = localtm.tm_isdst;
    localtm.tm_isdst = 0;
    const auto local_time_t = time::timegm(&localtm);
    NSTL2_THROW_EXCEPTION_IF(local_time_t == -1, "epoch cannot be calculated");
    return std::make_pair(date::local_seconds{ std::chrono::seconds{ local_time_t } }, ret_isdst);
}

std::pair<date::sys_seconds, int> c_timezone::to_sys_common(const date::local_seconds& local_seconds_)
{
    const ::time_t local_time_t = local_seconds_.time_since_epoch().count();

    struct tm localtm;
    NSTL2_THROW_EXCEPTION_IF(!time::gmtime_r(&local_time_t, &localtm), "date cannot be calculated");
    localtm.tm_isdst = -1;
    const auto utc_time_t = time::mktime(&localtm);
    NSTL2_THROW_EXCEPTION_IF(utc_time_t == -1, "epoch cannot be calculated");
    return std::make_pair(date::sys_seconds{ std::chrono::seconds{ utc_time_t } }, localtm.tm_isdst);
}
} // namespace nstl
