#include "nft.hpp"

#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using nstl::ddns::nft_base;
using nstl::ddns::nft_file;
using nstl::ddns::nft_logonly;

namespace
{
std::string read_all(const std::filesystem::path& path_)
{
    std::ifstream in{ path_, std::ios_base::binary };
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}
} // namespace

TEST(DdnsNft, LogOnly)
{
    nft_logonly target;
    EXPECT_TRUE(target.valid());
    EXPECT_TRUE(target.execute("flush set inet filter home"));
    EXPECT_TRUE(target.execute("add element inet filter home { 10.0.0.1 }"));
    EXPECT_FALSE(target.execute(nullptr));
    EXPECT_TRUE(target.valid());
}

TEST(DdnsNft, LogOnlyViaBase)
{
    std::unique_ptr<nft_base> target = std::make_unique<nft_logonly>();
    EXPECT_TRUE(target->valid());
    EXPECT_TRUE(target->execute("flush set inet filter home"));
}

TEST(DdnsNft, FileWritesCommands)
{
    const nstl::temp_dir dir;
    const auto path = dir / "nft.txt";
    {
        nft_file target{ path };
        ASSERT_TRUE(target.valid());
        EXPECT_TRUE(target.execute("flush set inet filter home"));
        EXPECT_TRUE(target.execute("add element inet filter home { 10.0.0.1 }"));
    }
    EXPECT_EQ(read_all(path), "flush set inet filter home\nadd element inet filter home { 10.0.0.1 }\n");
}

TEST(DdnsNft, FileNullCommand)
{
    const nstl::temp_dir dir;
    const auto path = dir / "nft.txt";
    {
        nft_file target{ path };
        EXPECT_FALSE(target.execute(nullptr));
        EXPECT_TRUE(target.valid());
    }
    EXPECT_EQ(read_all(path), "");
}

TEST(DdnsNft, FileAppends)
{
    const nstl::temp_dir dir;
    const auto path = dir / "nft.txt";
    {
        nft_file target{ path };
        EXPECT_TRUE(target.execute("first"));
    }
    {
        nft_file target{ path };
        EXPECT_TRUE(target.execute("second"));
    }
    EXPECT_EQ(read_all(path), "first\nsecond\n");
}

TEST(DdnsNft, FileInvalidTarget)
{
    const nstl::temp_dir dir;
    nft_file target{ dir / "missing_dir" / "nft.txt" };
    EXPECT_FALSE(target.valid());
    EXPECT_FALSE(target.execute("flush set inet filter home"));
}
