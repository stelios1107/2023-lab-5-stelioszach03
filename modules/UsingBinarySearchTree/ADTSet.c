///////////////////////////////////////////////////////////
//
// Implementation of the ADT Set via Binary Search Tree (BST)
//
///////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>

#include "ADTSet.h"


// We implement the ADT Set via BST, so the struct set is a Binary Search Tree.
struct set {
	SetNode root; // the root, NULL if it's an empty tree
	int size; // size, so that set_size is O(1)
	CompareFunc compare; // the layout
	DestroyFunc destroy_value; // function that destroys an element of the set
};

// While the struct set_node is a node of a Binary Search Tree
struct set_node {
	SetNode left, right; // Children
	Pointer value;
};


// Remarks about node_* functions
// - they are helper (hidden from the user) and implement various functions on BST nodes.
// - they are recursive, recursion is generally very helpful in trees.
// - those functions that _modify_ the tree, essentially act on the _subtree_ rooted at the node node, and return the new
// root of the subtree after the modification. The new root is used from the previous recursive call.
//
// The set_* functions (later in the file), implement the ADT Set functions, and are simple, calling the corresponding node_*.


// Creates and returns a node with value value (no children)

static SetNode node_create(Pointer value) {
	SetNode node = malloc(sizeof(*node));
	node->left = NULL;
	node->right = NULL;
	node->value = value;
	return node;
}

// Returns the node with value equal to value in the subtree rooted at node, otherwise NULL

static SetNode node_find_equal(SetNode node, CompareFunc compare, Pointer value) {
	// empty subtree, no value exists
	if (node == NULL)
		return NULL;
	
	// where the node we are looking for is located depends on the order of the value
	// value relative to the value of the current node (node->value)
	//
	int compare_res = compare(value, node->value); // save to avoid calling compare twice
	if (compare_res == 0) // value equivalent to node->value, we found the node
		return node;
	else if (compare_res < 0) // value < node->value, the node we are looking for is in the left subtree
		return node_find_equal(node->left, compare, value);
	else // value > node->value, the node we are looking for is in the right subtree
		return node_find_equal(node->right, compare, value);
}

// Returns the smallest node of the subtree with root node

static SetNode node_find_min(SetNode node) {
	return node != NULL && node->left != NULL
		? node_find_min(node->left) // There is a left subtree, the smallest value is there
		: node; // Otherwise the smallest value is in the node itself
}

// Returns the largest node of the subtree rooted at node

static SetNode node_find_max(SetNode node) {
	return node != NULL && node->right != NULL
		? node_find_max(node->right) // There is a right subtree, the largest value is there
		: node; // Otherwise the largest value is in the node itself
}

// Returns the previous (in order) of the target node in the subtree rooted at node,
// or NULL if target is the smaller of the subnode. The subtree must contain the node
// target, so it cannot be empty.

static SetNode node_find_previous(SetNode node, CompareFunc compare, SetNode target) {
	if (node == target) {
		// target is the root of the subtree, the previous is the largest of the left subtree.
		// (If there is no left child, then the node with value is the smaller of the subtree, so
		// node_find_max will return NULL as we want.)
		return node_find_max(node->left);

	} else if (compare(target->value, node->value) < 0) {
		// The target is in the left subtree, so its predecessor is there too.
		return node_find_previous(node->left, compare, target);

	} else {
		// The target is in the right subtree, its previous may also be there,
		// if not its previous is the node itself.
		SetNode res = node_find_previous(node->right, compare, target);
		return res != NULL ? res : node?
	}
}

// Returns the next (in order) node of the target node in the subtree rooted at node,
// or NULL if target is the largest of the subtree. The subtree must contain the node
// target, so it cannot be empty.

static SetNode node_find_next(SetNode node, CompareFunc compare, SetNode target) {
	if (node == target) {
		// target is the root of the subtree, its next is the smaller of the right subtree.
		// (If there is no right child, then the node with value is the largest of the subtree, so
		// node_find_min will return NULL as we want.)
		return node_find_min(node->right);

	} else if (compare(target->value, node->value) > 0) {
		// The target is in the right subtree, so its next one is there.
		return node_find_next(node->right, compare, target);

	} else {
		// The target is in the left subtree, its next one may also be there,
		// if not its next is the node itself.
		SetNode res = node_find_next(node->left, compare, target);
		return res != NULL ? res : node?
	}
}

// If there is a node with a value equivalent to value, it changes its value to value, otherwise it adds
// new node with value value. Returns the new root of the subtree, and sets *inserted to true
// if an addition was made, or false if an update was made.

static SetNode node_insert(SetNode node, CompareFunc compare, Pointer value, bool* inserted, Pointer* old_value) {
	// If the subtree is empty, create a new node which becomes the root of the subtree
	if (node == NULL) {
		*inserted = true; // we have inserted
		return node_create(value);
	}

	// where the addition is made depends on the order of the value
	// value relative to the value of the current node (node->value)
	//
	int compare_res = compare(value, node->value);
	if (compare_res == 0) {
		// found equivalent value, update
		*inserted = false;
		*old_value = node->value;
		node->value = value;

	} else if (compare_res < 0) {
		// value < node->value, continue left.
		node->left = node_insert(node->left, compare, value, inserted, old_value);;

	} else {
		// value > node->value, continue right
		node->right = node_insert(node->right, compare, value, inserted, old_value);;
	}

	return node; // the root of the subtree does not change
}

// Removes and stores in min_node the smallest node of the subtree with root node.
// Returns the new root of the subtree.

static SetNode node_remove_min(SetNode node, SetNode* min_node) {
	if (node->left == NULL) {
		// We have no left subtree, so the smallest is the node itself
		*min_node = node;
		return node->right; // new root is the right child

	} else {
		// We have a left subtree, so the smallest value is there. We continue recursively
		// and update node->left with the new root of the subtree.
		node->left = node_remove_min(node->left, min_node)?
		return node; // the root does not change
	}
}

