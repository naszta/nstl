#include <nstl/log_rotate.hpp>
#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace
{
void write_file(const fs::path& path_, const std::string& content_)
{
    std::ofstream out{ path_, std::ios::binary | std::ios::trunc };
    ASSERT_TRUE(out.is_open()) << path_;
    out << content_;
}

std::string read_file(const fs::path& path_)
{
    std::ifstream in{ path_, std::ios::binary };
    return { std::istreambuf_iterator<char>{ in }, std::istreambuf_iterator<char>{} };
}

fs::path rotated(const fs::path& base_, unsigned idx_)
{
    return fs::path{ base_.string() + "." + std::to_string(idx_) };
}
} // namespace

TEST(LogRotate, TestingEmpty) { EXPECT_FALSE(nstl::log_rotate(fs::path{})); }

TEST(LogRotate, MissingFileReturnsFalse)
{
    nstl::temp_dir dir;
    const auto log = dir / "missing.log";

    EXPECT_FALSE(nstl::log_rotate(log));
    EXPECT_FALSE(fs::exists(log));
    EXPECT_FALSE(fs::exists(rotated(log, 0)));
}

TEST(LogRotate, DirectoryReturnsFalse)
{
    nstl::temp_dir dir;
    const auto sub = dir / "subdir.log";
    fs::create_directory(sub);

    EXPECT_FALSE(nstl::log_rotate(sub));
    EXPECT_TRUE(fs::is_directory(sub));
    EXPECT_FALSE(fs::exists(rotated(sub, 0)));
}

TEST(LogRotate, SingleFileRotatedToZero)
{
    nstl::temp_dir dir;
    const auto log = dir / "app.log";
    write_file(log, "current");

    EXPECT_TRUE(nstl::log_rotate(log));

    EXPECT_FALSE(fs::exists(log));
    ASSERT_TRUE(fs::exists(rotated(log, 0)));
    EXPECT_EQ(read_file(rotated(log, 0)), "current");
    EXPECT_FALSE(fs::exists(rotated(log, 1)));
}

TEST(LogRotate, ExistingRotationsAreShifted)
{
    nstl::temp_dir dir;
    const auto log = dir / "app.log";
    write_file(log, "current");
    write_file(rotated(log, 0), "old0");
    write_file(rotated(log, 1), "old1");
    write_file(rotated(log, 2), "old2");

    EXPECT_TRUE(nstl::log_rotate(log));

    EXPECT_FALSE(fs::exists(log));
    EXPECT_EQ(read_file(rotated(log, 0)), "current");
    EXPECT_EQ(read_file(rotated(log, 1)), "old0");
    EXPECT_EQ(read_file(rotated(log, 2)), "old1");
    EXPECT_EQ(read_file(rotated(log, 3)), "old2");
    EXPECT_FALSE(fs::exists(rotated(log, 4)));
}

TEST(LogRotate, GapStopsShifting)
{
    nstl::temp_dir dir;
    const auto log = dir / "app.log";
    write_file(log, "current");
    write_file(rotated(log, 0), "old0");
    // app.log.1 intentionally missing
    write_file(rotated(log, 2), "old2");

    EXPECT_TRUE(nstl::log_rotate(log));

    EXPECT_FALSE(fs::exists(log));
    EXPECT_EQ(read_file(rotated(log, 0)), "current");
    EXPECT_EQ(read_file(rotated(log, 1)), "old0");
    // Files beyond the first gap are left untouched.
    EXPECT_EQ(read_file(rotated(log, 2)), "old2");
    EXPECT_FALSE(fs::exists(rotated(log, 3)));
}

TEST(LogRotate, RepeatedRotationAfterRecreate)
{
    nstl::temp_dir dir;
    const auto log = dir / "app.log";

    for (int i = 0; i < 4; ++i)
    {
        write_file(log, "gen" + std::to_string(i));
        EXPECT_TRUE(nstl::log_rotate(log));
        EXPECT_FALSE(fs::exists(log));
    }

    // Newest generation is at .0, oldest at .3
    EXPECT_EQ(read_file(rotated(log, 0)), "gen3");
    EXPECT_EQ(read_file(rotated(log, 1)), "gen2");
    EXPECT_EQ(read_file(rotated(log, 2)), "gen1");
    EXPECT_EQ(read_file(rotated(log, 3)), "gen0");
    EXPECT_FALSE(fs::exists(rotated(log, 4)));

    // Without recreating the base file, there is nothing to rotate.
    EXPECT_FALSE(nstl::log_rotate(log));
    EXPECT_EQ(read_file(rotated(log, 0)), "gen3");
}

TEST(LogRotate, RelativePath)
{
    nstl::temp_dir dir;
    const auto saved_cwd = fs::current_path();
    fs::current_path(dir.path());

    bool result = false;
    try
    {
        write_file("rel.log", "relative");
        result = nstl::log_rotate("rel.log");
    }
    catch (...)
    {
        fs::current_path(saved_cwd);
        throw;
    }
    fs::current_path(saved_cwd);

    EXPECT_TRUE(result);
    EXPECT_FALSE(fs::exists(dir / "rel.log"));
    EXPECT_EQ(read_file(dir / "rel.log.0"), "relative");
}

TEST(LogRotate, OtherFilesInDirectoryUntouched)
{
    nstl::temp_dir dir;
    const auto log = dir / "app.log";
    const auto other = dir / "other.log";
    const auto other0 = dir / "other.log.0";
    write_file(log, "current");
    write_file(other, "other");
    write_file(other0, "other0");

    EXPECT_TRUE(nstl::log_rotate(log));

    EXPECT_EQ(read_file(rotated(log, 0)), "current");
    EXPECT_EQ(read_file(other), "other");
    EXPECT_EQ(read_file(other0), "other0");
    EXPECT_FALSE(fs::exists(dir / "other.log.1"));
}
