#include "datahash.hpp"
#include "exception.hpp"

#include <openssl/evp.h>

#include <array>

namespace nstl
{
namespace
{
class HasherOpenSsl final : public Hasher
{
    const HashType _type;
    EVP_MD_CTX* _mdctx{ nullptr };

    void init()
    {
        const EVP_MD* md = nullptr;
        switch (_type)
        {
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
    explicit HasherOpenSsl(const HashType type_) : _type{ type_ }, _mdctx{ ::EVP_MD_CTX_new() } { this->init(); }

    void reset() override
    {
        if (_mdctx) [[likely]]
        {
            NSTL2_THROW_EXCEPTION_IF(!::EVP_MD_CTX_reset(_mdctx), "reset context failed");
            this->init();
        }
        else
        {
            NSTL2_THROW_EXCEPTION("Null context cannot be reset");
        }
    }

    ~HasherOpenSsl() override
    {
        if (_mdctx)
        {
            ::EVP_MD_CTX_free(_mdctx);
            _mdctx = nullptr;
        }
    }

    void add(const void* data_, size_t size_) override
    {
        NSTL2_THROW_EXCEPTION_IF(!::EVP_DigestUpdate(_mdctx, data_, size_), "EVP_DigestUpdate failed");
    }

    HashValue finish() override
    {
        HashValue buffer;
        std::uint8_t* data = nullptr;
        unsigned int md_len = 0;

        switch (_type)
        {
        case HashType::SHA1:
        {
            auto& ref = buffer.emplace<HashSha1Type>();
            data = ref.data();
            md_len = static_cast<unsigned int>(ref.size());
            break;
        }
        case HashType::SHA256:
        {
            auto& ref = buffer.emplace<HashSha256Type>();
            data = ref.data();
            md_len = static_cast<unsigned int>(ref.size());
            break;
        }
        case HashType::SHA512:
        {
            auto& ref = buffer.emplace<HashSha512Type>();
            data = ref.data();
            md_len = static_cast<unsigned int>(ref.size());
            break;
        }
        default:
            NSTL2_THROW_EXCEPTION("Unknown hash type");
        }

        NSTL2_THROW_EXCEPTION_IF(!::EVP_DigestFinal_ex(_mdctx, data, &md_len), "EVP_DigestFinal_ex failed");
        return buffer;
    }
};
} // namespace

std::shared_ptr<Hasher> Hasher::factory(const HashType type_) { return std::make_shared<HasherOpenSsl>(type_); }
} // namespace nstl
