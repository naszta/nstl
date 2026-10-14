#include "datahash.hpp"
#include "exception.hpp"
#include "compiler.hpp"

#include <CommonCrypto/CommonDigest.h>

namespace nstl
{
namespace
{
NSTL_DEPRECATED_PUSH

class MacMd5Hasher final : public Hasher
{
    CC_MD5_CTX _ctx;

public:
    MacMd5Hasher() { CC_MD5_Init(&_ctx); }
    ~MacMd5Hasher() override = default;
    void reset() override { CC_MD5_Init(&_ctx); }
    void add(const void* data_, size_t size_) override { CC_MD5_Update(&_ctx, data_, size_); }
    HashValue finish() override
    {
        HashMd5Type retval;
        CC_MD5_Final(retval.data(), &_ctx);
        return retval;
    }
};

NSTL_DEPRECATED_POP

class MacSha1Hasher final : public Hasher
{
    CC_SHA1_CTX _ctx;

public:
    MacSha1Hasher() { CC_SHA1_Init(&_ctx); }
    ~MacSha1Hasher() override = default;
    void reset() override { CC_SHA1_Init(&_ctx); }
    void add(const void* data_, size_t size_) override { CC_SHA1_Update(&_ctx, data_, size_); }
    HashValue finish() override
    {
        HashSha1Type retval;
        CC_SHA1_Final(retval.data(), &_ctx);
        return retval;
    }
};

class MacSha256Hasher final : public Hasher
{
    CC_SHA256_CTX _ctx;

public:
    MacSha256Hasher() { CC_SHA256_Init(&_ctx); }
    ~MacSha256Hasher() override = default;
    void reset() override { CC_SHA256_Init(&_ctx); }
    void add(const void* data_, size_t size_) override { CC_SHA256_Update(&_ctx, data_, size_); }
    HashValue finish() override
    {
        HashSha256Type retval;
        CC_SHA256_Final(retval.data(), &_ctx);
        return retval;
    }
};

class MacSha512Hasher final : public Hasher
{
    CC_SHA512_CTX _ctx;

public:
    MacSha512Hasher() { CC_SHA512_Init(&_ctx); }
    ~MacSha512Hasher() override = default;
    void reset() override { CC_SHA512_Init(&_ctx); }
    void add(const void* data_, size_t size_) override { CC_SHA512_Update(&_ctx, data_, size_); }
    HashValue finish() override
    {
        HashSha512Type retval;
        CC_SHA512_Final(retval.data(), &_ctx);
        return retval;
    }
};
} // namespace

std::shared_ptr<Hasher> Hasher::factory(const HashType type_)
{
    switch (type_)
    {
    case HashType::MD5:
        return std::make_shared<MacMd5Hasher>();
    case HashType::SHA1:
        return std::make_shared<MacSha1Hasher>();
    case HashType::SHA256:
        return std::make_shared<MacSha256Hasher>();
    case HashType::SHA512:
        return std::make_shared<MacSha512Hasher>();
    default:
        NSTL2_THROW_EXCEPTION("Invalid hash type!");
    }
}
} // namespace nstl
