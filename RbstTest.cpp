#include <assert.h>
#include <set>
#include <vector>
#include <string>
#include <utility>

#include "RbstNode.h"
#include "RbstCheck.h"
#include "RbstSet.h"


// Debug-dump tree structure and values:
template<class V>
std::ostream &operator<< (std::ostream &os, const RbstValuedNode<V> &node)
{
    os << '(';
    if (node.left()) os << *node.left();
    os << node.value();
    if (node.right()) os << *node.right();
    os << ')';
    return os;
}

template<class It>
static std::vector<int> get_contents(It begin, It end)
{
    std::vector<int> res;
    while (begin != end) res.push_back(*begin++);
    return res;
}

template<class Compare>
static void check(RbstSet<int, Compare> &set)
{
    assert(set.empty() == (set.size() == 0));
    const RbstTree<int, Compare> &tree = set.debug_tree();
    size_t max_depth = rbst_max_depth(&tree);
    assert(max_depth < 30);
    if (max_depth > 10)
    {
        // check tree is reasonably balanced:
        size_t total_depth = rbst_total_depth(&tree);
        size_t avg_depth = total_depth/set.size();
        assert(avg_depth <= 10 || set.size() > ((size_t)1 << (avg_depth/2)));
    }
    assert(rbst_check_structure(&tree));
    assert(rbst_check_values(tree.root(), tree.comp()));
}

static void test1()
{
    RbstSet<int> test;
    for (int i = 0; i < 1000; ++i)
    {
        assert(test.size() == i);
        test.insert(i);
        check(test);
        assert(*test.begin() == 0);
        assert(*test.rbegin() == i);
    }
    assert(test.size() == 1000);
    check(test);
    for (int i = 0; i < 1000; ++i)
    {
        assert(*test.begin() == i);
        assert(*test.rbegin() == 999);
        test.erase(i);
        assert(test.size() == 999 - i);
        check(test);
    }
    check(test);
}

// This checks set comparison operators (six in total) work correctly.
static void test2()
{
    int a_data[] = { 4,  8, 12 };
    int b_data[] = { 4,  7, 15 };
    int c_data[] = { 4,  9, 20 };
    int d_data[] = { 4,  8, 12, 13 };
    int e_data[] = { 12, 8, 4  };

    RbstSet<int> a(&a_data[0], &a_data[3]);
    RbstSet<int> b(&b_data[0], &b_data[3]);
    RbstSet<int> c(&c_data[0], &c_data[3]);
    RbstSet<int> d(&d_data[0], &d_data[4]);
    RbstSet<int> e(&e_data[0], &e_data[3]);

    assert(!(a == b) && a != b && a > b && a >= b && !(a < b) && !(a <= b));
    assert(!(b == a) && b != a && b < a && b <= a && !(b > a) && !(b >= a));
    assert(!(a == c) && a != c && a < c && a <= c && !(a > c) && !(a >= c));
    assert(!(c == a) && c != a && c > a && c >= a && !(c < a) && !(c <= a));
    assert(!(a == d) && a != d && a < d && a <= d && !(a > d) && !(a >= d));
    assert(!(d == a) && d != a && d > a && d >= a && !(d < a) && !(d <= a));
    assert((a == a) && !(a != a) && !(a > a) && (a >= a) && !(a < a) && (a <= a));
    assert((a == e) && !(a != e) && !(a > e) && (a >= e) && !(a < e) && (a <= e));
    assert((e == a) && !(e != a) && !(e < a) && (e <= a) && !(e > a) && (e >= a));
}

