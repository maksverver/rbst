#ifndef RBST_NODE_H_INCLUDED
#define RBST_NODE_H_INCLUDED

#include <cstddef>
#include <algorithm>
#include <functional>

// Randomized Binary Search Tree implementation.

/* RbstNode models a tree node with associate size, and pointers to the
   parent node, left child node, and right child node. */
class RbstNode
{
public:
    RbstNode( RbstNode *left = NULL, RbstNode *right = NULL,
              RbstNode *parent = NULL )
        : m_left(left), m_right(right), m_parent(parent),
          m_size(1 + size(left) + size(right)) { }

    // Returns the size of the subtree rooted at this node:
    size_t size() const { return m_size; }

    /* Convenience method that returns the size of the subtree rooted at `node`
       or 0 if `node` is NULL: */
    static size_t size(const RbstNode *node) { return node ? node->m_size : 0; }

    RbstNode* left()    { return m_left; }
    RbstNode* right()   { return m_right; }
    RbstNode* parent()  { return m_parent; }

    const RbstNode* left() const    { return m_left; }
    const RbstNode* right() const   { return m_right; }
    const RbstNode* parent() const  { return m_parent; }

    const RbstNode *first() const   { return m_left ? m_left->first() : this; }
    const RbstNode *last() const    { return m_right ? m_right->last() : this; }

    /* Retrieve the successor/predecessor of this node, or NULL if this node is
       the last/first node in set order: */
    const RbstNode *next() const;
    const RbstNode *previous() const;

    /* Retrieve a successor/predecessor of this node at offset `d`, or NULL if
       the requested node is out of range.  For example, node->offset(1) is
       equivalent to node->offset(1), and node->previous() to node->offset(-1).
    */
    const RbstNode *offset(ptrdiff_t d) const;

    /* Returns the 0-based index of the current node in the tree, i.e. the
       index i such that root->at(i) == this */
    inline size_t index() const;

    /* Returns the node at the given 0-based index in the subtree rooted at
       this node.  When this node is the root of the tree (i.e. parent() ==
       NULL) then at(i) is equivalent to first()->offset(i). */
    inline RbstNode *at(size_t index);

    /* Removes this node from its tree, probabilistically merging its children
       into a new subtree to replace it, and returns the new root of the tree,
       which is different from the old root if this node was the old root. */
    template<class RNG>
    RbstNode *erase(RNG &rng);

    /* Inserts this node in the subtree at `node` with parent `parent`.
       Returs the new  root of the subtree, which is either `this` or `node`,
       depending on whether the current node has replaced `node` as the root
       of its subtree. */
    template<class NodeCompare, class RNG>
    RbstNode *insert( RbstNode *node, RbstNode *parent,
                      NodeCompare &compare, RNG &rng );

protected:
    template<class NodeCompare>
    void split( RbstNode &tree, RbstNode &lesser,
                                RbstNode &greater, NodeCompare &compare );

    template<class RNG>
    static RbstNode *join(RbstNode *lesser, RbstNode *greater, RNG &rng);

protected:
    RbstNode *m_left, *m_right, *m_parent;
    size_t m_size;

    template<class V, class Comparator> friend class RbstTree;
};

const RbstNode *RbstNode::previous() const
{
    if (m_left) return m_left->last();
    const RbstNode *node = this;
    while (node->m_parent && node == node->m_parent->m_left)
        node = node->m_parent;
    return node->m_parent;
}

const RbstNode *RbstNode::next() const
{
    if (m_right) return m_right->first();
    const RbstNode *node = this;
    while (node->m_parent && node == node->m_parent->m_right)
        node = node->m_parent;
    return node->m_parent;
}

const RbstNode *RbstNode::offset(ptrdiff_t d) const
{
    if (d > 0)
    {
        if (d <= size(m_right))
            return m_right->offset(d - 1 - size(m_right->m_left));
    }
    else
    if (d < 0)
    {
        if (-d <= size(m_left))
            return m_left->offset(d + 1 + size(m_left->m_right));
    }
    else
    {
        return this;  // d == 0
    }

    if (!m_parent)
        return NULL;

    return m_parent->offset( this == m_parent->m_left ? d - 1 - size(m_right)
                                                      : d + 1 + size(m_left) );
}

size_t RbstNode::index() const
{
    size_t index = size(m_left);
    for (const RbstNode *node = this; node->m_parent; node = node->m_parent)
    {
        if (node == node->m_parent->m_right)
            index += node->m_parent->m_size - node->m_size;
    }
    return index;
}

