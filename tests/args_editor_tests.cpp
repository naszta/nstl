#include <nstl/args_editor.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(ArgEditor, Test)
{
	const std::vector<std::string> items{ "item0", "item1", "item2" };
    std::vector<const char*> test_items{ items[0].c_str(), items[1].c_str(), items[2].c_str(), nullptr };
    std::vector<const char*> test_exected{ items[0].c_str(), items[2].c_str(), items[1].c_str() , nullptr };

    const char* not_exist = "not_exists";
    const char* item1 = "item1";
    const char* item2 = "item2";

	int argc = 3;
    EXPECT_FALSE(nstl::is_arg_set(argc, test_items.data(), not_exist));
    ASSERT_EQ(argc, 3);
    EXPECT_TRUE(nstl::is_arg_set(argc, test_items.data(), item1));
    EXPECT_EQ(argc, 2);
    EXPECT_EQ(test_items, test_exected);
    EXPECT_TRUE(nstl::is_arg_set(argc, test_items.data(), item2));
    EXPECT_EQ(argc, 1);
    EXPECT_EQ(test_items, test_exected);
}
