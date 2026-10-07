#ifndef _NSTL_DATAHASH_FWD
#define _NSTL_DATAHASH_FWD 1

#include <cstdint>
#include <optional>
#include <string_view>

namespace nstl
{
enum class HashType : std::uint32_t
{
    SHA1 = 20,
    SHA256 = 32,
    SHA512 = 64,
    Default = SHA256,
};

std::optional<HashType> parseHashType(std::string_view name_);
} // namespace nstl

#endif
