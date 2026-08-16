#ifndef _NSTL_TEMP_DIR
#define _NSTL_TEMP_DIR 1

#include <nstl/handle_raii.hpp>

#include <filesystem>
#include <optional>

namespace nstl
{
constexpr size_t file_name_len = 8;

std::filesystem::path temp_directory_path();
std::filesystem::path random_file_name(size_t len = file_name_len, const std::optional<unsigned int>& seed_ = std::nullopt);

class temp_dir
{
    const std::filesystem::path _target;
    const bool _owned{ false };

public:
    explicit temp_dir(const std::optional<unsigned int>& seed_ = std::nullopt);
    explicit temp_dir(const std::filesystem::path& name_);
    explicit temp_dir(const std::filesystem::path& parent_, const std::filesystem::path& name_);
    ~temp_dir();

    temp_dir(const temp_dir&) = delete;
    temp_dir& operator=(const temp_dir&) = delete;

    const std::filesystem::path& path() const;

    operator const std::filesystem::path&() const { return this->path(); }
};


class temp_file
{
    std::filesystem::path _target;
    bool _owned{ false };
    bool _readable{ false };
    FileIntRaii _handler;

    explicit temp_file(std::nullopt_t);
public:
    using handle_type = FileIntRaii::handle_type;

    enum class Mode { Write, ReadWrite };

    explicit temp_file(const std::optional<unsigned int>& seed_ = std::nullopt, Mode mode_ = Mode::Write);
    explicit temp_file(const std::filesystem::path& name_, Mode mode_ = Mode::Write);
    explicit temp_file(const std::filesystem::path& parent_, const std::filesystem::path& name_, Mode mode_ = Mode::Write);
    ~temp_file();

    temp_file(const temp_file&) = delete;
    temp_file& operator=(const temp_file&) = delete;

    temp_file(temp_file&& other_) noexcept;
    temp_file& operator=(temp_file&& other_) noexcept;

    bool is_readable() const { return _readable; }
    void swap(temp_file& other_);
    bool valid() const;

    const std::filesystem::path& path() const;

    explicit operator handle_type() const { return static_cast<handle_type>(this->_handler); }
    explicit operator const std::filesystem::path&() const { return this->path(); }
};

inline std::filesystem::path operator/(const temp_dir& dir, const std::filesystem::path& right_)
{
    return dir.path() / right_;
}
} // namespace nstl

#endif
