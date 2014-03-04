#ifndef RBST_SET_H_INCLUDED
#define RBST_SET_H_INCLUDED

#include "RbstNode.h"
#include <stdint.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <functional>
#include <iterator>
#include <utility>

// For the randomized binary search tree, a random number generator is
// simply a functor that when passed a number n, generates a number uniformly
// at random between 0 and n (exclusive).  The arguments passed to the RNG
// correspond with subtree sizes.

// Linear congruential pseudo-random number generator.
template<class Type, Type A, Type C>
class LinearCongruentialGenerator
{
public:
    LinearCongruentialGenerator(Type seed = 1) : state(seed) { }
    size_t operator()(size_t bound) { state = A*state + C; return state%bound; }
private:
    Type state;
};

// Default RNG.  Parameters from Numerical Recipes.  Note that it has only
// 32-bit state, so it may not be ideal for very large sets!
typedef LinearCongruentialGenerator<uint32_t, 1664525, 1013904223> DefaultRng;

// Forward declaration of RbstSet class.
template< class Key,
          class Comparator = std::less<Key>,
          class Allocator = std::allocator<Key>,
          class Rng = DefaultRng >
class RbstSet;

// Iterator used by RbstSet class; a random-access iterator which is implemented
// as a thin wrapper around a RbstNode pointer.  Note that most random-access
// operations take O(log N) expected time, not O(1) time like in random-access
// containers such as std::vector. 
//
// Technically we'd need separate classes for const and non-const iterators,
// but since the keys in a set are const anyway, the only operation that is
// exclusive to non-const iterators is erasing elements.  To avoid code bloat,
// we'll just cast the const pointer to a non-const pointer to handle that
// case.
template<class V>
struct RbstSetIterator : std::iterator<std::random_access_iterator_tag, const V>
{
    RbstSetIterator(const RbstNode *n = NULL) : m_node(n) { }

    // Iterator comparisons:
    bool operator==(const RbstSetIterator &other) const { return m_node == other.m_node; }
    bool operator!=(const RbstSetIterator &other) const { return m_node != other.m_node; }
    bool operator< (const RbstSetIterator &other) const { return index() < other.index(); }
    bool operator> (const RbstSetIterator &other) const { return index() > other.index(); }
    bool operator<=(const RbstSetIterator &other) const { return m_node == other.m_node || index() <= other.index(); }
    bool operator>=(const RbstSetIterator &other) const { return m_node == other.m_node || index() >= other.index(); }

    // Accessing value (const only!)
    const V &operator* () const  { return static_cast<const RbstValuedNode<V>*>(m_node)->value(); }
    const V *operator-> () const { return &static_cast<const RbstValuedNode<V>*>(m_node)->value(); }

    RbstSetIterator &operator++ ()      { m_node = m_node->next();     return *this;}
    RbstSetIterator &operator-- ()      { m_node = m_node->previous(); return *this; }
    RbstSetIterator operator++ (int)    { RbstSetIterator old(m_node); m_node = m_node->next();     return old; }
    RbstSetIterator operator-- (int)    { RbstSetIterator old(m_node); m_node = m_node->previous(); return old; }

    // Iterator difference
    ptrdiff_t operator-(const RbstSetIterator &other) const
        { return (ptrdiff_t)index() - (ptrdiff_t)other.index(); }

    // Scalar addition/subtraction
    RbstSetIterator &operator+=(ptrdiff_t n) { m_node = m_node->offset(+n); return *this; }
    RbstSetIterator &operator-=(ptrdiff_t n) { m_node = m_node->offset(-n); return *this; }
    RbstSetIterator operator+(ptrdiff_t n) const { return RbstSetIterator(m_node->offset(+n)); }
    RbstSetIterator operator-(ptrdiff_t n) const { return RbstSetIterator(m_node->offset(-n)); }

    const V &operator[] (ptrdiff_t n) const { return *(*this + n); }

protected:
    size_t index() const { return m_node->index(); }

private:
    const RbstNode *m_node;

    // FIXME: I want to restrict Key to V, but I don't know how to do this!
    template<class Key, class Comparator, class Allocator, class Rng>
    friend class RbstSet;
};

template<class V>
RbstSetIterator<V> operator+(ptrdiff_t n, const RbstSetIterator<V> &it)
    { return it + n; }

