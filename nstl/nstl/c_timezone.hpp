#ifndef _NSTL_C_TIMEZONE
#define _NSTL_C_TIMEZONE 1

#include <nstl/compiler.hpp>

#include <chrono>
#include <ctime>
#include <string>
#include <string_view>
#include <type_traits>

#ifdef NSTL_USING_HH_DATE
NSTL_WRN_DATE_PUSH
#include <date/date.h>
#include <date/tz.h>
NSTL_WRN_DATE_POP
#else
namespace date = std::chrono;
#endif

namespace nstl
{
namespace time
{
struct std::tm* localtime_r(const std::time_t* src, struct std::tm* tgt);
struct std::tm* gmtime_r(const std::time_t* src, struct std::tm* tgt);
std::time_t timegm(struct std::tm* src);
std::time_t mktime(struct std::tm* src);
} // namespace time

class c_timezone
{
    std::string _name;

    static date::sys_info _get_sys_info(const date::sys_seconds& sys_secs_, const date::local_seconds& local_secs_,
                                        const int is_dst_);
    static std::pair<date::local_seconds, int> to_local_common(const date::sys_seconds& tp_);
    static std::pair<date::sys_seconds, int> to_sys_common(const date::local_seconds& tp_);

public:
    explicit c_timezone(std::string name_ = "current") : _name{ std::move(name_) } {}

    std::string_view name() const noexcept { return _name; }

    template <class Duration>
    auto to_sys(const date::local_time<Duration>& tp_) const
        -> std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
    {
        const auto local_seconds = date::floor<std::chrono::seconds>(tp_);
        const auto local_sub_seconds = tp_ - local_seconds;

        const auto [utc_seconds, _] = this->to_sys_common(local_seconds);
        return utc_seconds + local_sub_seconds;
    }

    template <class Duration>
    auto to_sys(const date::local_time<Duration>& tp_, date::choose) const
        -> std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
    {
        return this->to_sys(tp_);
    }

    template <class Duration> date::local_info get_info(const date::local_time<Duration>& tp) const
    {
        const auto local_secs = date::floor<std::chrono::seconds>(tp);
        const auto [sys_secs, is_dst] = this->to_sys_common(local_secs);

        return date::local_info{ .result = date::local_info::unique,
                                 .first = this->_get_sys_info(sys_secs, local_secs, is_dst),
                                 .second = date::sys_info{
                                     .begin = date::sys_seconds{ std::chrono::seconds::zero() },
                                     .end = date::sys_seconds{ std::chrono::seconds::zero() },
                                     .offset = std::chrono::seconds::zero(),
                                     .save = std::chrono::minutes::zero(),
                                     .abbrev = std::string{},
                                 } };
    }

    template <class Duration>
    auto to_local(const std::chrono::sys_time<Duration>& tp_) const
        -> date::local_time<std::common_type_t<Duration, std::chrono::seconds>>
    {
        const auto utc_seconds = date::floor<std::chrono::seconds>(tp_);
        const auto utc_sub_seconds = tp_ - utc_seconds;

        const auto [local_seconds, _] = this->to_local_common(utc_seconds);
        return local_seconds + utc_sub_seconds;
    }

    template <class Duration> date::sys_info get_info(const std::chrono::sys_time<Duration>& tp) const
    {
        const auto sys_secs = date::floor<std::chrono::seconds>(tp);
        const auto [local_secs, is_dst] = this->to_local_common(sys_secs);
        return this->_get_sys_info(sys_secs, local_secs, is_dst);
    }
};

inline bool operator==(const c_timezone& _Left, const c_timezone& _Right) noexcept
{
    return _Left.name() == _Right.name();
}

inline std::strong_ordering operator<=>(const c_timezone& _Left, const c_timezone& _Right) noexcept
{
    return _Left.name() <=> _Right.name();
}
} // namespace nstl

#endif
