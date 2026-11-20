#include "log_rotate.hpp"
#include "logging.hpp"
#include "exception.hpp"

#include <cstdint>
#include <format>
#include <string>
#include <vector>

namespace nstl
{
namespace fs = std::filesystem;

bool log_rotate(const fs::path& file_)
{
    if (file_.empty())
    {
        return false;
    }
    const auto fullpath = fs::absolute(file_);
    if (!fs::exists(fullpath) || !fs::is_regular_file(fullpath))
    {
        return false;
    }
    std::vector<std::string> next_paths;

    const auto fullstr = fullpath.string();
    std::uint32_t path_idx = 0;
    next_paths.push_back(fullstr);

    auto next_path = std::format("{}.{}", fullstr, path_idx);
    for (; fs::exists(next_path); ++path_idx, next_path = std::format("{}.{}", fullstr, path_idx))
    {
        next_paths.push_back(std::move(next_path));
    }
    next_paths.push_back(std::move(next_path));

    for (auto to_itr = next_paths.crbegin(), from_itr = std::next(to_itr); from_itr != next_paths.crend(); ++to_itr, ++from_itr)
    {
        NSTL2_THROW_EXCEPTION_IF(fs::exists(*to_itr), *to_itr << " exists: it cannot be target");
        NSTL2_THROW_EXCEPTION_IF(!fs::exists(*from_itr), *from_itr << " does not exists: it cannot be a source");
        fs::rename(*from_itr, *to_itr);
        NSTL_DEBUG(*from_itr << " renamed to " << *to_itr);
    }

    return true;
}
}
