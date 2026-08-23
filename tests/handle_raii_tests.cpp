#include <nstl/handle_raii.hpp>
#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

#include <fcntl.h>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>
#define file_open _wopen
#define file_close _close
#else
#include <unistd.h>
#define file_open open
#define file_close close
#endif

namespace
{
nstl::FileIntRaii openTestFile(const std::filesystem::path& path_)
{
    return nstl::FileIntRaii{ ::file_open(path_.c_str(), O_RDWR | O_CREAT, 0644) };
}
} // namespace

TEST(FileIntRaii, DefaultConstructedIsInvalid)
{
    const nstl::FileIntRaii handle;
    EXPECT_FALSE(static_cast<bool>(handle));
    EXPECT_EQ(static_cast<int>(handle), nstl::FileIntRaii::invalid_value);
}

TEST(FileIntRaii, ValidHandleFromRealFile)
{
    const nstl::temp_dir dir;
    const auto handle = openTestFile(dir.path() / "handle_raii_valid.txt");
    EXPECT_TRUE(static_cast<bool>(handle));
    EXPECT_LT(0, static_cast<int>(handle));
}

TEST(FileIntRaii, FdZeroIsNotValid)
{
    const nstl::FileIntRaii handle{ 0 };
    EXPECT_FALSE(static_cast<bool>(handle));
}

TEST(FileIntRaii, ResetClosesPreviousHandleAndAdoptsNew)
{
    const nstl::temp_dir dir;
    auto handle = openTestFile(dir.path() / "handle_raii_reset_a.txt");
    auto second = openTestFile(dir.path() / "handle_raii_reset_b.txt");
    const int second_fd = static_cast<int>(second);

    handle.reset(second.release());
    EXPECT_EQ(static_cast<int>(handle), second_fd);
}

TEST(FileIntRaii, ReleaseTransfersOwnershipToCaller)
{
    const nstl::temp_dir dir;
    auto handle = openTestFile(dir.path() / "handle_raii_release.txt");
    const int fd = static_cast<int>(handle);
    ASSERT_LT(0, fd);

    const int released = handle.release();
    EXPECT_EQ(released, fd);
    EXPECT_FALSE(static_cast<bool>(handle));

    ::file_close(released);
}

TEST(FileIntRaii, MoveConstructTransfersHandle)
{
    const nstl::temp_dir dir;
    auto handle = openTestFile(dir.path() / "handle_raii_move_ctor.txt");
    const int fd = static_cast<int>(handle);

    const nstl::FileIntRaii moved{ std::move(handle) };
    EXPECT_EQ(static_cast<int>(moved), fd);
    EXPECT_FALSE(static_cast<bool>(handle)); // NOLINT(bugprone-use-after-move) - checking post-move state is intentional
}

TEST(FileIntRaii, MoveAssignClosesPreviousAndTransfers)
{
    const nstl::temp_dir dir;
    auto first = openTestFile(dir.path() / "handle_raii_move_assign_a.txt");
    auto second = openTestFile(dir.path() / "handle_raii_move_assign_b.txt");
    const int second_fd = static_cast<int>(second);

    first = std::move(second);
    EXPECT_EQ(static_cast<int>(first), second_fd);
    EXPECT_FALSE(static_cast<bool>(second)); // NOLINT(bugprone-use-after-move)
}

TEST(FileIntRaii, SelfMoveAssignIsNoop)
{
    const nstl::temp_dir dir;
    auto handle = openTestFile(dir.path() / "handle_raii_self_move.txt");
    const int fd = static_cast<int>(handle);

    auto& self_ref = handle;
    handle = std::move(self_ref);
    EXPECT_EQ(static_cast<int>(handle), fd);
}

TEST(FileIntRaii, SwapExchangesHandles)
{
    const nstl::temp_dir dir;
    auto first = openTestFile(dir.path() / "handle_raii_swap_a.txt");
    auto second = openTestFile(dir.path() / "handle_raii_swap_b.txt");
    const int first_fd = static_cast<int>(first);
    const int second_fd = static_cast<int>(second);

    first.swap(second);
    EXPECT_EQ(static_cast<int>(first), second_fd);
    EXPECT_EQ(static_cast<int>(second), first_fd);
}

TEST(FileIntRaii, ComparisonOperators)
{
    nstl::FileIntRaii a{ 5 };
    nstl::FileIntRaii b{ 5 };
    nstl::FileIntRaii c{ 6 };

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a < c);
    EXPECT_FALSE(c < a);

    EXPECT_TRUE(5 == a);
    EXPECT_TRUE(a == 5);
    EXPECT_TRUE(4 < a);
    EXPECT_TRUE(a < 6);

    // these are fabricated fd values, not real handles - release before destruction so the
    // raii dtor never calls close()/CloseHandle() on a number that was never actually opened.
    a.release();
    b.release();
    c.release();
}

#ifdef _WIN32
TEST(HandleRaii, DefaultConstructedIsInvalid)
{
    const nstl::HandleRaii handle;
    EXPECT_FALSE(static_cast<bool>(handle));
}

TEST(HandleRaii, NullptrIsInvalid)
{
    const nstl::HandleRaii handle{ nullptr };
    EXPECT_FALSE(static_cast<bool>(handle));
}

TEST(HandleRaii, ValidHandleFromRealEvent)
{
    const HANDLE evt = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ASSERT_NE(evt, nullptr);

    const nstl::HandleRaii handle{ evt };
    EXPECT_TRUE(static_cast<bool>(handle));
    EXPECT_EQ(static_cast<HANDLE>(handle), evt);
}

TEST(HandleRaii, ResetClosesHandle)
{
    const HANDLE evt = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ASSERT_NE(evt, nullptr);

    nstl::HandleRaii handle{ evt };
    handle.reset();
    EXPECT_FALSE(static_cast<bool>(handle));
}
#endif
