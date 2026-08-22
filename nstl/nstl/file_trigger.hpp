#ifndef _NSTL_FILE_TRIGGER
#define _NSTL_FILE_TRIGGER 1

#include <filesystem>
#include <functional>
#include <memory>
#include <span>

#ifdef _WIN32
extern "C" { using HANDLE = void*; }
#endif

namespace nstl
{
class file_trigger : public std::enable_shared_from_this<file_trigger>
{
protected:
    file_trigger();

public:
#ifdef _WIN32
    using native_handle = HANDLE;
#else
    using native_handle = int;
#endif
    using data_cb = std::function<void(std::span<char> data_)>;

    static std::shared_ptr<file_trigger> factory(const std::filesystem::path& file_, data_cb cb_, bool sow_ = false, size_t buffer_size_ = 1000000);
    static std::shared_ptr<file_trigger> factory(native_handle handle_, data_cb cb_, bool sow_ = false,
                                                 size_t buffer_size_ = 1000000);

    file_trigger(const file_trigger&) = delete;
    file_trigger& operator=(const file_trigger&) = delete;

    virtual ~file_trigger() = 0;
    virtual void stop() = 0;
};

nstl::file_trigger::native_handle open_native(const std::filesystem::path& path_);
}

#endif
