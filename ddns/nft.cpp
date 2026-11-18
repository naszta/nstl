#include "nft.hpp"

#include <nstl/logging.hpp>

#ifdef __linux__
#include <nftables/libnftables.h>
#endif

namespace nstl::ddns
{
nft_base::nft_base() = default;
nft_base::~nft_base() = default;

#ifdef __linux__
nft::nft() : _ptr{ ::nft_ctx_new(0) } {}

nft::~nft()
{
    if (_ptr)
    {
        nft_ctx_free(_ptr);
        _ptr = nullptr;
    }
}

bool nft::execute(const char* command_)
{
    if (!command_) [[unlikely]]
    {
        return false;
    }
    if (0 <= ::nft_run_cmd_from_buffer(_ptr, command_))
    {
        NSTL_INFO("nft executed: " << command_);
        return true;
    }
    else
    {
        NSTL_ERROR("nft execution failed: " << command_);
        return false;
    }
}

bool nft::valid() const { return !!_ptr; }
#endif

nft_file::nft_file(const std::filesystem::path& tgt_)
    : _file{ tgt_, std::ios_base::out | std::ios_base::binary | std::ios_base::app }
{
}

nft_file::~nft_file() = default;

bool nft_file::execute(const char* command_)
{
    if (!command_) [[unlikely]]
    {
        return false;
    }
    _file << command_ << std::endl;
    return _file.good();
}

bool nft_file::valid() const { return _file.good(); }

nft_logonly::nft_logonly() = default;
nft_logonly::~nft_logonly() = default;

bool nft_logonly::execute(const char* command_)
{
    const auto log_idx = ++_idx;
    if (!command_) [[unlikely]]
    {
        NSTL_ERROR(log_idx << " command is nullptr");
        return false;
    }
    NSTL_INFO(log_idx << ": nft executed: " << command_);
    return true;
}

bool nft_logonly::valid() const { return true; }

} // namespace nstl::ddns
