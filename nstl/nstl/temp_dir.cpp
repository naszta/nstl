#include "temp_dir.hpp"
#include "exception.hpp"
#include "logging.hpp"

#include <random>

#include <cstdlib>

#ifdef __linux__
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace
{
fs::path get_temp_path()
{
    if (const char* raw_path = std::getenv("TMPDIR"); raw_path && raw_path[0] != '\0')
    {
        return fs::path{ raw_path };
    }
    return fs::temp_directory_path();
}

unsigned int get_seed(const std::optional<unsigned int>& seed)
{
    if (seed)
    {
        return seed.value();
    }

    std::random_device device;
    return device();
}

fs::path random_name(const size_t len, const std::optional<unsigned int>& seed)
{
    thread_local std::default_random_engine engine{ get_seed(seed) };

    NSTL2_THROW_EXCEPTION_IF(len == 0, "0 name length doesn't make sense");

    std::uniform_int_distribution<int> dist{ 'a', 'z' };

    std::string retval;
    retval.reserve(len);

    for (size_t idx = 0; idx < len; ++idx)
    {
        retval.push_back(static_cast<char>(dist(engine)));
    }
    return retval;
}

constexpr size_t dir_name_len = 8;
} // namespace

namespace nstl
{
temp_dir::temp_dir(const std::optional<unsigned int> seed_)
    : temp_dir{ get_temp_path(), random_name(dir_name_len, seed_) }
{
    static_assert(std::is_same_v<unsigned int, std::random_device::result_type>,
                  "Random device's output is not unsigned int");
}

temp_dir::temp_dir(const std::filesystem::path& name_) : temp_dir{ get_temp_path(), name_ } {}

temp_dir::temp_dir(const std::filesystem::path& parent_, const std::filesystem::path& name_)
    : _target{ parent_ / name_ }, _owned{ fs::create_directories(_target) }
{
    NSTL2_THROW_EXCEPTION_IF(!_owned, _target << " cannot be created");
#ifdef __linux__
    ::chmod(_target.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP);
#endif
    NSTL_DEBUG(_target << " created");
}

temp_dir::~temp_dir()
{
    if (!_owned) [[unlikely]]
    {
        return;
    }
    std::error_code ec;
    fs::remove_all(_target, ec);
    if (ec)
    {
        NSTL_ERROR(_target << " cannot be deleted");
    }
    else
    {
        NSTL_DEBUG(_target << " deleted");
    }
}

const std::filesystem::path& temp_dir::path() const { return _target; }
} // namespace nstl
