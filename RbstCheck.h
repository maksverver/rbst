#ifndef RBST_CHECK_H_INCLUDED
#define RBST_CHECK_H_INCLUDED

#include "RbstNode.h"
#include <iostream>

/* Checks the internal consistency of the RBST structure; if errors are found,
   a message is written to `os` and `false` is returned. */
bool rbst_check_structure( const RbstNode *node, const RbstNode *parent = NULL,
                           size_t index = 0, std::ostream &os = std::cerr )
{
    // Empty tree is valid.
    if (!node) return true;

    const RbstNode *left  = node->left(),
                   *right = node->right();

    // Check left subtree:
    if (left && !rbst_check_structure(left, node, index, os))
        return false;

    // Check invariants at current node:
    size_t node_index = index + RbstNode::size(left),
           node_size  = 1 + RbstNode::size(left) + RbstNode::size(right);
    if (node->parent() != parent)
    {
        os << "Incorrect parent at node " << node_index << " (" << node << "): "
           << node->parent() << " (should be: " << parent << ")\n";
        return false;
    }
    if (node->size() != node_size)
    {
        os << "Incorrect size at node " << node_index << " (" << node << "): "
           << node->size() << " (should be: " << node_size << ")\n";
        return false;
    }

    // Check right subtree:
    if (right && !rbst_check_structure(right, node, node_index + 1, os))
        return false;

    return true;
}

/* Checks the ordering of values in the RBST; if errors are found, a message is
   written to `os` and `false` is returned. */
template<class V, class Compare>
bool rbst_check_values( const RbstValuedNode<V> *node, Compare &comp,
                        size_t index = 0, std::ostream &os = std::cerr )
{
    // Empty tree is valid.
    if (!node) return true;

    const RbstValuedNode<V> *left  = node->left(),
                            *right = node->right();

    // Check left subtree:
    if (left && !rbst_check_values(left, comp, index, os))
        return false;

    // Check invariants at current node:
    size_t node_index = index + RbstNode::size(left);
    if (left && comp(node->value(), left->value()))
    {
        os << "Value at node " << node_index << " (" << node << ")"
           << " is less than value at left child node.\n";
        return false;
    }
    if (right && comp(right->value(), node->value()))
    {
        os << "Value at node " << node_index << " (" << node << ")"
           << " is greater than value at right child node.\n";
        return false;
    }

    // Check right subtree:
    if (right && !rbst_check_values(right, comp, node_index + 1, os))
        return false;

    return true;
}

template<class V>
bool rbst_check_values( const RbstValuedNode<V> *node,
                        size_t index = 0, std::ostream &os = std::cerr )
{
    std::less<V> comp;
    return rbst_check_values<V, std::less<V> >(node, index, comp, os);
}

// Returns the maximum depth of a tree.
static size_t rbst_max_depth(const RbstNode *node)
{
    return node ? 1 + std::max( rbst_max_depth(node->left()),
                                rbst_max_depth(node->right()) ) : 0;
}

// Returns the total depth of a tree, which is defined as the sum of the
// depths of all nodes of the tree, with the root node at depth 1.
static unsigned long long rbst_total_depth( const RbstNode *node,
                                            unsigned long long depth = 0 )
{
    return node ? (1 + depth) + rbst_total_depth(node->left(),  depth + 1)
                              + rbst_total_depth(node->right(), depth + 1) : 0;
}

#endif   /* ndef RBST_CHECK_H_INCLUDED */