RbstNode *RbstNode::at(size_t index)
{
    size_t n = size(m_left);
    return (index < n) ? m_left->at(index)
         : (index > n) ? m_right->at(index - n - 1)
         : this;
}

/* Splits the given `tree`.  Nodes in the tree are attached into lesser/greater
   depending on how they compare to the current node.

   When called, lesser->m_right and greater->m_left are uninitialized and will
   be updated by this function.
*/
template<class NodeCompare>
void RbstNode::split( RbstNode &tree, RbstNode &lesser,
                                      RbstNode &greater, NodeCompare &compare )
{
    if (compare(this, &tree))
    {
        greater.m_left = &tree;
        tree.m_parent  = &greater;
        if (tree.m_left)
            split(*tree.m_left, lesser, tree, compare);
        else
            lesser.m_right = NULL;
    }
    else
    {
        lesser.m_right = &tree;
        tree.m_parent  = &lesser;
        if (tree.m_right)
            split(*tree.m_right, tree, greater, compare);
        else
            greater.m_left = NULL;
    }
    tree.m_size = 1 + size(tree.m_left) + size(tree.m_right);
}

template<class NodeCompare, class RNG>
RbstNode *RbstNode::insert( RbstNode *node, RbstNode *parent,
                            NodeCompare &compare, RNG &rng )
{
    if (!node || rng(1 + node->size()) == 0)
    {
        // Insert new node here.
        if (!node)
        {
            m_left  = NULL;
            m_right = NULL;
            m_size  = 1;
        }
        else
        {
            split(*node, *this, *this, compare);
            std::swap(m_left, m_right);
            m_size = 1 + size(m_left) + size(m_right);
        }
        m_parent = parent;
        return this;
    }
    else
    {
        // Insert in left/right subtree.
        if (compare(this, node))
            node->m_left = insert(node->m_left, node, compare, rng);
        else
            node->m_right = insert(node->m_right, node, compare, rng);
        ++node->m_size;
        return node;
    }
}

/* Probabilistically merges two random binary search trees, `lesser` and
   `greater`, where the elements of `lesser` are less than (or equal to) the
   elements of `greater`.  The result is another random binary search tree. */
template<class RNG>
RbstNode *RbstNode::join( RbstNode *lesser,
                          RbstNode *greater, RNG &rng )
{
    if (!lesser) return greater;
    if (!greater) return lesser;

    if (rng(lesser->m_size + greater->m_size) < lesser->m_size)
    {
        lesser->m_size += size(greater);
        lesser->m_right = join(lesser->m_right, greater, rng);
        lesser->m_right->m_parent = lesser;
        return lesser;
    }
    else
    {
        greater->m_size += size(lesser);
        greater->m_left = join(lesser, greater->m_left, rng);
        greater->m_left->m_parent = greater;
        return greater;
    }
}

template<class RNG>
RbstNode *RbstNode::erase(RNG &rng)
{
    RbstNode *parent = m_parent,
             *child = join(m_left, m_right, rng);

    m_parent = m_left = m_right = NULL;
    m_size = 1;

    if (child)
    {
        // Set correct parent for new child node:
        child->m_parent = parent;
    }
    if (parent)
    {
        // Replace either left or right node of parent node:
        if (parent->m_left == this)
            parent->m_left = child;
        else
            parent->m_right = child;

        // Adjust size of all nodes from parent to root:
        --parent->m_size;
        while (parent->m_parent)
        {
            parent = parent->m_parent;
            --parent->m_size;
        }
        return parent;
    }
    else
    {
        return child;
    }
}

/* An RbstValuedNode extends an RbstNode instance with a value of type V.
   As an extra requirement, the children of an RbstValuedNode<V> must also be
   RbstValuednode<V>s. */
template<class V>
class RbstValuedNode : public RbstNode
{
public:
    RbstValuedNode( const V &value, RbstNode *left = NULL,
                    RbstNode *right = NULL, RbstNode *parent = NULL )
        : RbstNode(left, right, parent), m_value(value) { }

    const V &value() const { return m_value; }

    // Access methods for left/right subtree pointers, as pointers to
    // RbstValuedNode<V> rather than RbstNode.
    const RbstValuedNode* left() const  { return static_cast<const RbstValuedNode*>(m_left); }
    const RbstValuedNode* right() const { return static_cast<const RbstValuedNode*>(m_right); }

