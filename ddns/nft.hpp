#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>

extern "C"
{
    struct nft_ctx;
}

namespace nstl::ddns
{
class nft_base
{
public:
    nft_base();
    virtual ~nft_base();
    nft_base(const nft_base&) = delete;
    nft_base& operator=(const nft_base&) = delete;

    virtual bool execute(const char* command_) = 0;
    virtual bool valid() const = 0;
};

class nft_file : public nft_base
{
    std::ofstream _file;

public:
    explicit nft_file(const std::filesystem::path& tgt_);
    ~nft_file() override;

    bool execute(const char* command_) override;
    bool valid() const override;
};

class nft_logonly : public nft_base
{
    std::atomic_uint64_t _idx{ 0 };

public:
    nft_logonly();
    ~nft_logonly() override;

    bool execute(const char* command_) override;
    bool valid() const override;
};

#ifdef __linux__
class nft : public nft_base
{
    nft_ctx* _ptr{ nullptr };

public:
    nft();
    ~nft() override;

    bool execute(const char* command_) override;
    bool valid() const override;
};
#else
using nft = nft_logonly;
#endif

} // namespace nstl::ddns
