#include <nstl/vector.hpp>

#include <gtest/gtest.h>

#include <utility>

TEST(Vector, DefaultConstruction)
{
    nstl::vector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.data(), nullptr);
}

TEST(Vector, PushBackLValueGrowsSizeAndValues)
{
    nstl::vector<int> v;
    for (int i = 0; i < 5; ++i)
    {
        const int val = i;
        v.push_back(val);
    }
    ASSERT_EQ(v.size(), 5u);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(v[i], i);
    }
}

TEST(Vector, PushBackRValue)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(Vector, CapacityGrowsAutomaticallyOnPushBack)
{
    nstl::vector<int> v;
    size_t last_capacity = v.capacity();
    for (int i = 0; i < 100; ++i)
    {
        v.push_back(i);
        EXPECT_GE(v.capacity(), v.size());
        last_capacity = v.capacity();
    }
    EXPECT_GE(last_capacity, 100u);
}

TEST(Vector, PopBackRemovesLastElement)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.pop_back();
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 1);
}

TEST(Vector, PopBackOnEmptyThrows)
{
    nstl::vector<int> v;
    EXPECT_THROW(v.pop_back(), std::runtime_error);
}

TEST(Vector, AtThrowsOutOfRange)
{
    nstl::vector<int> v;
    v.push_back(1);
    EXPECT_EQ(v.at(0), 1);
    EXPECT_THROW(v.at(1), std::out_of_range);
    EXPECT_THROW(v.at(42), std::out_of_range);
}

TEST(Vector, AtConstThrowsOutOfRange)
{
    nstl::vector<int> v;
    v.push_back(1);
    const nstl::vector<int>& cv = v;
    EXPECT_EQ(cv.at(0), 1);
    EXPECT_THROW(cv.at(1), std::out_of_range);
}

TEST(Vector, OperatorIndexReadWrite)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v[0] = 10;
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 2);
}

TEST(Vector, OperatorIndexConst)
{
    nstl::vector<int> v;
    v.push_back(5);
    const nstl::vector<int>& cv = v;
    EXPECT_EQ(cv[0], 5);
}

TEST(Vector, FrontAndBack)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    EXPECT_EQ(v.front(), 1);
    EXPECT_EQ(v.back(), 3);

    const nstl::vector<int>& cv = v;
    EXPECT_EQ(cv.front(), 1);
    EXPECT_EQ(cv.back(), 3);
}

TEST(Vector, DataPointerAndConstData)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    ASSERT_NE(v.data(), nullptr);
    EXPECT_EQ(v.data()[0], 1);
    EXPECT_EQ(v.data()[1], 2);

    const nstl::vector<int>& cv = v;
    EXPECT_EQ(cv.data()[0], 1);
}

TEST(Vector, IteratorsRangeFor)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    int sum = 0;
    for (const int val : v)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 6);

    int idx = 0;
    for (auto it = v.begin(); it != v.end(); ++it, ++idx)
    {
        EXPECT_EQ(*it, idx + 1);
    }
}

TEST(Vector, ConstIterators)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);

    const nstl::vector<int>& cv = v;
    EXPECT_EQ(std::distance(cv.begin(), cv.end()), 2);
    EXPECT_EQ(std::distance(cv.cbegin(), cv.cend()), 2);
    EXPECT_EQ(*cv.cbegin(), 1);
}

TEST(Vector, ClearRemovesAllElementsButKeepsCapacity)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    const size_t cap_before = v.capacity();
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), cap_before);
}

TEST(Vector, ResetClearsCapacityAndPointer)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.reset();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_EQ(v.data(), nullptr);
}

TEST(Vector, ReserveIncreasesCapacityAndPreservesElements)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.reserve(64);
    EXPECT_GE(v.capacity(), 64u);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
}

TEST(Vector, ReserveSmallerThanCurrentCapacityIsNoop)
{
    nstl::vector<int> v;
    v.reserve(64);
    const size_t cap = v.capacity();
    v.reserve(1);
    EXPECT_EQ(v.capacity(), cap);
}

TEST(Vector, SwapExchangesContents)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);

    nstl::vector<int> b;
    b.push_back(9);

    a.swap(b);

    ASSERT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0], 9);

    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
}

TEST(Vector, CopyConstructorDeepCopies)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);

    nstl::vector<int> b{ a };
    ASSERT_EQ(b.size(), 3u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b[2], 3);

    b[0] = 100;
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(b[0], 100);
}

TEST(Vector, CopyAssignmentDeepCopies)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);

    nstl::vector<int> b;
    b.push_back(42);
    b = a;

    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);

    b[0] = 100;
    EXPECT_EQ(a[0], 1);
}

#ifndef __clang__
TEST(Vector, SelfCopyAssignmentIsNoop)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);

    a = a;

    ASSERT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
}
#endif

TEST(Vector, MoveConstructorTransfersOwnership)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);
    const int* orig_data = a.data();

    nstl::vector<int> b{ std::move(a) };
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b.data(), orig_data);

    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Vector, MoveAssignmentTransfersOwnership)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);

    nstl::vector<int> b;
    b.push_back(42);

    b = std::move(a);
    ASSERT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);

    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Vector, SelfMoveAssignmentIsNoop)
{
    nstl::vector<int> a;
    a.push_back(1);
    a.push_back(2);

    // indirect through a pointer so -Wself-move doesn't flag the intentionally-tested self-move
    nstl::vector<int>* self = &a;
    a = std::move(*self);

    ASSERT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
}

TEST(Vector, ReleaseTransfersRawStateAndResetsVector)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.reserve(10);

    auto [ptr, size, capacity] = v.release();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(size, 2u);
    EXPECT_EQ(capacity, 16u);
    EXPECT_EQ(ptr[0], 1);
    EXPECT_EQ(ptr[1], 2);

    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.data(), nullptr);
    EXPECT_EQ(v.capacity(), 0u);

    std::free(ptr);
}

TEST(Vector, EmptyCtor)
{
    nstl::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.clear();
    nstl::vector<int> u{v};
    EXPECT_TRUE(u.empty());
    EXPECT_EQ(u.capacity(), 0);
}