    // For the search functions below, the comparator passed must be consistent
    // with the comparator which was used to construct the binary search tree!

    /* Searches the subtree rooted at `node` for a node with value equal to `v`,
       or returns `res` instead if this value is not found. */
    template<class Comparator>
    static inline const RbstNode *find( const RbstValuedNode *node,
        const V &value, Comparator &comp, const RbstNode *res = NULL );

    /* Returns a pointer to the first node with value no less than (i.e. greater
       than or equal to) `v`, or returns `res` if all nodes in the subtree are
       less than `v`. */
    template<class Comparator>
    static inline const RbstNode *lower_bound( const RbstValuedNode *node,
        const V &value, Comparator &comp, const RbstNode *res = NULL );

    /* Returns a pointer to the first node with value greater than `v` or `res`
       if all nodes in the subtree are less than or equal to `v`. */
    template<class Comparator>
    static inline const RbstNode *upper_bound( const RbstValuedNode *node,
            const V &value, Comparator &comp, const RbstNode *res = NULL );

protected:
    V m_value;
};


template<class V> template<class Comparator>
const RbstNode *RbstValuedNode<V>::find( const RbstValuedNode<V> *node,
    const V &value, Comparator &comp, const RbstNode *res )
{
    while (node)
    {
        if (comp(value, node->value()))
            node = node->left();
        else
        if (comp(node->value(), value))
            node = node->right();
        else
            return node;
    }
    return res;
}

template<class V> template<class Comparator>
const RbstNode *RbstValuedNode<V>::lower_bound( const RbstValuedNode<V> *node,
    const V &value, Comparator &comp, const RbstNode *res )
{
    while (node)
    {
        if (comp(node->value(), value))
            node = node->right();
        else
            res = node, node = node->left();
    }
    return res;
}

template<class V> template<class Comparator>
const RbstNode *RbstValuedNode<V>::upper_bound( const RbstValuedNode<V> *node,
    const V &value, Comparator &comp, const RbstNode *res )
{
    while (node)
    {
        if (comp(value, node->value()))
            res = node, node = node->left();
        else
            node = node->right();
    }
    return res;
}

/* Tree node that represents the root of a binary search tree, which is itself
   an RbstNode.  It stores a comparator in m_comp, a pointer to the values in
   m_left, and the size + 1 in m_size, while m_parent and m_right are always
   NULL.  All children must be instances of RbstValuedNode<V> and the binary
   search tree is ordered using the given comparator. */
template<class V, class Comparator>
class RbstTree : public RbstNode
{
public:
    RbstTree(const Comparator &comp, RbstValuedNode<V> *tree = NULL)
        : RbstNode(tree), m_comp(comp) { }

    template<class RNG>
    void insert(RbstValuedNode<V> &node, RNG &rng)
    {
        ++m_size;
        m_left = node.insert(m_left, this, *this, rng);
    }

    // RbstNode comparator.  This allows the tree to be used as the comparison
    // object passed to RbstNode::insert().
    bool operator() (RbstNode *left, RbstNode *right)
    {
        return m_comp( static_cast<RbstValuedNode<V>*>(left)->value(),
                       static_cast<RbstValuedNode<V>*>(right)->value() );
    }

    // Efficient swapping of contents.
    void swap(RbstTree &other)
    {
        std::swap(m_left, other.m_left);
        std::swap(m_size, other.m_size);
        std::swap(m_comp, other.m_comp);
        if (m_left) m_left->m_parent = this;
        if (other.m_left) other.m_left->m_parent = &other;
        // N.B. m_right and m_parent are NULL in both this and other.
    }

    const RbstValuedNode<V> *root() const
    {
        return static_cast<const RbstValuedNode<V>*>(m_left);
    }
    void set_root(RbstValuedNode<V> *node)
    {
        if (node) node->m_parent = this;
        m_left = node;
        m_size = 1 + size(node);
    }

    const Comparator &comp() const { return m_comp; }
    void set_comp(const Comparator &comp) { m_comp = comp; }

    const RbstNode *find(const V &v) const { return RbstValuedNode<V>::find(root(), v, m_comp, this); }
    const RbstNode *lower_bound(const V &v) const { return RbstValuedNode<V>::lower_bound(root(), v, m_comp, this); }
    const RbstNode *upper_bound(const V &v) const { return RbstValuedNode<V>::upper_bound(root(), v, m_comp, this); }

private:
    Comparator m_comp;
};

#endif  /* ndef RBST_NODE_H_INCLUDED */
