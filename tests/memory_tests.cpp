#include <nstl/memory.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <unordered_set>

namespace
{
struct Base
{
    virtual ~Base() = default;
    int value{ 42 };
};

struct Derived : Base
{
};
} // namespace

TEST(ObserverPtr, DefaultConstruction)
{
    nstl::observer_ptr<int> p;
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(static_cast<bool>(p));
}

TEST(ObserverPtr, NullptrConstruction)
{
    nstl::observer_ptr<int> p{ nullptr };
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(static_cast<bool>(p));
}

TEST(ObserverPtr, PointerConstruction)
{
    int value = 5;
    nstl::observer_ptr<int> p{ &value };
    EXPECT_EQ(p.get(), &value);
    EXPECT_TRUE(static_cast<bool>(p));
}

TEST(ObserverPtr, ConvertingConstruction)
{
    Derived d;
    nstl::observer_ptr<Derived> derived_p{ &d };
    nstl::observer_ptr<Base> base_p{ derived_p };
    EXPECT_EQ(base_p.get(), &d);
}

TEST(ObserverPtr, ConstructFromUniquePtr)
{
    auto up = std::make_unique<int>(7);
    nstl::observer_ptr<int> p{ up };
    EXPECT_EQ(p.get(), up.get());
}

TEST(ObserverPtr, ConstructFromSharedPtr)
{
    auto sp = std::make_shared<int>(9);
    nstl::observer_ptr<int> p{ sp };
    EXPECT_EQ(p.get(), sp.get());
}

TEST(ObserverPtr, DereferenceAndArrow)
{
    Base b;
    nstl::observer_ptr<Base> p{ &b };
    EXPECT_EQ((*p).value, 42);
    EXPECT_EQ(p->value, 42);
}

TEST(ObserverPtr, ExplicitPointerConversion)
{
    int value = 3;
    nstl::observer_ptr<int> p{ &value };
    int* raw = static_cast<int*>(p);
    EXPECT_EQ(raw, &value);
}

TEST(ObserverPtr, Release)
{
    int value = 1;
    nstl::observer_ptr<int> p{ &value };
    int* released = p.release();
    EXPECT_EQ(released, &value);
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(static_cast<bool>(p));
}

TEST(ObserverPtr, ResetToNullptr)
{
    int value = 1;
    nstl::observer_ptr<int> p{ &value };
    p.reset();
    EXPECT_EQ(p.get(), nullptr);
}

TEST(ObserverPtr, ResetToNewPointer)
{
    int a = 1, b = 2;
    nstl::observer_ptr<int> p{ &a };
    p.reset(&b);
    EXPECT_EQ(p.get(), &b);
}

TEST(ObserverPtr, Swap)
{
    int a = 1, b = 2;
    nstl::observer_ptr<int> pa{ &a };
    nstl::observer_ptr<int> pb{ &b };
    pa.swap(pb);
    EXPECT_EQ(pa.get(), &b);
    EXPECT_EQ(pb.get(), &a);
}

TEST(ObserverPtr, MakeObserverFromRawPointer)
{
    int value = 1;
    auto p = nstl::make_observer(&value);
    static_assert(std::is_same_v<decltype(p), nstl::observer_ptr<int>>);
    EXPECT_EQ(p.get(), &value);
}

TEST(ObserverPtr, MakeObserverFromUniquePtr)
{
    auto up = std::make_unique<int>(4);
    auto p = nstl::make_observer(up);
    EXPECT_EQ(p.get(), up.get());
}

TEST(ObserverPtr, MakeObserverFromSharedPtr)
{
    auto sp = std::make_shared<int>(6);
    auto p = nstl::make_observer(sp);
    EXPECT_EQ(p.get(), sp.get());
}

TEST(ObserverPtr, EqualityAndInequality)
{
    int a = 1, b = 2;
    nstl::observer_ptr<int> pa1{ &a };
    nstl::observer_ptr<int> pa2{ &a };
    nstl::observer_ptr<int> pb{ &b };

    EXPECT_TRUE(pa1 == pa2);
    EXPECT_FALSE(pa1 != pa2);
    EXPECT_TRUE(pa1 != pb);
    EXPECT_FALSE(pa1 == pb);
}

TEST(ObserverPtr, NullptrComparisons)
{
    int a = 1;
    nstl::observer_ptr<int> null_p;
    nstl::observer_ptr<int> non_null_p{ &a };

    EXPECT_TRUE(null_p == nullptr);
    EXPECT_TRUE(nullptr == null_p);
    EXPECT_FALSE(non_null_p == nullptr);
    EXPECT_FALSE(nullptr == non_null_p);

    EXPECT_TRUE(non_null_p != nullptr);
    EXPECT_TRUE(nullptr != non_null_p);
    EXPECT_FALSE(null_p != nullptr);
    EXPECT_FALSE(nullptr != null_p);
}

TEST(ObserverPtr, OrderingComparisons)
{
    int arr[2]{};
    nstl::observer_ptr<int> low{ &arr[0] };
    nstl::observer_ptr<int> high{ &arr[1] };

    EXPECT_TRUE(low < high);
    EXPECT_FALSE(high < low);
    EXPECT_TRUE(high > low);
    EXPECT_FALSE(low > high);
    EXPECT_TRUE(low <= high);
    EXPECT_TRUE(low <= low);
    EXPECT_TRUE(high >= low);
    EXPECT_TRUE(high >= high);
}

TEST(ObserverPtr, CrossTypeComparison)
{
    Derived d;
    nstl::observer_ptr<Derived> derived_p{ &d };
    nstl::observer_ptr<Base> base_p{ &d };

    EXPECT_TRUE(derived_p == base_p);
    EXPECT_FALSE(derived_p != base_p);
}

TEST(ObserverPtr, Hash)
{
    int value = 1;
    nstl::observer_ptr<int> p1{ &value };
    nstl::observer_ptr<int> p2{ &value };

    EXPECT_EQ(std::hash<nstl::observer_ptr<int>>{}(p1), std::hash<nstl::observer_ptr<int>>{}(p2));

    std::unordered_set<nstl::observer_ptr<int>> set;
    set.insert(p1);
    EXPECT_EQ(set.count(p2), 1u);
}

TEST(ObserverPtr, LeakingPtrAlias)
{
    int value = 1;
    nstl::leaking_ptr<int> p{ &value };
    EXPECT_EQ(p.get(), &value);
    static_assert(std::is_same_v<nstl::leaking_ptr<int>, nstl::observer_ptr<int>>);
}
