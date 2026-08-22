#include <nstl/file_trigger.hpp>
#include <nstl/exception.hpp>
#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

// Collects the data delivered through file_trigger callbacks and lets tests
// wait for a specific amount of bytes to arrive instead of racing the
// background reader thread.
class collector
{
    mutable std::mutex _mtx;
    mutable std::condition_variable _cv;
    std::string _data;

public:
    nstl::file_trigger::data_cb cb()
    {
        return [this](std::span<char> data_)
        {
            const std::lock_guard lock{ _mtx };
            _data.append(data_.data(), data_.size());
            _cv.notify_all();
        };
    }

    std::string data() const
    {
        const std::lock_guard lock{ _mtx };
        return _data;
    }

    bool wait_for_size(size_t size_, std::chrono::milliseconds timeout_ = 5s) const
    {
        std::unique_lock lock{ _mtx };
        return _cv.wait_for(lock, timeout_, [&] { return _data.size() >= size_; });
    }

    bool wait_stable(std::chrono::milliseconds quiet_ = 200ms) const
    {
        std::unique_lock lock{ _mtx };
        return !_cv.wait_for(lock, quiet_, [&] { return false; });
    }
};

void append(const fs::path& path_, const std::string_view text_)
{
    std::ofstream out{ path_, std::ios::binary | std::ios::app };
    ASSERT_TRUE(out.is_open());
    out.write(text_.data(), static_cast<std::streamsize>(text_.size()));
    out.flush();
}

// file_trigger opens its target for read while other processes/handles keep
// appending to it, so the fixture must not hold a competing handle open the
// way nstl::temp_file does -- create and immediately close the file instead.
class trigger_file
{
    nstl::temp_dir _dir;
    fs::path _path{ _dir.path() / "target.txt" };

public:
    trigger_file() { append(_path, ""); }
    const fs::path& path() const { return _path; }
};
} // namespace

TEST(FileTrigger, MissingFileThrows)
{
    nstl::temp_dir dir;
    const auto missing = dir.path() / "does_not_exist.txt";
    collector col;
    EXPECT_THROW(nstl::file_trigger::factory(missing, col.cb()), std::runtime_error);
}

TEST(FileTrigger, InvalidCallbackThrows)
{
    trigger_file file;
    EXPECT_THROW(nstl::file_trigger::factory(file.path(), nstl::file_trigger::data_cb{}), std::runtime_error);
}

TEST(FileTrigger, AppendedDataIsDelivered)
{
    trigger_file file;
    collector col;
    auto trigger = nstl::file_trigger::factory(file.path(), col.cb());
    ASSERT_TRUE(trigger);

    append(file.path(), "hello");
    ASSERT_TRUE(col.wait_for_size(5)) << "received: " << col.data();
    EXPECT_EQ(col.data(), "hello");

    append(file.path(), " world");
    ASSERT_TRUE(col.wait_for_size(11)) << "received: " << col.data();
    EXPECT_EQ(col.data(), "hello world");

    trigger->stop();
}

TEST(FileTrigger, StopSuppressesFurtherCallbacks)
{
    trigger_file file;
    collector col;
    auto trigger = nstl::file_trigger::factory(file.path(), col.cb());
    ASSERT_TRUE(trigger);

    append(file.path(), "before-stop");
    ASSERT_TRUE(col.wait_for_size(11));

    trigger->stop();
    const auto before = col.data();

    append(file.path(), "after-stop");
    EXPECT_TRUE(col.wait_stable());
    EXPECT_EQ(col.data(), before);
}

TEST(FileTrigger, SowFalseSkipsExistingContent)
{
    trigger_file file;
    append(file.path(), "pre-existing");

    collector col;
    auto trigger = nstl::file_trigger::factory(file.path(), col.cb(), false);
    ASSERT_TRUE(trigger);

    append(file.path(), "new");
    ASSERT_TRUE(col.wait_for_size(3)) << "received: " << col.data();
    EXPECT_EQ(col.data(), "new");

    trigger->stop();
}

TEST(FileTrigger, SowTrueDeliversExistingContent)
{
    trigger_file file;
    append(file.path(), "pre-existing ");

    collector col;
    auto trigger = nstl::file_trigger::factory(file.path(), col.cb(), true);
    ASSERT_TRUE(trigger);
    ASSERT_TRUE(col.wait_for_size(13)) << "received: " << col.data();

    append(file.path(), "new");
    ASSERT_TRUE(col.wait_for_size(16)) << "received: " << col.data();
    EXPECT_EQ(col.data(), "pre-existing new");

    trigger->stop();
}

TEST(FileTrigger, NativeHandleOverload)
{
    trigger_file file;
    const auto handle = nstl::open_native(file.path());

    collector col;
    auto trigger = nstl::file_trigger::factory(handle, col.cb());
    ASSERT_TRUE(trigger);

    append(file.path(), "via-handle");
    ASSERT_TRUE(col.wait_for_size(10)) << "received: " << col.data();
    EXPECT_EQ(col.data(), "via-handle");

    trigger->stop();
}

TEST(FileTrigger, StopIsIdempotent)
{
    trigger_file file;
    collector col;
    auto trigger = nstl::file_trigger::factory(file.path(), col.cb());
    ASSERT_TRUE(trigger);

    trigger->stop();
    EXPECT_NO_THROW(trigger->stop());
}

TEST(FileTrigger, DestructorStopsWorker)
{
    trigger_file file;
    collector col;
    {
        auto trigger = nstl::file_trigger::factory(file.path(), col.cb());
        ASSERT_TRUE(trigger);
        append(file.path(), "seen");
        ASSERT_TRUE(col.wait_for_size(4));
    }
    // The trigger (and its background thread) is gone now; further writes
    // must not reach the collector.
    append(file.path(), "unseen");
    EXPECT_TRUE(col.wait_stable());
    EXPECT_EQ(col.data(), "seen");
}