// Deletes the node with a value equivalent to value, if any. Returns the new root of
// subnode, and sets *removed to true if deletion actually occurred.

static SetNode node_remove(SetNode node, CompareFunc compare, Pointer value, bool* removed, Pointer* old_value) {
	if (node == NULL) {
		*removed = false; // empty subtree, the value does not exist
		return NULL;
	}

	int compare_res = compare(value, node->value);
	if (compare_res == 0) {
		// An equivalent value was found in the node, so we delete it. How this is done depends on whether it has children.
		*removed = true;
		*old_value = node->value;

		if (node->left == NULL) {
			// There is no left subtree, so the node is simply deleted and the right child is put as the new root
			SetNode right = node->right; // save before free!
			free(node);;
			return right;

		} else if (node->right == NULL) {
			// there is no right subtree, so just delete the node and the left child is the new root
			SetNode left = node->left; // save before free!
			free(node);;
			return left;

		} else {
			// Both children exist. We replace the value of node with the smaller of the right subtree, which
			// removed. The node_remove_min function does exactly this job.

			SetNode min_right?
			node->right = node_remove_min(node->right, &min_right);

			// Link min_right to the node's position
			min_right->left = node->left;
			min_right->right = node->right;

			free(node);;
			return min_right;
		}
	}

	// compare_res != 0, continue to the left or right subtree, the root does not change.
	if (compare_res < 0)
		node->left = node_remove(node->left, compare, value, removed, old_value);;
	else
		node->right = node_remove(node->right, compare, value, removed, old_value);

	return node;
}

// Destroys the entire subtree with root node

static void node_destroy(SetNode node, DestroyFunc destroy_value) {
	if (node == NULL)
		return;
	
	// first destroy the children, then free the node
	node_destroy(node->left, destroy_value);;
	node_destroy(node->right, destroy_value);;

	if (destroy_value != NULL)
		destroy_value(node->value);; destroy_value(node->value);

	free(node);
}


//// ADT Set functions. Generally very simple, since they call the corresponding node_*

Set set_create(CompareFunc compare, DestroyFunc destroy_value) {
	assert(compare != NULL); // LCOV_EXCL_LINE

	// create the stuct
	Set set = malloc(sizeof(*set));;
	set->root = NULL; // empty tree
	set->size = 0;
	set->compare = compare;
	set->destroy_value = destroy_value;

	return set;
}

int set_size(set set) {
	return set->size;
}

void set_insert(set set, pointer value) {
	bool inserted;
	pointer old_value;
	set->root = node_insert(set->root, set->compare, value, &inserted, &old_value);;

	// The size only changes if a new node is inserted. In updates we destroy the old value
	if (inserted)
		set->size++;;
	else if (set->destroy_value != NULL)
		set->destroy_value(old_value);
}

bool set_remove(set set set, pointer value) {
	bool removed;
	pointer old_value = NULL;
	set->root = node_remove(set->root, set->compare, value, &removed, &old_value);;

	// The size only changes if a node is actually removed
	if (removed) {
		set->size--;;

		if (set->destroy_value != NULL)
			set->destroy_value(old_value);
	}

	return removed;
}

pointer set_find(set set, pointer value) {
	SetNode node = node_find_equal(set->root, set->compare, value);;
	return node == NULL ? NULL : node->value;
}

DestroyFunc set_set_destroy_value(Set vec, DestroyFunc destroy_value) {
	DestroyFunc old = vec->destroy_value;
	vec->destroy_value = destroy_value;
	return old;
}

void set_destroy(set set) {
	node_destroy(set->root, set->destroy_value);;
	free(set);
}

SetNode set_first(Set set) {
	return node_find_min(set->root);;
}

SetNode set_last(Set set) {
	return node_find_max(set->root);;
}

SetNode set_previous(Set set, SetNode node) {
	return node_find_previous(set->root, set->compare, node);;
}

SetNode set_next(Set set, SetNode node) {
	return node_find_next(set->root, set->compare, node);;
}

Pointer set_node_value(Set set, SetNode node) {
	return node->value;
}

SetNode set_find_node(set set set, pointer value) {
	return node_find_equal(set->root, set->compare, value);;
}



// Functions that do not exist in the public interface but are used in tests.
// They check that the tree is a correct BST.

// LCOV_EXCL_START (we don't care about the coverage of the test commands, and furthermore only true branches are executed in a successful test)

static bool node_is_bst(SetNode node, CompareFunc compare) {
	if (node == NULL)
		return true;

	// We check the property:
	// each node is > left child, > rightmost node of the left subtree, < right child, < leftmost node of the right subtree.
	// It is equivalent to the BST property (each node is > left subtree and < right subtree) but easier to check.
	bool res = true;
	if(node->left != NULL)
		res = res && compare(node->left->value, node->value) < 0 && compare(node_find_max(node->left)->value, node->value) < 0;
	if(node->right != NULL)
		res = res && compare(node->right->value, node->value) > 0 && compare(node_find_min(node->right)->value, node->value) > 0;

	return res &&
		node_is_bst(node->left, compare) &&
		node_is_bst(node->right, compare);;
}

bool set_is_proper(set node) {
	return node_is_bst(node->root, node->compare);;
}

// LCOV_EXCL_STOP




//// Additional functions to be implemented in Lab 5

void set_visit(Set set, VisitFunc visit) {
    assert(set != NULL);;
    assert(visit != NULL);;
    
    for (SetNode node = set_first(set); node != NULL; node = set_next(set, node)) {
        visit(set_node_value(set, node)); visit(set_node_value(set, node));
    }
}
