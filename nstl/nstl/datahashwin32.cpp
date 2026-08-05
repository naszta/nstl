#include "datahash.hpp"
#include "exception.hpp"

#include <windows.h>
#include <Wincrypt.h>

#include <array>

namespace nstl
{

namespace
{
ALG_ID translateAlgoId(const HashType type_)
{
    switch (type_)
    {
    case HashType::MD5:
        return CALG_MD5;
    case HashType::SHA1:
        return CALG_SHA;
    case HashType::SHA256:
        return CALG_SHA_256;
    case HashType::SHA512:
        return CALG_SHA_512;
    default:
        NSTL2_THROW_EXCEPTION(static_cast<int>(type_) << " is invalid");
    }
}

struct ThreadProvider
{
    ThreadProvider()
    {
        NSTL2_THROW_EXCEPTION_IF(!::CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT),
                                 "CryptAcquireContextA failed: " << ::GetLastError());
    }
    ~ThreadProvider()
    {
        if (hProv)
        {
            ::CryptReleaseContext(hProv, 0);
            hProv = 0;
        }
    }

    HCRYPTPROV hProv{ 0 };
};

class HasherWin32 final : public Hasher
{
    const HashType _type;

    HCRYPTHASH _hHash{ 0 };

    void clear()
    {
        if (_hHash)
        {
            ::CryptDestroyHash(_hHash);
            _hHash = 0;
        }
    }

    void create()
    {
        thread_local const ThreadProvider context;
        const auto algoid = translateAlgoId(_type);
        NSTL2_THROW_EXCEPTION_IF(_hHash, "Reinit of _hHash");
        NSTL2_THROW_EXCEPTION_IF(!::CryptCreateHash(context.hProv, algoid, 0, 0, &_hHash),
                                 "CryptCreateHash failed: " << ::GetLastError());
    }

public:
    explicit HasherWin32(const HashType type_) : _type{ type_ } { this->create(); }

    void reset() override
    {
        this->clear();
        this->create();
    }

    ~HasherWin32() override { this->clear(); }

    void add(const void* data_, size_t size_) override
    {
        NSTL2_THROW_EXCEPTION_IF(
            !::CryptHashData(_hHash, reinterpret_cast<const BYTE*>(data_), static_cast<DWORD>(size_), 0),
            "CryptHashData failed: " << ::GetLastError());
    }

    HashValue finish() override
    {
        DWORD hash_size = 0;
        DWORD hashLen = sizeof(DWORD);

        NSTL2_THROW_EXCEPTION_IF(
            !::CryptGetHashParam(_hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hash_size), &hashLen, 0),
            "CryptGetHashParam HP_HASHSIZE failed: " << ::GetLastError());

        HashValue buffer;
        buffer.resize(hash_size);
        hashLen = static_cast<DWORD>(buffer.size());

        NSTL2_THROW_EXCEPTION_IF(!::CryptGetHashParam(_hHash, HP_HASHVAL, buffer.data(), &hashLen, 0),
                                 "CryptGetHashParam HP_HASHVAL failed: " << ::GetLastError());
        return buffer;
    }
};

} // namespace

std::shared_ptr<Hasher> Hasher::factory(const HashType type_) { return std::make_shared<HasherWin32>(type_); }

} // namespace nstl