// Tests random-access iterators.
static void test3()
{
    RbstSet<int> test;
    for (int i = 0; i < 20; ++i)
        test.insert(7*i%20);

    // Check increment/decrement near end works.
    assert(++test.find(19) == test.end());
    assert(--test.end() == test.find(19));

    RbstSet<int>::const_iterator it = test.begin();
    for (int i = 0; i <= 20; ++i)
    {
        assert(it - test.begin() == i);
        assert(test.end() - it == 20 - i);
        RbstSet<int>::const_iterator jt = test.begin();
        for (int j = 0; j < 20; ++j)
        {
            assert(*(it + (j - i)) == j);
            assert(*((j - i) + it) == j);
            assert(*(it - (i - j)) == j);
            assert(it[j - i] == j);
            assert(it + (j - i) == jt);
            assert(it - jt == i - j);
            assert(jt - it == j - i);
            ++jt;
        }

        if (it != test.end())
        {
            RbstSet<int>::const_iterator old = it;
            ++it;

            // Test post-decrement/post-increment
            RbstSet<int>::const_iterator tmp2 = it;
            assert(tmp2-- == it);
            assert(tmp2   == old);
            assert(tmp2++ == old);
            assert(tmp2   == it);

            // Test pre-decrement/pre-increment
            RbstSet<int>::const_iterator tmp3 = it;
            assert(--tmp3 == old);
            assert(tmp3   == old);
            assert(++tmp3 == it);
            assert(tmp3   == it);
        }
    }

    // Test iterator comparison methods
    for (int i = 0; i <= 20; ++i)
    {
        for (int j = 0; j <= 20; ++j)
        {
            RbstSet<int>::const_iterator it = test.find(i), jt = test.find(j);
            assert(i == 20 ? it == test.end() : *it == i);
            assert(j == 20 ? jt == test.end() : *jt == j);
            assert((it == jt) == (i == j));
            assert((it != jt) == (i != j));
            assert((it <  jt) == (i <  j));
            assert((it <= jt) == (i <= j));
            assert((it >  jt) == (i > j));
            assert((it >= jt) == (i >= j));
        }
    }

    // Check += and -= operators.
    RbstSet<int>::const_iterator a = test.find(7), b = a, c = a;
    a += 5;
    b -= 7;
    c += 13;
    assert(a == test.begin() + 12 && *a == 12);
    assert(b == test.begin()      && *b == 0);
    assert(c == test.end());
}

// Test iterator dereference operators
static void test4()
{
    RbstSet<std::pair<int,int> > test;
    test.insert(std::make_pair(3, 7));
    test.insert(std::make_pair(3, 9));
    test.insert(std::make_pair(1, 10));
    RbstSet<std::pair<int,int> >::iterator it = test.begin();
    assert((*it).first == 1);
    assert((*it).second == 10);
    assert(it->first == 1);
    assert(it->second == 10);
    ++it;
    assert(it->first == 3);
    assert(it->second == 7);
    assert((*it).first == 3);
    assert((*it).second == 7);
    ++it;
    assert(it->first == 3);
    assert(it->second == 9);
    assert((*it).first == 3);
    assert((*it).second == 9);
    ++it;
    assert(it == test.end());
}

// Tests swapability.
// (This also tests assignment and copy constructors).
static void test5()
{
    int a_data[3] = { 3, 2, 1 };
    int b_data[4] = { 4, 5, 6, 7 };

    // Using RbstSet swap method:
    {
        RbstSet<int> a(&a_data[0], &a_data[3]);
        RbstSet<int> b(&b_data[0], &b_data[4]);
        const int *p = &*a.find(2);
        const int *q = &*b.find(6);
        assert(*p == 2 && *q == 6);
        assert(a.size() == 3 && b.size() == 4);
        assert(*a.begin() == 1 && *(a.begin() + 1) == 2 && *(a.begin() + 2) == 3);
        assert(*b.begin() == 4 && *(b.begin() + 1) == 5 && *(b.begin() + 2) == 6 && *(b.begin() + 3) == 7);
        a.swap(b);
        assert(p == &*b.find(2));
        assert(q == &*a.find(6));
        assert(a.size() == 4 && b.size() == 3);
        assert(*a.begin() == 4 && *(a.begin() + 1) == 5 && *(a.begin() + 2) == 6 && *(a.begin() + 3) == 7);
        assert(*b.begin() == 1 && *(b.begin() + 1) == 2 && *(b.begin() + 2) == 3);
        check(a);
        check(b);
    }

    // Using std::swap:
    {
        RbstSet<int> a(&a_data[0], &a_data[3]);
        RbstSet<int> b(&b_data[0], &b_data[4]);
        const int *p = &*a.find(2);
        const int *q = &*b.find(6);
        assert(*p == 2 && *q == 6);
        assert(a.size() == 3 && b.size() == 4);
        assert(*a.begin() == 1 && *(a.begin() + 1) == 2 && *(a.begin() + 2) == 3);
        assert(*b.begin() == 4 && *(b.begin() + 1) == 5 && *(b.begin() + 2) == 6 && *(b.begin() + 3) == 7);
        std::swap(a, b);
        assert(a.size() == 4 && b.size() == 3);
        assert(*a.begin() == 4 && *(a.begin() + 1) == 5 && *(a.begin() + 2) == 6 && *(a.begin() + 3) == 7);
        assert(*b.begin() == 1 && *(b.begin() + 1) == 2 && *(b.begin() + 2) == 3);
        assert(p == &*b.find(2));   // same pointer!
        assert(q == &*a.find(6));   // same pointer!
        check(a);
        check(b);
    }

    // Verify swapping through a temporary preserves values but not pointers:
    {
        RbstSet<int> a(&a_data[0], &a_data[3]);
        RbstSet<int> b(&b_data[0], &b_data[4]);
        const int *p = &*a.find(2);
        const int *q = &*b.find(6);
        assert(a.size() == 3 && b.size() == 4);
        assert(*a.begin() == 1 && *(a.begin() + 1) == 2 && *(a.begin() + 2) == 3);
        assert(*b.begin() == 4 && *(b.begin() + 1) == 5 && *(b.begin() + 2) == 6 && *(b.begin() + 3) == 7);
        RbstSet<int> c = a;
        a = b;
        b = c;
        assert(a.size() == 4 && b.size() == 3);
        assert(*a.begin() == 4 && *(a.begin() + 1) == 5 && *(a.begin() + 2) == 6 && *(a.begin() + 3) == 7);
        assert(*b.begin() == 1 && *(b.begin() + 1) == 2 && *(b.begin() + 2) == 3);
        assert(p != &*b.find(2));   // different pointer!
        assert(q != &*a.find(6));   // different pointer!
        check(a);
        check(b);
    }
}

