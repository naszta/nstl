#ifndef __C_TIMEZONE
#define __C_TIMEZONE 1

#include <chrono>
#include <string>
#include <string_view>
#include <type_traits>

namespace nstl
{
    class c_timezone
    {
        std::string _name;

        std::chrono::sys_info _get_sys_info(const std::chrono::sys_seconds& sys_secs_, const std::chrono::local_seconds& local_secs_, const int is_dst_) const;
        std::pair<std::chrono::local_seconds, int> to_local_common(const std::chrono::sys_seconds& tp_) const;
        std::pair<std::chrono::sys_seconds, int> to_sys_common(const std::chrono::local_seconds& tp_) const;

    public:
        explicit c_timezone(std::string name_ = "current") : _name{std::move(name_)} {}

        std::string_view name() const noexcept { return _name; }

        template <class Duration>
        auto to_sys(const std::chrono::local_time<Duration>& tp_) const -> std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
        {
            const auto local_seconds = std::chrono::floor<std::chrono::seconds>(tp_);
            const auto local_sub_seconds = tp_ - local_seconds;

            const auto [utc_seconds, _] = this->to_sys_common(local_seconds);
            return utc_seconds + local_sub_seconds;
        }

        template <class Duration>
        auto to_sys(const std::chrono::local_time<Duration>& tp_, std::chrono::choose) const -> std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>
        {
            return this->to_sys(tp_);
        }

        template< class Duration >
        std::chrono::local_info get_info( const std::chrono::local_time<Duration>& tp ) const
        {
            const auto local_secs = std::chrono::floor<std::chrono::seconds>(tp);
            const auto [sys_secs, is_dst] = this->to_sys_common(local_secs);

            return std::chrono::local_info{
                .result = std::chrono::local_info::unique,
                .first = this->_get_sys_info(sys_secs, local_secs, is_dst),
                .second= std::chrono::sys_info{
                    .begin = std::chrono::sys_seconds{std::chrono::seconds::zero()},
                    .end = std::chrono::sys_seconds{std::chrono::seconds::zero()},
                    .offset = std::chrono::seconds::zero(),
                    .save = std::chrono::minutes::zero(),
                    .abbrev = std::string{},
                }
            };
        }

        template <class Duration>
        auto to_local(const std::chrono::sys_time<Duration>& tp_) const -> std::chrono::local_time<std::common_type_t<Duration, std::chrono::seconds>>
        {
            const auto utc_seconds = std::chrono::floor<std::chrono::seconds>(tp_);
            const auto utc_sub_seconds = tp_ - utc_seconds;

            const auto [local_seconds, _] = this->to_local_common(utc_seconds);
            return local_seconds + utc_sub_seconds;
        }

        template< class Duration >
        std::chrono::sys_info get_info( const std::chrono::sys_time<Duration>& tp ) const
        {
            const auto sys_secs = std::chrono::floor<std::chrono::seconds>(tp);
            const auto [local_secs, is_dst] = this->to_local_common(sys_secs);
            return this->_get_sys_info(sys_secs, local_secs, is_dst);
        }
    };

    inline bool operator==(const c_timezone& _Left, const c_timezone& _Right) noexcept {
        return _Left.name() == _Right.name();
    }

    inline std::strong_ordering operator<=>(const c_timezone& _Left, const c_timezone& _Right) noexcept {
        return _Left.name() <=> _Right.name();
    }
}

#endif