// The RbstSet class proper.  This is an ordered container that is intended
// to be compatible with std::set, but has the added benefit that it provides
// random-access iterators.
template< class Key,
          class Comparator,
          class Allocator,
          class Rng >
class RbstSet
{
public:
    // Keys and values (single type, since this is a set, not a map.)
    typedef Key key_type, value_type;

    // Size and difference type.
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;

    // Comparator.
    typedef Comparator key_compare, value_compare;

    // Alocator.
    typedef Allocator allocator_type;

    // Reference/pointer types.
    typedef typename Allocator::reference        reference;
    typedef typename Allocator::const_reference  const_reference;
    typedef typename Allocator::pointer          pointer;
    typedef typename Allocator::const_pointer    const_pointer;

    // Iterators.
    typedef RbstSetIterator<Key> iterator, const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator, const_reverse_iterator;

    // Destructor.
    ~RbstSet() { clear(); }

    // Constructs an empty set.
    explicit RbstSet( const Comparator &comp = Comparator(),
                      const Allocator &alloc = Allocator(),
                      const Rng &rng = Rng() )
        : m_tree(comp), m_alloc(alloc), m_rng(rng), m_node_alloc()
    {
    }

    // Constructs a set with initial values.
    template<class InputIterator>
    RbstSet( InputIterator first, InputIterator last,
             const Comparator& comp = Comparator(),
             const Allocator& alloc = Allocator(),
             const Rng &rng = Rng() )
        : m_tree(comp), m_alloc(alloc), m_rng(rng), m_node_alloc()
    {
        insert(first, last);
    }

    // Copy constructor.
    RbstSet(const RbstSet &that)
        : m_tree(that.m_tree.comp()),
          m_alloc(that.m_alloc), m_rng(that.m_rng), m_node_alloc(that.m_node_alloc)
    {
        // Note: this must be done after initializing the rng/node allocator,
        //       otherwise cloning doesn't work correctly!
        m_tree.set_root(clone(that.m_tree.root()));
    }

    // Assignment operator.
    RbstSet &operator=(const RbstSet &that)
    {
        if (this != &that)
        {
            clear();
            m_tree.set_comp(that.m_tree.comp());
            m_tree.set_root(clone(that.m_tree.root()));
        }
        return *this;
    }

    // Iterators
    const_iterator          begin() const   { return const_iterator(m_tree.first()); }
    const_iterator          end() const     { return const_iterator(static_cast<const RbstNode*>(&m_tree)); }
    const_reverse_iterator  rbegin() const  { return const_reverse_iterator(end()); }
    const_reverse_iterator  rend() const    { return const_reverse_iterator(begin()); }

    // Size and capacity
    bool empty() const          { return m_tree.root() == NULL; }
    size_type size() const      { return m_tree.size() - 1; }
    size_type max_size() const  { return m_node_alloc.max_size(); }

    // Erases all elements.
    void clear()
    {
        free(const_cast<node_type*>(m_tree.root()));
        m_tree.set_root(NULL);
    }

    // Insert a value, and returns an iterator paired with a Boolean indicating
    // whether the element was newly added (true) or previously present (false).
    std::pair<iterator,bool> insert(const value_type &value)
    {
        // FIXME: insertion can be made more efficient by integrating
        //        the lookup with the insertion
        const RbstNode *node = m_tree.find(value);
        if (node != &m_tree)
        {
            return make_pair(iterator(node), false);
        }
        node_type *new_node = m_node_alloc.allocate(1);
        new (new_node) node_type(value);
        m_tree.insert(*new_node, m_rng);
        return make_pair(iterator(new_node), true);
    }

    // Insert a value near given 'position`.
    iterator insert(iterator position, const value_type& val)
    {
        /* Note: insertion with a position hint should be more efficient, but I
           have not figured out how to do this yet, so just insert regularly: */
        return insert(val).second;
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        /* Note: should use position hint to make insertion of sorted ranges
                 more efficient (as is officially required). */
        while (first != last) insert(*first++);
    }

    // Erasing at a specific position:
    void erase(iterator pos)
    {
        node_type *node = const_cast<node_type*>(static_cast<const node_type*>(pos.m_node));
        node->erase(m_rng);
        node->~node_type();
        m_node_alloc.deallocate(node, 1);
        pos.m_node = NULL;
    }

    // Erasing a range of elements:
    void erase(iterator first, iterator last)
    {
        // FIXME: we can erase ranges more efficiently!
        while (first != last) erase(first++);
    }

