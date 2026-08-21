#ifndef _NSTL_HANDLE_RAII
#define _NSTL_HANDLE_RAII 1

#include <algorithm>
#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef _WIN32
extern "C"
{
    typedef void* HANDLE;
}
#endif

namespace nstl
{
template <class Type>
concept trivial = std::is_trivial_v<Type>;

template <class Type>
concept not_boolean = !std::is_same_v<bool, Type>;

template <class Type, class Tools>
    requires(trivial<Type> && not_boolean<Type>)
class custom_raii
{
    Type _hndlr{ invalid_value };
    Tools _tools;

public:
    using handle_type = Type;
    static constexpr Type invalid_value = Tools::invalid_value;

    explicit custom_raii(Type hndlr_ = invalid_value) : _hndlr{ hndlr_ } {}
    ~custom_raii() { reset(); }
    custom_raii(const custom_raii& other_) = delete;
    custom_raii& operator=(const custom_raii& other_) = delete;
    custom_raii(custom_raii&& other_) noexcept : _hndlr{ std::exchange(other_._hndlr, invalid_value) } {}
    custom_raii& operator=(custom_raii&& other_) noexcept
    {
        if (this != &other_)
        {
            this->reset();
            this->swap(other_);
        }
        return *this;
    }

    void reset(Type hndlr_ = invalid_value)
    {
        if (_tools.valid(_hndlr))
        {
            _tools.free(_hndlr);
            _hndlr = invalid_value;
        }
        _hndlr = hndlr_;
    }

    Type release() { return std::exchange(_hndlr, invalid_value); }

    explicit operator bool() const { return _tools.valid(_hndlr); }
    explicit operator Type() const { return _hndlr; }

    void swap(custom_raii& other_) { std::swap(_hndlr, other_._hndlr); }
    void swap(Type& other_) { std::swap(_hndlr, other_); }

    friend bool operator==(const custom_raii& left_, const custom_raii& right_)
    {
        return left_._hndlr == right_._hndlr;
    }

    friend bool operator<(const custom_raii& left_, const custom_raii& right_) { return left_._hndlr < right_._hndlr; }

    friend bool operator==(const Type& left_, const custom_raii& right_) { return left_ == right_._hndlr; }

    friend bool operator<(const Type& left_, const custom_raii& right_) { return left_ < right_._hndlr; }

    friend bool operator==(const custom_raii& left_, const Type& right_) { return left_._hndlr == right_; }

    friend bool operator<(const custom_raii& left_, const Type& right_) { return left_._hndlr < right_; }
};

#ifdef _WIN32
struct WindowsHandle
{
    static constexpr HANDLE invalid_value = reinterpret_cast<HANDLE>(std::int64_t{ -1 });
    WindowsHandle();
    bool valid(HANDLE hnd_) const;
    void free(HANDLE hnd_) const;
};

using HandleRaii = custom_raii<HANDLE, WindowsHandle>;
#endif

struct FileIntHandle
{
    static constexpr int invalid_value = -1;
    FileIntHandle();
    bool valid(int hnd_) const;
    void free(int hnd_) const;
};

using FileIntRaii = custom_raii<int, FileIntHandle>;
} // namespace nstl

#endif
