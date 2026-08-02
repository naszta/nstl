#ifndef _NSTL_ENV_VAR_RAII
#define _NSTL_ENV_VAR_RAII 1

#include <optional>
#include <string>
#include <string_view>

namespace nstl
{
class env_var_raii
{
    std::string _name;
    std::optional<std::string> _prev;

public:
    env_var_raii(const char* name_, const char* value_);
    env_var_raii(std::string name_, const char* value_);
    ~env_var_raii();

    env_var_raii(const env_var_raii&) = delete;
    env_var_raii& operator=(const env_var_raii&) = delete;
};

std::string_view get_env_var(const char* name_);
} // namespace nstl

#endif
