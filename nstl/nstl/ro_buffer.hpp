#ifndef __NSTL_RO_BUFFER
#define __NSTL_RO_BUFFER 1

#include <streambuf>
#include <string_view>
#ifdef __cpp_lib_span
#include <span>
#endif

namespace nstl
{
template <class CharT, class TraitsT = std::char_traits<CharT>>
class basic_ro_buffer : public std::basic_streambuf<CharT, TraitsT>
{
public:
    using char_type = CharT;
    using traits_type = TraitsT;
    using int_type = typename TraitsT::int_type;
    using pos_type = typename TraitsT::pos_type;
    using off_type = typename TraitsT::off_type;
    using view_type = std::basic_string_view<CharT, TraitsT>;
    using base_type = std::basic_streambuf<CharT, TraitsT>;

    basic_ro_buffer(const char* ptr, size_t size)
    {
        char* begin = const_cast<char*>(ptr); // it's safe because pbackfail made to be dead
        this->setg(begin, begin, begin + size);
        this->setp(begin, begin);
    }

    explicit basic_ro_buffer(const view_type strview)
        : basic_ro_buffer{strview.data(), strview.size()}
    {}

#ifdef __cpp_lib_span
    template <std::size_t extent = std::dynamic_extent>
    explicit basic_ro_buffer(const std::span<CharT, extent>& value)
        : basic_ro_buffer{value.data(), value.size()}
    {}
#endif

    size_t readBytes() const
    {
        return static_cast<size_t>(this->gptr() - this->eback());
    }

    view_type content() const
    {
        return view_type{this->eback(), static_cast<size_t>(this->egptr(), this->eback())};
    }

    void swap(basic_ro_buffer& right_) noexcept
    {
        base_type::swap(right_);
    }

    // no copy or move: we use it on stack
    basic_ro_buffer(const basic_ro_buffer&) = delete;
    basic_ro_buffer& operator= (const basic_ro_buffer&) = delete;
    ~basic_ro_buffer() override = default;

protected:
    // read only: no write at all
    int_type overflow(int_type) final
    {
        return traits_type::eof();
    }
    // read only: no flush
    int sync() final
    {
        return -1;
    }
    // nothing to read
    int_type underflow() final
    {
        return traits_type::eof();
    }
    // the buffer is const: we must not change it
    int_type pbackfail(int_type) final
    {
        return traits_type::eof();
    }
};

using ro_buffer = basic_ro_buffer<char>;
using wro_buffer = basic_ro_buffer<wchar_t>;
}

#endif
