#ifndef _NSTL_EXCEPTION
#define _NSTL_EXCEPTION 1

#include <nstl/safe_basename.hpp>

#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace nstl
{
class exception : public std::runtime_error
{
public:
    explicit exception(const std::string& val_, const char* file_ = nullptr, int line_ = 0);
    explicit exception(const char* val_, const char* file_ = nullptr, int line_ = 0);

    std::string_view file() const;
    int line() const;

private:
    const char* _file{ nullptr };
    const int _line{ 0 };
};

std::ostream& operator<<(std::ostream& os_, const exception& exc_);
} // namespace nstl

#define NSTL2_THROW_EXCEPTION_IMPL(error, detail)                                             \
    do                                                                                        \
    {                                                                                         \
        std::ostringstream _oss__;                                                            \
        _oss__ << ::nstl::safe_basename_view(__FILE__) << ':' << __LINE__ << " - " << detail; \
        throw ::nstl::exception{ _oss__.str(), __FILE__, __LINE__ };                          \
    } while (false)

#define NSTL2_THROW_EXCEPTION(detail) NSTL2_THROW_EXCEPTION_IMPL(::nstl::exception, detail)

#define NSTL2_THROW_EXCEPTION_IF_IMPL(cond, error, detail) \
    if (cond) [[unlikely]]                                 \
    {                                                      \
        NSTL2_THROW_EXCEPTION_IMPL(error, detail);         \
    }

#define NSTL2_THROW_EXCEPTION_IF(cond, detail) NSTL2_THROW_EXCEPTION_IF_IMPL(cond, ::nstl::exception, detail)

#define NSTL2_NESTED_THROW_EXCEPTION_IMPL(error, detail)                                      \
    do                                                                                        \
    {                                                                                         \
        std::ostringstream _oss__;                                                            \
        _oss__ << ::nstl::safe_basename_view(__FILE__) << ':' << __LINE__ << " - " << detail; \
        std::throw_with_nested(::nstl::exception{ _oss__.str(), __FILE__, __LINE__ });        \
    } while (false)

#define NSTL2_NESTED_THROW_EXCEPTION(detail) NSTL2_NESTED_THROW_EXCEPTION_IMPL(::nstl::exception, detail)

#endif
