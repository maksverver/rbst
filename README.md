An RbstSet is a std::set-like data structure providing random access iterators.

This allows accessing elements by index, and resolving iterators to set indices.
Otherwise, this is intended to be a drop-in replacement for std::set with
comparable expected time complexity bounds on most operations.

To use this class, include "RbstSet.h", which implements the std::set-like
interface.  This will pull in "RbstNode.h", which implements the RBST itself.

Based on "Randomized Binary Search Trees" by Conrado Martínez & Salvador Roura.

http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.17.243
