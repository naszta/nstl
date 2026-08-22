#ifndef __NSTL_VECTOR
#define __NSTL_VECTOR 1

#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstdlib>

namespace nstl
{
template <class Type>
concept trivially_copyable = std::is_trivially_copyable_v<Type>;

template <class Type>
    requires(trivially_copyable<Type>)
class vector
{
    Type* _ptr{ nullptr };
    size_t _size{ 0 };
    size_t _capacity{ 0 };

    Type* _mem_reserve(const size_t new_capacity_)
    {
        Type* ptr = reinterpret_cast<Type*>(std::realloc(_ptr, new_capacity_ * sizeof(Type)));
        if (ptr == nullptr) [[unlikely]]
        {
            throw std::bad_alloc{};
        }
        _ptr = ptr;
        _capacity = new_capacity_;
        return ptr;
    }

    Type* _auto_capacity(const size_t needed_)
    {
        if (needed_ <= _capacity) [[likely]]
        {
            return _ptr;
        }
        if (needed_ < 16)
        {
            return this->_mem_reserve(16);
        }
        else
        {
            return this->_mem_reserve(std::max(needed_, _capacity * 2));
        }
    }

public:
    using value_type = Type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = Type&;
    using const_reference = const Type&;
    using pointer = Type*;
    using const_pointer = const Type*;
    using iterator = Type*;
    using const_iterator = const Type*;

    vector() = default;
    ~vector() { this->reset(); }
    vector(const vector& other_)
    {
        if (other_.empty())
        {
            return;
        }
        auto ptr = this->_auto_capacity(other_.size());
        std::memcpy(ptr, other_.data(), other_.size() * sizeof(Type));
        _size = other_.size();
    }
    vector& operator=(const vector& other_)
    {
        if (this != &other_)
        {
            vector other{ other_ };
            this->swap(other);
        }
        return *this;
    }

    vector(vector&& other_) noexcept { this->swap(other_); }
    vector& operator=(vector&& other_) noexcept
    {
        if (this != &other_)
        {
            this->reset();
            this->swap(other_);
        }
        return *this;
    }

    void swap(vector& other_)
    {
        std::swap(_ptr, other_._ptr);
        std::swap(_size, other_._size);
        std::swap(_capacity, other_._capacity);
    }

    void clear()
    {
        for (; 0 < _size; --_size)
        {
            const auto idx = _size - 1;
            (_ptr + idx)->Type::~Type();
        }
    }

    void push_back(const Type& val_)
    {
        const auto new_size = _size + 1;
        auto ptr = this->_auto_capacity(new_size);
        new (ptr + _size) Type{ val_ };
        _size = new_size;
    }

    void push_back(Type&& val_)
    {
        const auto new_size = _size + 1;
        auto ptr = this->_auto_capacity(new_size);
        new (ptr + _size) Type{ std::move(val_) };
        _size = new_size;
    }

    void pop_back()
    {
        if (this->empty()) [[unlikely]]
        {
            throw std::runtime_error{ "container is empty" };
        }
        (_ptr + _size - 1)->Type::~Type();
        --_size;
    }

    void reset()
    {
        this->clear();
        std::free(_ptr);
        _capacity = 0;
        _ptr = nullptr;
    }

    void reserve(const size_t new_capacity_) { _auto_capacity(new_capacity_); }

    reference at(const size_type idx)
    {
        if (idx < _size) [[likely]]
        {
            return _ptr[idx];
        }
        throw std::out_of_range{ "index out of range" };
    }

    const_reference at(const size_type idx) const
    {
        if (idx < _size) [[likely]]
        {
            return _ptr[idx];
        }
        throw std::out_of_range{ "index out of range" };
    }

    reference operator[](const size_type idx) { return _ptr[idx]; }
    const_reference operator[](const size_type idx) const { return _ptr[idx]; }
    pointer data() { return _ptr; }
    const_pointer data() const { return _ptr; }

    iterator begin() { return _ptr; }
    iterator end() { return _ptr + _size; }
    const_iterator begin() const { return _ptr; }
    const_iterator end() const { return _ptr + _size; }
    const_iterator cbegin() const { return _ptr; }
    const_iterator cend() const { return _ptr + _size; }
    reference front() { return *_ptr; }
    const_reference front() const { return *_ptr; }
    reference back() { return *(_ptr + _size - 1); }
    const_reference back() const { return *(_ptr + _size - 1); }

    bool empty() const { return _size == 0; }
    size_type size() const { return _size; }
    size_type capacity() const { return _capacity; }

    std::tuple<pointer, size_type, size_type> release()
    {
        return std::make_tuple(std::exchange(_ptr, nullptr), std::exchange(_size, 0), std::exchange(_capacity, 0));
    }
};
} // namespace nstl

#endif