// Test erasing of elements.
static void test6()
{
    RbstSet<int> test;
    for (int i = 0; i < 20; ++i)
        test.insert(7*i%20);
    check(test);

    // Ranges
    test.erase(test.begin(), test.begin() + 4);     // 4..19 left
    test.erase(test.end() - 3, test.end());         // 4..16 left
    check(test);

    // Values
    test.erase(8);          // 4..7, 9..16
    test.erase(9);          // 4..7, 10..16
    test.erase(13);         // 4..7, 10..12, 14..16
    check(test);

    // Iterators
    test.erase(test.begin());       // 5..7, 10..12, 14..16
    test.erase(--test.end());       // 5..7, 10..12, 14..15
    test.erase(test.find(5));       // 6..7, 10..12, 14..15
    test.erase(test.find(11));      // 6..7, 10, 12, 14..15
    check(test);

    // Check contents are as expected:
    std::vector<int> a = get_contents(test.begin(), test.end());
    int b_data[6] = { 6, 7, 10, 12, 14, 15 };
    std::vector<int> b(&b_data[0], &b_data[6]);
    assert(a == b);
}


/* A weird comparator that sorts odd numbers before even numbers. */
struct IntCompare
{
    IntCompare(int foo) { }  // disable hide default constructor

    bool operator() (int a, int b) const
    {
        return ((a&1) != (b&1)) ? ((a&1) > (b&1)) : (a < b);
    }
};

// Test custom comparators and const/reverse iterators
static void test7()
{
    IntCompare comp(7);
    std::set<int,IntCompare> reference(comp);
    RbstSet<int,IntCompare> test(comp);
    for (int n = 0; n < 1000; ++n)
    {
        int i = rand()%1000;
        test.insert(i);
        reference.insert(i);
    }
    check(test);
    assert(test.size() == reference.size());

    {   // Forward iterators
        std::set<int,IntCompare>::iterator it = reference.begin();
        RbstSet<int,IntCompare>::iterator jt = test.begin();
        while (it != reference.end() && jt != test.end())
        {
            assert(*it == *jt);
            ++jt;
            assert(jt == test.end() || (*it < *jt) || (*it%2 == 1 && *jt%2 == 0));
            ++it;
        }
        assert(it == reference.end() && jt == test.end());
    }

    {   // Bacwkard iterators
        std::set<int,IntCompare>::reverse_iterator it = reference.rbegin();
        RbstSet<int,IntCompare>::reverse_iterator jt = test.rbegin();
        while (it != reference.rend() && jt != test.rend())
        {
            assert(*it == *jt);
            ++jt;
            assert(jt == test.rend() || (*it > *jt) || (*it%2 == 0 && *jt%2 == 1));
            ++it;
        }
        assert(it == reference.rend() && jt == test.rend());
    }

    const std::set<int,IntCompare> &const_reference = reference;
    const RbstSet<int,IntCompare> &const_test = test;

    {   // Const forward iterators
        std::set<int,IntCompare>::const_iterator it = const_reference.begin();
        RbstSet<int,IntCompare>::const_iterator jt = const_test.begin();
        while (it != reference.end() && jt != test.end())
        {
            assert(*it == *jt);
            ++jt;
            assert(jt == const_test.end() || (*it < *jt) || (*it%2 == 1 && *jt%2 == 0));
            ++it;
        }
        assert(it == const_reference.end() && jt == const_test.end());
        assert(it == reference.end() && jt == test.end());
    }

    {   // Backward iterators
        std::set<int,IntCompare>::const_reverse_iterator it = const_reference.rbegin();
        RbstSet<int,IntCompare>::const_reverse_iterator jt = const_test.rbegin();
        while (it != const_reference.rend() && jt != const_test.rend())
        {
            assert(*it == *jt);
            ++jt;
            assert(jt == const_test.rend() || (*it > *jt) || (*it%2 == 0 && *jt%2 == 1));
            ++it;
        }
        assert(it == const_reference.rend() && jt == const_test.rend());
        assert(it == reference.rend() && jt == test.rend());
    }
}

