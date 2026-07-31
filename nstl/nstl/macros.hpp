#ifndef _NSTL_MACROS
#define _NSTL_MACROS 1

#include <nstl/safe_basename.hpp>

#include <sstream>

#define NSTL_THROW_EXCEPTION(error, detail)                                                   \
    do                                                                                        \
    {                                                                                         \
        std::ostringstream _oss__;                                                            \
        _oss__ << ::nstl::safe_basename_view(__FILE__) << ':' << __LINE__ << " - " << detail; \
        throw error{ _oss__.str() };                                                          \
    } while (false)

#define NSTL_THROW_EXCEPTION_IF(cond, error, detail) \
    if (cond) [[unlikely]]                           \
    {                                                \
        NSTL_THROW_EXCEPTION(error, detail);         \
    }

#endif
