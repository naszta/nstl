#include "datahash.hpp"
#include "exception.hpp"
#include "scope_exit.hpp"

#include <iterator>

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#define file_open _wopen
#else
#include <unistd.h>
#define O_BINARY 0
#define file_open open
#endif

namespace nstl
{
Hasher::Hasher() = default;
Hasher::~Hasher() = default;

HashValue hash_file(const std::filesystem::path& path_, const HashType type_, const size_t buffersize_)
{
    NSTL2_THROW_EXCEPTION_IF(buffersize_ == 0, "Buffer size must not be 0");
    const auto file_handler = ::file_open(path_.c_str(), O_RDONLY | O_BINARY);
    NSTL2_THROW_EXCEPTION_IF(file_handler == -1, path_ << " cannot be opened");
    const auto cleanup = nstl::on_scope_exit([file_handler]() { ::close(file_handler); });
    const auto hasher = Hasher::factory(type_);
    std::vector<char> buffer;
    buffer.resize(buffersize_);

    int read_bytes = 1;
    while (0 < read_bytes)
    {
        read_bytes = ::read(file_handler, buffer.data(), static_cast<unsigned int>(buffer.size()));
        if (0 < read_bytes)
        {
            hasher->add(buffer.data(), static_cast<size_t>(read_bytes));
            continue;
        }
        NSTL2_THROW_EXCEPTION_IF(read_bytes < 0, "Error while reading file: " << path_);
    }

    return hasher->finish();
}

std::string hash_to_hex(const HashValue& hash_)
{
    static constexpr const char* digits = "0123456789ABCDEF";
    std::string retval;
    retval.reserve(hash_.size() * 2);
    for (auto b : hash_)
    {
        retval.push_back(digits[b >> 4]);
        retval.push_back(digits[b & 0x0F]);
    }
    return retval;
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
} // namespace nstl
