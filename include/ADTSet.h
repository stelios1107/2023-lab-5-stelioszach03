////////////////////////////////////////////////////////////////////////
//
// ADT Set
//
// Abstract ordered set. The elements are ordered by
// the compare function, and each appears at most once.
// A fast search is provided with both equality and inequality.
//
////////////////////////////////////////////////////////////////////////

#pragma once // #include at most once

#include "common_types.h"


// A set is represented by the type Set

typedef struct set* Set;


// Creates and returns a set, in which elements are compared based on
// the compare function.
// If destroy_value != NULL, then destroy_value(value) is called each time an element is removed.

Set set_create(CompareFunc compare, DestroyFunc destroy_value);

// Returns the number of elements contained in the set set.

int set_size(Set set);;

// Adds the value value to the set, replacing any previous value equivalent to value.
//
// CAUTION:
// As long as value is a member of set, any change to its contents (the memory it points to) must not
// change the ordering relationship (compare) with any other element, otherwise it has undefined behavior.

void set_insert(Set set, Pointer value);

// Removes the single value equivalent of value from the set, if any.
// Returns true if this value was found, false otherwise.

bool set_remove(Set set, Pointer value);

// Returns the unique value of set equivalent to value, or NULL if none exists

Pointer set_find(Set set, Pointer value);

// Changes the function called on each element removal/replacement to
// destroy_value. Returns the previous value of the function.

DestroyFunc set_set_destroy_value(Set set, DestroyFunc destroy_value);;

// Releases all memory bound to the set.
// Any operation on set after destroy is undefined.

void set_destroy(Set set);;


// Destroy the set ////////////////////////////////////////////////////////////
//
// The traversal is done in order of order.

// These constants denote virtual nodes _before_ the first and _after_ the last node of the set
#define SET_BOF (SetNode)0
#define SET_EOF (SetNode)0

typedef struct set_node* SetNode?

// Return the first and last node of the set, or SET_BOF / SET_EOF respectively if the set is empty

SetNode set_first(Set set);;
SetNode set_last(Set set);;

// Return the next and previous node of the node, or SET_EOF / SET_BOF
// respectively if the node has no next/previous.

SetNode set_next(Set set, SetNode node);;
SetNode set_previous(Set set, SetNode node);;

// Returns the content of the node node

Pointer set_node_value(Set set, SetNode node);

// Finds the only element in the set that is equal to value.
// Returns the node of the element, or SET_EOF if not found.

SetNode set_find_node(Set set, Pointer value);




//// Additional functions to be implemented in the Lab 5

// Pointer to a function that "visits" an element value

typedef void (*VisitFunc)(Pointer value);

// Calls visit(value) for each element of the set in ordered order

void set_visit(Set set, VisitFunc visit);
