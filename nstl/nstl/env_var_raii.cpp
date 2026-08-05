#include "env_var_raii.hpp"
#include "exception.hpp"

#include <format>

#include <stdlib.h>

namespace
{
bool loc_setenv(const char* name_, const char* value_)
{
#ifdef _WIN32
    return ::_putenv_s(name_, value_) == 0;
#else
    return ::setenv(name_, value_, 1) == 0;
#endif
}

bool loc_unsetenv(const char* name_)
{
#ifdef _WIN32
    const auto value = std::format("{}=", name_);
    return ::_putenv(value.c_str()) == 0;
#else
    return ::unsetenv(name_) == 0;
#endif
}
} // namespace

namespace nstl
{
env_var_raii::env_var_raii(std::string name_, const char* value_) : _name{ std::move(name_) }
{
    NSTL2_THROW_EXCEPTION_IF(_name.empty(), "enivronment variable name cannot be nullptr");
    if (const char* value = ::getenv(_name.c_str()))
    {
        _prev.emplace(value);
    }
    if (value_)
    {
        NSTL2_THROW_EXCEPTION_IF(!loc_setenv(_name.c_str(), value_), _name << " enivronment variable cannot be set");
    }
    else if (_prev.has_value())
    {
        NSTL2_THROW_EXCEPTION_IF(!loc_unsetenv(_name.c_str()), _name << " cannot be removed");
    }
    else
    {
        _noop = true;
    }
}

env_var_raii::~env_var_raii()
{
    if (_noop)
    {
        return;
    }
    else if (_prev)
    {
        loc_setenv(_name.c_str(), _prev->c_str());
    }
    else
    {
        loc_unsetenv(_name.c_str());
    }
}

std::string_view get_env_var(const char* name_)
{
    NSTL2_THROW_EXCEPTION_IF(!name_, "environment variable name cannot be nullptr");
    const char* ptr = ::getenv(name_);
    return ptr ? std::string_view{ ptr } : std::string_view{};
}
} // namespace nstl