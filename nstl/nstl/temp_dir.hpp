#ifndef _NSTL_TEMP_DIR
#define _NSTL_TEMP_DIR 1

#include <filesystem>
#include <optional>

namespace nstl
{
class temp_dir
{
    const std::filesystem::path _target;
    const bool _owned{ false };

public:
    explicit temp_dir(const std::optional<unsigned int> seed_ = std::nullopt);
    explicit temp_dir(const std::filesystem::path& name_);
    explicit temp_dir(const std::filesystem::path& parent_, const std::filesystem::path& name_);
    ~temp_dir();

    temp_dir(const temp_dir&) = delete;
    temp_dir& operator=(const temp_dir&) = delete;

    const std::filesystem::path& path() const;

    operator const std::filesystem::path& () const
    {
        return this->path();
    }
};

inline std::filesystem::path operator/(const temp_dir& dir, const std::filesystem::path& right_)
{
    return dir.path() / right_;
}
}

#endif
