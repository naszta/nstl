#include <nstl/temp_dir.hpp>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

TEST(TempDir, DefaultRandomName)
{
    fs::path saved;
    {
        nstl::temp_dir dir;
        saved = dir.path();
        EXPECT_TRUE(fs::exists(saved));
        EXPECT_TRUE(fs::is_directory(saved));
        EXPECT_TRUE(fs::equivalent(saved.parent_path(), fs::temp_directory_path()));
    }
    EXPECT_FALSE(fs::exists(saved));
}

TEST(TempDir, ExplicitName)
{
    const fs::path name{ "nstl_temp_dir_explicit_name_test" };
    fs::path saved;
    {
        nstl::temp_dir dir{ name };
        saved = dir.path();
        EXPECT_EQ(saved.filename(), name);
        EXPECT_TRUE(fs::exists(saved));
    }
    EXPECT_FALSE(fs::exists(saved));
}

TEST(TempDir, ExplicitParentAndName)
{
    nstl::temp_dir parent;
    const fs::path name{ "child" };

    fs::path saved;
    {
        nstl::temp_dir child{ parent.path(), name };
        saved = child.path();
        EXPECT_EQ(saved, parent.path() / name);
        EXPECT_TRUE(fs::exists(saved));
    }
    EXPECT_FALSE(fs::exists(saved));
    EXPECT_TRUE(fs::exists(parent.path()));
}

TEST(TempDir, ImplicitConversionAndDivide)
{
    nstl::temp_dir dir;

    EXPECT_TRUE(fs::exists(static_cast<const fs::path&>(dir)));
    EXPECT_EQ(dir / "file.txt", dir.path() / "file.txt");
}

TEST(TempDir, DuplicateNameThrows)
{
    nstl::temp_dir parent;
    const fs::path name{ "duplicate" };

    nstl::temp_dir first{ parent.path(), name };
    EXPECT_THROW((nstl::temp_dir{ parent.path(), name }), std::runtime_error);
}

TEST(TempDir, NameReusableAfterDestruction)
{
    nstl::temp_dir parent;
    const fs::path name{ "reusable" };

    fs::path saved;
    {
        nstl::temp_dir first{ parent.path(), name };
        saved = first.path();
    }
    EXPECT_FALSE(fs::exists(saved));

    nstl::temp_dir second{ parent.path(), name };
    EXPECT_TRUE(fs::exists(second.path()));
}

TEST(TempDir, TempFile)
{
    fs::path target;
    {
        nstl::temp_file file;
        target = file.path();
        EXPECT_TRUE(fs::exists(target)) << target << " never created";
    }
    EXPECT_FALSE(fs::exists(target));
}
