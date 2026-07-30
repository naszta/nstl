#include "datahash.hpp"
#include "exception.hpp"

#include <openssl/evp.h>

#include <array>

namespace nstl
{
namespace
{
class HasherOpenSsl : public Hasher
{
    const HashType _type;
    EVP_MD_CTX* _mdctx{nullptr};

    void init()
    {
        const EVP_MD *md = nullptr;
        switch (_type)
        {
        case HashType::MD5:
            md = EVP_md5();
            break;
        case HashType::SHA1:
            md = EVP_sha1();
            break;
        case HashType::SHA256:
            md = EVP_sha256();
            break;
        case HashType::SHA512:
            md = EVP_sha512();
            break;
        default:
            NSTL2_THROW_EXCEPTION(static_cast<int>(_type) << " is invalid");
        }
        NSTL2_THROW_EXCEPTION_IF(!::EVP_DigestInit_ex2(_mdctx, md, NULL), "EVP_DigestInit_ex2 failed");
    }

public:
    explicit HasherOpenSsl(const HashType type_)
        : _type{type_}
        , _mdctx{::EVP_MD_CTX_new()}
    {
        this->init();
    }

    void reset() override
    {
        if (_mdctx) [[likely]] {
            NSTL2_THROW_EXCEPTION_IF(!::EVP_MD_CTX_reset(_mdctx), "reset context failed");
            this->init();
        } else {
            NSTL2_THROW_EXCEPTION("Null context cannot be reset");
        }
    }

    ~HasherOpenSsl() override
    {
        if (_mdctx) {
            ::EVP_MD_CTX_free(_mdctx);
            _mdctx = nullptr;
        }
    }

    void add(const char* data_, size_t size_) override
    {
        NSTL2_THROW_EXCEPTION_IF(!EVP_DigestUpdate(_mdctx, data_, size_), "EVP_DigestUpdate failed");
    }

    HashValue finish() override
    {
        HashValue buffer;
        buffer.resize(static_cast<std::underlying_type_t<HashType>>(_type));
        unsigned int md_len = buffer.size();
        NSTL2_THROW_EXCEPTION_IF(!EVP_DigestFinal_ex(_mdctx, buffer.data(), &md_len), "EVP_DigestFinal_ex failed");
        buffer.resize(md_len);
        return buffer;
    }
};
}

std::shared_ptr<Hasher> Hasher::factory(const HashType type_)
{
    return std::make_shared<HasherOpenSsl>(type_);
}
}