    /* Erases elements which equal `key` and returns the number of elements
       removed (0/1) */
    size_type erase(const key_type &key)
    {
        iterator it = find(key);
        if (it == end()) return 0;
        erase(it);
        return 1;
    }

    /* Efficiently swaps contents of two sets. */
    void swap(RbstSet &that)
    {
        if (this != &that)
        {
            m_tree.swap(that.m_tree);
            std::swap(m_alloc, that.m_alloc);
            std::swap(m_node_alloc, that.m_node_alloc);
        }
    }

    /* Returns how many elements in the set equal `key`. */
    size_type count(const Key &key) const
    {
        return node_type::find(key, m_tree.root(), NULL) != NULL;
    }

    // Search for elements:
    const_iterator find(const Key &key) const        { return iterator(m_tree.find(key)); }
    const_iterator lower_bound(const Key& key) const { return iterator(m_tree.lower_bound(key)); }
    const_iterator upper_bound(const Key& key) const { return iterator(m_tree.upper_bound(key)); }

    // Get range of equal elements:
    std::pair<const_iterator,const_iterator> equal_range(const Key& key) const
    {
        const_iterator lo = lower_bound(key), hi = lo;
        if (hi != end() && !m_tree.comp()(key, *hi)) ++hi;
        return std::make_pair(lo, hi);
    }

    // Access to comparators used:
    key_compare   key_comp() const   { return m_tree.comp(); }
    value_compare value_comp() const { return m_tree.comp(); }

    // Access to RNG used:
    Rng rng() const { return m_rng; }

    // For debugging:
    const RbstTree<Key, Comparator> &debug_tree() { return m_tree; }

protected:
    typedef RbstValuedNode<Key> node_type;
    typedef typename Allocator::template rebind<node_type>::other node_allocator_type;

    /* Returns a deep copy of a the subtree rooted at `node`, and sets the
       parent of the new root node (if not NULL) to `parent`. */
    node_type *clone(const node_type *node, node_type *parent = NULL)
    {
        if (!node) return NULL;
        node_type *copy = m_node_alloc.allocate(1);
        new (copy) node_type( node->value(), clone(node->left(), copy),
                              clone(node->right(), copy), parent );
        return copy;
    }

    // Frees all nodes in the subtree rooted at `node`.
    void free(node_type *node)
    {
        if (!node) return;
        free(const_cast<node_type*>(node->left()));
        free(const_cast<node_type*>(node->right()));
        node->~node_type();
        m_node_alloc.deallocate(node, 1);
    }

protected:
    RbstTree<Key, Comparator>           m_tree;
    allocator_type                      m_alloc;
    Rng                                 m_rng;
    node_allocator_type                 m_node_alloc;
};

// Comparison operators

template<class Key, class Comparator, class Allocator>
bool operator== ( const RbstSet<Key,Comparator,Allocator> &lhs,
                  const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return (&lhs == &rhs) || (lhs.size() == rhs.size() &&
        std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template<class Key, class Comparator, class Allocator>
bool operator!= ( const RbstSet<Key,Comparator,Allocator> &lhs,
                  const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return !(lhs == rhs);
}

template<class Key, class Comparator, class Allocator>
bool operator< ( const RbstSet<Key,Comparator,Allocator> &lhs,
                 const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return (&lhs != &rhs) && std::lexicographical_compare(
        lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<class Key, class Comparator, class Allocator>
bool operator> ( const RbstSet<Key,Comparator,Allocator> &lhs,
                 const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return rhs < lhs;
}

template<class Key, class Comparator, class Allocator>
bool operator<= ( const RbstSet<Key,Comparator,Allocator> &lhs,
                  const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return !(lhs > rhs);
}

template<class Key, class Comparator, class Allocator>
bool operator>= ( const RbstSet<Key,Comparator,Allocator> &lhs,
                  const RbstSet<Key,Comparator,Allocator> &rhs )
{
    return rhs <= lhs;
}

// std::swap() implementation:

namespace std
{
    template<class Key, class Comparator, class Allocator>
    inline void swap( RbstSet<Key,Comparator,Allocator> &lhs,
                      RbstSet<Key,Comparator,Allocator> &rhs )
    {
        lhs.swap(rhs);
    }
}

#endif /* ndef RBST_SET_H_INCLUDED */
