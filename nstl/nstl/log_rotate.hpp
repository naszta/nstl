#ifndef _NSTL_LOG_ROTATE
#define _NSTL_LOG_ROTATE 1

#include <filesystem>

namespace nstl
{
bool log_rotate(const std::filesystem::path& file_);
}

#endif
