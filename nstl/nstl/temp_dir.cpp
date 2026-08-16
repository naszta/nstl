#include "temp_dir.hpp"
#include "exception.hpp"
#include "logging.hpp"

#include <random>

#include <cstdlib>

#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#define file_open _wopen
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IRGRP 0
#else
#include <unistd.h>
#define file_open open
#endif

namespace fs = std::filesystem;

namespace
{
unsigned int get_seed(const std::optional<unsigned int>& seed)
{
    if (seed)
    {
        return seed.value();
    }

    std::random_device device;
    return device();
}
} // namespace

namespace nstl
{
fs::path random_file_name(const size_t len, const std::optional<unsigned int>& seed)
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

fs::path temp_directory_path()
{
    if (const char* raw_path = std::getenv("TMPDIR"); raw_path && raw_path[0] != '\0')
    {
        return fs::path{ raw_path };
    }
    return fs::temp_directory_path();
}

temp_dir::temp_dir(const std::optional<unsigned int>& seed_)
    : temp_dir{ temp_directory_path(), random_file_name(file_name_len, seed_) }
{
    static_assert(std::is_same_v<unsigned int, std::random_device::result_type>,
                  "Random device's output is not unsigned int");
}

temp_dir::temp_dir(const std::filesystem::path& name_) : temp_dir{ temp_directory_path(), name_ } {}

temp_dir::temp_dir(const std::filesystem::path& parent_, const std::filesystem::path& name_)
    : _target{ parent_ / name_ }, _owned{ fs::create_directories(_target) }
{
    NSTL2_THROW_EXCEPTION_IF(!_owned, _target << " cannot be created");
#ifdef __linux__
    NSTL2_THROW_EXCEPTION_IF(::chmod(_target.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) != 0, "chmod 750 failed on " << _target); // 750
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

temp_file::temp_file(std::nullopt_t) {}

temp_file::temp_file(const std::optional<unsigned int>& seed_, Mode mode_)
    : temp_file{ temp_directory_path(), random_file_name(file_name_len, seed_), mode_ }
{}

temp_file::temp_file(const std::filesystem::path& name_, Mode mode_) : temp_file{ temp_directory_path(), name_, mode_ } {}

temp_file::temp_file(const std::filesystem::path& parent_, const std::filesystem::path& name_, const Mode mode_)
    : _target{ parent_ / name_ }, _owned{ !fs::exists(_target) }, _readable{mode_ == Mode::ReadWrite}
{
    _handler.reset(
        ::file_open(_target.c_str(), (_readable ? O_RDWR | O_CREAT : O_WRONLY | O_CREAT), S_IRUSR | S_IWUSR | S_IRGRP));
    NSTL2_THROW_EXCEPTION_IF(!_handler, _target << " cannot be opened");
}

temp_file::~temp_file()
{
    if (_handler)
    {
        _handler.reset();
        if (_owned)
        {
            std::error_code ec;
            fs::remove(_target, ec);
        }
    }
}

temp_file::temp_file(temp_file&& other_) noexcept
    : _target{std::move(other_._target)}
    , _owned{std::exchange(other_._owned, false)}
    , _readable{std::exchange(other_._readable, false)}
    , _handler{std::move(other_._handler)}
{}

temp_file& temp_file::operator=(temp_file&& other_) noexcept
{
    if (this != &other_)
    {
        this->swap(other_);
        temp_file tmp{std::nullopt};
        tmp.swap(other_);
    }
    return *this;
}

void temp_file::swap(temp_file& other_)
{
    _target.swap(other_._target);
    std::swap(_owned, other_._owned);
    std::swap(_readable, other_._readable);
    _handler.swap(other_._handler);
}

bool temp_file::valid() const
{
    return !!_handler;
}

const std::filesystem::path& temp_file::path() const
{
    return _target;
}
} // namespace nstl
