#include "datahash.hpp"
#include "exception.hpp"
#include "scope_exit.hpp"

#include <iterator>

#include <cstring>

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#define file_open _wopen
#else
#include <unistd.h>
#define file_open open
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

namespace nstl
{
Hasher::Hasher() = default;
Hasher::~Hasher() = default;

namespace
{
HashValue hash_file_impl(const std::filesystem::path& path_, const HashType type_, char* buffer_,
                         const unsigned int buffersize_)
{
    NSTL2_THROW_EXCEPTION_IF(path_.empty(), "filename is empty");
    const auto file_handler = ::file_open(path_.c_str(), O_RDONLY | O_BINARY);
    NSTL2_THROW_EXCEPTION_IF(file_handler == -1, path_ << " cannot be opened");
    const auto cleanup = nstl::on_scope_exit([file_handler]() { ::close(file_handler); });
    const auto hasher = Hasher::factory(type_);

    int read_bytes = 1;
    while (0 < read_bytes)
    {
        read_bytes = ::read(file_handler, buffer_, buffersize_);
        if (0 < read_bytes)
        {
            hasher->add(buffer_, static_cast<size_t>(read_bytes));
            continue;
        }
        NSTL2_THROW_EXCEPTION_IF(read_bytes < 0, "Error while reading file: " << path_);
    }

    return hasher->finish();
}
} // namespace

HashValue hash_file(const std::filesystem::path& path_, const HashType type_, const size_t buffersize_)
{
    NSTL2_THROW_EXCEPTION_IF(buffersize_ == 0, "Buffer size must not be 0");
    std::vector<char> buffer;
    buffer.resize(buffersize_);
    return hash_file_impl(path_, type_, buffer.data(), static_cast<unsigned int>(buffer.size()));
}

#ifdef __cpp_lib_span
HashValue hash_file(const std::filesystem::path& path_, const std::span<char>& buffer_, const HashType type_)
{
    NSTL2_THROW_EXCEPTION_IF(buffer_.empty(), "Span is empty!");
    return hash_file_impl(path_, type_, buffer_.data(), static_cast<unsigned int>(buffer_.size()));
}
#endif

namespace
{
template <class CharT, class TraitsT = std::char_traits<CharT>>
std::basic_string<CharT, TraitsT> hash_to_hex_t(const HashValue& hash_,
                                                const std::basic_string_view<CharT, TraitsT> digits_)
{
    std::basic_string<CharT, TraitsT> retval;
    retval.reserve(hash_.size() * 2);
    for (auto b : hash_)
    {
        retval.push_back(digits_[b >> 4]);
        retval.push_back(digits_[b & 0x0F]);
    }
    return retval;
}
} // namespace

std::string hash_to_hex(const HashValue& hash_)
{
    constexpr const std::string_view digits{ "0123456789ABCDEF" };
    return hash_to_hex_t(hash_, digits);
}

std::wstring whash_to_hex(const HashValue& hash_)
{
    constexpr const std::wstring_view digits{ L"0123456789ABCDEF" };
    return hash_to_hex_t(hash_, digits);
}

std::optional<HashType> parseHashType(const std::string_view name_)
{
    if (name_.empty())
    {
        return std::nullopt;
    }
    else if (name_ == "MD5")
    {
        return HashType::MD5;
    }
    else if (name_ == "SHA" || name_ == "SHA1")
    {
        return HashType::SHA1;
    }
    else if (name_ == "SHA256")
    {
        return HashType::SHA256;
    }
    else if (name_ == "SHA512")
    {
        return HashType::SHA512;
    }
    else if (name_ == "Default")
    {
        return HashType::Default;
    }
    NSTL2_THROW_EXCEPTION(name_ << " is unknown hash type");
}

void Hasher::add(const char* data_)
{
    if (data_)
    {
        this->add(data_, std::strlen(data_));
    }
}

void Hasher::add(const wchar_t* data_)
{
    if (data_)
    {
        this->add(data_, std::wcslen(data_) * sizeof(wchar_t));
    }
}
} // namespace nstl
