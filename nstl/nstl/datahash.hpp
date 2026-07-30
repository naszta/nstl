#ifndef _NSTL_DATAHASH_H
#define _NSTL_DATAHASH_H 1

#include <nstl/datahash_fwd.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace nstl
{
using HashValue = std::vector<unsigned char>;

constexpr size_t megabyte = 1048576;

class Hasher
{
protected:
    Hasher();

public:
    static std::shared_ptr<Hasher> factory(HashType type_ = HashType::Default);

    virtual ~Hasher();
    Hasher(const Hasher&) = delete;
    Hasher& operator= (const Hasher&) = delete;

    virtual void reset() = 0;

    virtual void add(const char* data_, size_t size_) = 0;

#ifdef __cpp_lib_span
    inline void add(const std::span<char> data_)
    {
        this->add(data_.data(), data_.size());
    }
#endif

#ifdef __cpp_lib_string_view
    inline void add(const std::string_view view_)
    {
        this->add(view_.data(), view_.size());
    }
#endif

    virtual HashValue finish() = 0;
};

HashValue hash_file(const std::filesystem::path& path_, HashType type_ = HashType::Default, size_t buffersize_ = megabyte);
std::string hash_to_hex(const HashValue& hash_);
}

#endif