// Random brute force testing
static void test8()
{
    typedef RbstSet<int>  set_t;
    typedef std::set<int> ref_t;

    set_t test;
    ref_t reference;

    for (int n = 0; n < 100000; ++n)
    {
        int i = rand()%1000;
        switch (rand()%3)
        {
        case 0:
            test.insert(i);
            reference.insert(i);
            break;

        case 1:
            test.erase(i);
            reference.erase(i);
            break;

        case 2:
            {
                ref_t::iterator it = reference.find(i);
                set_t::iterator jt = test.find(i);
                assert((it == reference.end()) == (jt == test.end()));
                assert((it == reference.end()) || *it == *jt);

                ref_t::iterator lo_it = reference.lower_bound(i);
                set_t::iterator lo_jt = test.lower_bound(i);
                assert((lo_it == reference.end()) == (lo_jt == test.end()));
                assert((lo_it == reference.end()) || *lo_it == *lo_jt);

                ref_t::iterator hi_it = reference.upper_bound(i);
                set_t::iterator hi_jt = test.upper_bound(i);
                assert((hi_it == reference.end()) == (hi_jt == test.end()));
                assert((hi_it == reference.end()) || *hi_it == *hi_jt);

                std::pair<ref_t::iterator,ref_t::iterator> pair1 = reference.equal_range(i);
                std::pair<set_t::iterator,set_t::iterator> pair2 = test.equal_range(i);
                assert(pair1.first == lo_it && pair1.second == hi_it);
                assert(pair2.first == lo_jt && pair2.second == hi_jt);
            } break;
        }
        if (n%1000 == 0) check(test);
    }
    check(test);
    assert(test.size() == reference.size());
}

struct TestValue
{
    static int constructed, destructed;

    TestValue(int j)
        : i(j) { ++constructed; }
    TestValue(const TestValue &f)
        : i(f.i) { ++constructed; }
    ~TestValue()
        { ++destructed; }

    bool operator<(const TestValue &f) const
        { return i < f.i; }

private:
    TestValue &operator=(const TestValue &f) const;
private:
    int i;
};

int TestValue::constructed = 0;
int TestValue::destructed = 0;

// Track allocations from the TestAllocator.
static std::set<std::pair<void*,size_t> > allocated;

template<class T>
struct TestAllocator : std::allocator<T>
{
    T *allocate(size_t n, void *hint = 0)
    {
        T *p = std::allocator<T>::allocate(n, hint);
        assert(p != NULL);
        allocated.insert(std::make_pair(p, n*sizeof(T)));
        return p;
    }

    void deallocate(T *p, size_t n)
    {
        std::set<std::pair<void*,size_t> >::iterator it
            = allocated.find(std::make_pair(p, n*sizeof(T)));
        assert(it != allocated.end());
        allocated.erase(it);
        std::allocator<T>::deallocate(p, n);
    }

    template <class Type> struct rebind {
        typedef TestAllocator<Type> other;
    };
};

/* Check that constructors/destructors are called as required. */
static void test9()
{
    {
        RbstSet<TestValue> test;
        for (int i = 0; i < 20; ++i) test.insert(TestValue(3*i%10));
        for (int i = 5; i < 10; ++i) test.erase(TestValue(i));
        for (int i = 0; i < 20; ++i) test.insert(TestValue(3*i%10));
        test.clear();
        for (int i = 0; i < 20; ++i) test.insert(TestValue(3*i%10));
    }
    assert(TestValue::constructed == TestValue::destructed);
    assert(allocated.empty());
    {
        RbstSet<TestValue, std::less<TestValue>, TestAllocator<int> > test;
        for (int i = 0; i < 20; ++i) test.insert(TestValue(3*i%10));
        assert(allocated.size() == 10);
        for (int i = 5; i < 10; ++i) test.erase(TestValue(i));
        assert(allocated.size() == 5);
        for (int i = 0; i < 20; ++i) test.insert(TestValue(3*i%10));
        assert(allocated.size() == 10);
        RbstSet<TestValue, std::less<TestValue>, TestAllocator<int> > test2 = test;
        assert(allocated.size() == 20);
        test.clear();
        assert(allocated.size() == 10);
        test = test2;
        assert(allocated.size() == 20);
    }
    assert(TestValue::constructed == TestValue::destructed);
    assert(allocated.empty());
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();

    // .check if tests cover all implemented methods (tedious...)
    // see also TODO's in RbstSet (and add testcases for them)
}
