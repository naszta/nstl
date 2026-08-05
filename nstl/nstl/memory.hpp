#ifndef _NSTL_MEMORY
#define _NSTL_MEMORY 1

#include <cstddef>
#include <functional>
#include <type_traits>
#include <memory>
#include <utility>

namespace nstl
{

template <class Type> class observer_ptr
{
    Type* _ptr{ nullptr };

public:
    using element_type = Type;
    using pointer = std::add_pointer_t<Type>;
    using reference = std::add_lvalue_reference_t<Type>;

    // --- Construction ------------------------------------------------------

    constexpr observer_ptr() noexcept = default;

    constexpr observer_ptr(std::nullptr_t) noexcept : observer_ptr{} {}

    explicit constexpr observer_ptr(pointer p) noexcept : _ptr{ p } {}

    template <typename Type2,
              typename = std::enable_if_t<std::is_convertible<std::add_pointer_t<Type2>, pointer>::value>>
    constexpr observer_ptr(observer_ptr<Type2> other) noexcept : _ptr{ other.get() }
    {
    }

    template <class Deleter = std::default_delete<Type>>
    constexpr observer_ptr(const std::unique_ptr<Type, Deleter>& other) noexcept : _ptr{ other.get() }
    {
    }
    constexpr observer_ptr(const std::shared_ptr<Type>& other) noexcept : _ptr{ other.get() } {}

    constexpr observer_ptr(const observer_ptr&) noexcept = default;
    constexpr observer_ptr& operator=(const observer_ptr&) noexcept = default;
    constexpr ~observer_ptr() noexcept = default;

    constexpr pointer get() const noexcept { return _ptr; }

    constexpr reference operator*() const { return *_ptr; }
    constexpr pointer operator->() const noexcept { return _ptr; }

    constexpr explicit operator bool() const noexcept { return !!_ptr; }
    constexpr explicit operator pointer() const noexcept { return _ptr; }

    constexpr pointer release() noexcept
    {
        pointer p = nullptr;
        std::swap(_ptr, p);
        return p;
    }

    constexpr void reset(pointer p = nullptr) noexcept { _ptr = p; }

    constexpr void swap(observer_ptr& other) noexcept { std::swap(_ptr, other._ptr); }
};

template <typename Type> inline constexpr observer_ptr<Type> make_observer(Type* p) noexcept
{
    return observer_ptr<Type>{ p };
}

template <typename Type> inline constexpr observer_ptr<Type> make_observer(const std::unique_ptr<Type>& p) noexcept
{
    return observer_ptr<Type>{ p.get() };
}

template <typename Type> inline constexpr observer_ptr<Type> make_observer(const std::shared_ptr<Type>& p) noexcept
{
    return observer_ptr<Type>{ p.get() };
}

template <typename Type1, typename Type2>
inline constexpr bool operator==(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    return a.get() == b.get();
}

template <typename Type1, typename Type2>
inline constexpr bool operator!=(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    return a.get() != b.get();
}

template <typename Type> inline constexpr bool operator==(const observer_ptr<Type> p, std::nullptr_t) noexcept
{
    return !p;
}

template <typename Type> inline constexpr bool operator==(std::nullptr_t, const observer_ptr<Type> p) noexcept
{
    return !p;
}

template <typename Type> inline constexpr bool operator!=(const observer_ptr<Type> p, std::nullptr_t) noexcept
{
    return static_cast<bool>(p);
}

template <typename Type> inline constexpr bool operator!=(std::nullptr_t, const observer_ptr<Type> p) noexcept
{
    return static_cast<bool>(p);
}

template <typename Type1, typename Type2>
inline constexpr bool operator<(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    using CT = std::common_type_t<std::add_pointer_t<Type1>, std::add_pointer_t<Type2>>;
    return std::less<CT>()(a.get(), b.get());
}

template <typename Type1, typename Type2>
inline constexpr bool operator>(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    return b < a;
}

template <typename Type1, typename Type2>
inline constexpr bool operator<=(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    return !(b < a);
}

template <typename Type1, typename Type2>
inline constexpr bool operator>=(const observer_ptr<Type1> a, const observer_ptr<Type2> b)
{
    return !(a < b);
}

template <class Type> using leaking_ptr = observer_ptr<Type>;

} // namespace nstl

namespace std
{
template <typename Type> struct hash<nstl::observer_ptr<Type>>
{
    std::size_t operator()(const nstl::observer_ptr<Type>& p) const noexcept
    {
        return std::hash<typename nstl::observer_ptr<Type>::pointer>{}(p.get());
    }
};
} // namespace std

#endif
