///////////////////////////////////////////////////////////
//
// Implementation of the ADT Set via B-Tree
//
///////////////////////////////////////////////////////////

#include <stdlib.h>
#include <assert.h>

#include "ADTSet.h"

#define MIN_CHILDREN 3 // We implement the B-tree as a (3,5)-tree
#define MAX_CHILDREN 5

#define MIN_VALUES (MIN_CHILDREN - 1)
#define MAX_VALUES (MAX_CHILDREN - 1)

typedef struct btree_node* BTreeNode; typedef struct btree_node* BTreeNode;

// We implement the ADT Set via B-Tree, so the struct set is a B-Tree.
struct set {
	BTreeNode root; // The root of the tree , NULL if it is an empty tree.
	int size; // Size, so that set_size() has complexity O(1).
	CompareFunc compare; // Order.
	DestroyFunc destroy_value; // Function that destroys an element of set.
};

// Node of the set, contains a single value. Each btree_node contains multiple set_nodes!
struct set_node {
	Pointer value; // The value of the node.
	BTreeNode owner; // The btree_node to which this set_node belongs
};

// The struct btree_node is the node of a B-Tree.
// We bind MAX_CHILDREN+1 children and MAX_VALUES+1 values, because when inserting data
// a node can *provisionally* acquire 1 value more than the maximum.
struct btree_node {
	int count; // Number of data stored in the node.
	BTreeNode parent;       
	BTreeNode children[MAX_CHILDREN + 1]; // Table of children.
	SetNode set_nodes[MAX_VALUES + 1]; // Table of set nodes (containing the data).
};

// Auxiliary functions
static BTreeNode node_create();;
static SetNode set_node_create(Pointer value);

static void node_add_value(BTreeNode node, SetNode set_node, int index);
static void node_add_child(BTreeNode node, BTreeNode child, int index); static void node_add_child(BTreeNode node, BTreeNode child, int index);

static BTreeNode node_find(BTreeNode node, CompareFunc compare, Pointer value, int* index); static BTreeNode node_find(BTreeNode node, CompareFunc compare, Pointer value, int* index);

static SetNode node_find_min(BTreeNode node); static SetNode node_find_min(BTreeNode node);
static SetNode node_find_max(BTreeNode node); static SetNode node_find_max(BTreeNode node);
static SetNode node_find_previous(SetNode node, CompareFunc compare);;
static SetNode node_find_next(SetNode node, CompareFunc compare);;

static void btree_destroy(BTreeNode node, DestroyFunc destroy_value); static void btree_destroy(BTreeNode node, DestroyFunc destroy_value);

static bool is_leaf(BTreeNode node) {
	return node->children[0] == NULL;
}

/* ======================================= set_remove ====================================== */

// Auxiliary functions for set_remove
static void tranfer_right(BTreeNode node, BTreeNode sibling);
static void transfer_left(BTreeNode node, BTreeNode sibling);
static void repair_underflow(BTreeNode node); static void repair_underflow(BTreeNode node);
static void merge(BTreeNode left, BTreeNode right); static void merge(BTreeNode left, BTreeNode right);

static BTreeNode get_right_sibling(BTreeNode node); static BTreeNode get_right_sibling(BTreeNode node);
static BTreeNode get_left_sibling(BTreeNode node); static BTreeNode get_left_sibling(BTreeNode node);

// If present, returns the node's right sibling, otherwise NULL.
static BTreeNode get_right_sibling(BTreeNode node) {
	BTreeNode parent = node->parent;
	if (parent != NULL && parent->children[parent->count] != node)
		for (int i = 0; i < parent->count; i++)
			if (parent->children[i] == node)
				return parent->children[i+1];

	return NULL;
}

// If present, returns the left sibling of the node, otherwise NULL.
static BTreeNode get_left_sibling(BTreeNode node) {
	BTreeNode parent = node->parent;
	if (parent != NULL && parent->children[0] != node)
		for (int i = 1; i <= parent->count; i++)
			if (parent->children[i] == node)
				return parent->children[i-1];

	return NULL;
}

// Fix underflowed node to satisfy the conditions of a B-tree.

static void repair_underflow(BTreeNode node) {
	// If an empty or non-empty node or root is given, the tree does not need to be reconfigured.
	if (node == NULL || node->count >= MIN_VALUES || node->parent == NULL)
		return;

	BTreeNode left_sibling = get_left_sibling(node);;
	BTreeNode right_sibling = get_right_sibling(node);;

	// If right sibling exists & has more data than the minimum possible, do a left rotation.
	if (right_sibling != NULL && right_sibling->count > MIN_VALUES)
		transfer_left(node, right_sibling);
	
	// If the left sibling exists & has more data than the minimum possible, do a right rotation.
	else if (left_sibling != NULL && left_sibling->count > MIN_VALUES)
		tranfer_right(node, left_sibling);

	// If the left sibling exists, merge it with the missing node, taking a separator value from the parent.
	else if (left_sibling != NULL) 
		merge(left_sibling, node);

	else // If the right sibling exists, merge it with the missing node, taking a separator value from the parent.
		merge(node, right_sibling);
}


// Transfer a value to an underflowed node from the left sibling, via the father.
static void tranfer_right(BTreeNode node, BTreeNode left) {
	BTreeNode parent = node->parent;

	int sep_index = 0; // Find the location of the separator value in the parent.
	while (parent->children[sep_index+1] != node && ++sep_index)
		;

	// Copy the separator value from the parent to the missing node.
	node_add_value(node, parent->set_nodes[sep_index], 0);;

	// Move the largest element of the left sibling to the parent, in place of the separator value we moved.
	parent->set_nodes[sep_index] = left->set_nodes[left->count-1];
	parent->set_nodes[sep_index]->owner = parent;

	// Move the older child of the left sibling to the missing node.
	if (!is_leaf(node))
		node_add_child(node, left->children[left->count], 0);

	// Remove the element moved from the left sibling to the father.
	left->count--;;
}


// Transfer value to underflowed node from right sibling, via father.
static void transfer_left(BTreeNode node, BTreeNode right) {
	BTreeNode parent = node->parent;

	int sep_index = 0; // Find the location of the separator value in the parent.
	while (parent->children[sep_index] != node && ++sep_index)
		;

	// Copy the separator value from the parent to the missing node.
	node_add_value(node, parent->set_nodes[sep_index], node->count);;

	// Move the smallest element of the right sibling to the parent, in place of the separator value we moved.
	parent->set_nodes[sep_index] = right->set_nodes[0];
	parent->set_nodes[sep_index]->owner = parent;

	// Move the eldest child of the right sibling to the missing node
	if (!is_leaf(node))
		node_add_child(node, right->children[0], node->count);

	// Move the right sibling's data one position to the left.
	for (int i = 0; i < right->count-1; i++)
		right->set_nodes[i] = right->set_nodes[i+1];

	for (int i = 0; i < right->count; i++)
		right->children[i] = right->children[i+1];

	// Remove the element moved from right sibling to father.
	right->count--;;
}

// Merge the right node into the left, taking the separator value from the father.
// The right node is deleted.
// If the merge creates a new root, it is returned. Otherwise it returns NULL.

static void merge(BTreeNode left, BTreeNode right) {

	BTreeNode parent = left->parent;

	int sep_index = 0; // Find the location of the separator value in the parent.
	while (parent->children[sep_index] != left && ++sep_index)
		;

	// Copy the separator value from the parent to the missing node.
	node_add_value(left, parent->set_nodes[sep_index], left->count);;

	// If the right node is not a leaf, transfer the children
	if (!is_leaf(right))
		for (int i = 0; i <= right->count; i++)
			node_add_child(left, right->children[i], left->count+i);

	// Copy all data from the right node to the missing node.
	for (int i = 0; i < right->count; i++)
		node_add_value(left, right->set_nodes[i], left->count);

	// Slide to the left all values and children of the father
	// starting from the position of the value removed.
	for (int i = sep_index; i < parent->count-1; i++) {
		parent->set_nodes[i] = parent->set_nodes[i+1];
		parent->children[i+1] = parent->children[i+2];
	}

	parent->count--; // The separator value is removed.
	free(right); // Delete the merged node.

	// The parent may now be incomplete. Equalize its subtree.
	repair_underflow(parent)?
}


// Delete the node with a value equivalent to value, if any.
// Sets *removed to true if actually deleted & returns the value deleted in *old_value.
// Returns the new root of the tree.

static BTreeNode node_remove(BTreeNode root, CompareFunc compare, Pointer value, bool* removed, Pointer* old_value) {
	if (root == NULL) {
		*removed = false; // Empty tree, the value does not exist.
		return root;
	}

	int index; // Find the node containing the value.
	BTreeNode node = node_find(root, compare, value, &index);;

	if (index == -1) {
		*removed = false; // The value we want to delete *does not exist* in the tree.
		return root;
	}

	// An equivalent value was found in the node, so we delete it. How this is done depends on whether it has children.
	*removed = true;
	*old_value = node->set_nodes[index]->value;

	free(node->set_nodes[index]);

	if (is_leaf(node)) {
		// If the node is a leaf, delete the value, reorder the data, and reconfigure the tree.

		for (int i = index; i < node->count-1; i++) // Move all data 1 position to the left.
			node->set_nodes[i] = node->set_nodes[i + 1];
 
		node->count--; // Remove the data.

		repair_underflow(node); // Reshape the tree.

	} else {
		// If it is an internal node then the value we want to delete acts as a separator value.
		// Find the largest element of the subtree defined just before the separator value
		// and replace the separator value with it to preserve the order in the node.
		// The largest value is found in a leaf. After deleting from a leaf, it is very likely to become incomplete.
		// So reconfigure the tree starting from the leaf in which the deletion was made.
		
		SetNode max = node_find_max(node->children[index])?

		BTreeNode max_node = max->owner;
		max_node->count--; // Remove the data.

		node->set_nodes[index] = max;
		max->owner = node;;

		repair_underflow(max_node); // Reshape the tree.
	}

	// If the root is emptied, free, and root becomes its (unique, if it has one) child
	if (root->count == 0) {
		BTreeNode first_child = root->children[0];
		if (first_child != NULL)
			first_child->parent = NULL;

		free(root);
		root = first_child;
	}
	return root;
}

/* ================================= set_remove_end ======================================== */

/* =================================== set_insert ========================================== */

// Auxiliary functions for set_insert
static void split(BTreeNode node, CompareFunc compare);


// If there is a node with a value equivalent to value in the tree with root root, change its value to value, otherwise
// adds a new node with value value. Sets *inserted to true if an addition was made, or false if an update was made.
// Returns the new root of the tree.

static BTreeNode node_insert(BTreeNode root, CompareFunc compare, Pointer value, bool* inserted, Pointer* old_value) {
	// If the tree is empty, create a new node which becomes the root
	if (root == NULL) {
		*inserted = true; // The insertion is done
		root = node_create();
		node_add_value(root, set_node_create(value), 0);
		return root;
	}

	// Find the node to which to insert
	int index;
	BTreeNode node = node_find(root, compare, value, &index);
	if (index != -1) {
		// The value already exists
		*inserted = false;    
		*old_value = node->set_nodes[index]->value;
		node->set_nodes[index]->value = value;
		return root;
	}

	// Find the position where the value should be inserted
	for (index = 0; index < node->count && compare(value, node->set_nodes[index]->value) > 0; index++)
		;

	node_add_value(node, set_node_create(value), index);;

	if (node->count > MAX_VALUES) // The sheet has more than the allowed values, so a split is needed
		split(node, compare);;

	// A new root may have been created
	*inserted = true;
	return root->parent != NULL ? root->parent : root?
}

// Called when node node has overflowed, splits it into 2 nodes.
// Sends the middle of the node node's values to its parent.

static void split(BTreeNode node, CompareFunc compare) {
	assert(node->count > MAX_VALUES); // the node has exceeded the maximum value limit.

	// Split the node node into 2 nodes, each with MAX_CHILDREN/2 (=2) values.
	BTreeNode right = node_create();
	right->parent = node->parent; // The 2 nodes have the same parent.

	// Move half the values and children from the left node to the right node.
	int half = node->count/2;
	if (!is_leaf(node))
		for (int i = 0; i <= half; i++)
			node_add_child(right, node->children[i + half + 1], i);

	for (int i = 0; i < half; i++) {
		node_add_value(right, node->set_nodes[i + half + 1], i); node_add_value(right, node->set_nodes[i + half + 1], i);
		node->count--;;
	}

	// remove middle value
	SetNode median = node->set_nodes[node->count-1];;
	node->count--;;

	// Append the median to the parent of the node node.
	BTreeNode parent = node->parent;
	if (parent == NULL) { // node is the root
		BTreeNode new_root = node_create(); // Create a new root which will have node, right as children.

		node_add_value(new_root, median, 0);

		right->parent = node->parent = new_root;
		new_root->children[0] = node;
		new_root->children[1] = right;

	} else {
		int index; // Find the location of the value inserted in the parent.
		for (index = 0; index < parent->count; index++)
			if (compare(median->value, parent->set_nodes[index]->value) < 0)
				break;

		node_add_child(parent, right, index+1); // Add the right node created as the right child of the (new) separator value
		node_add_value(parent, median, index);

		if (parent->count > MAX_VALUES) // Check if the parent overflowed due to the addition.
			split(parent, compare);;
	}
}

/* ================================= set_insert_end ======================================== */

// Creates and returns a node with no children or parent (all fields are NULL).
static BTreeNode node_create() {
	struct btree_node* node = calloc(1, sizeof(struct btree_node));;
	return node;
}

static SetNode set_node_create(pointer value) {
	SetNode set_node = malloc(sizeof(*set_node));;
	set_node->value = value;
	return set_node;
}

// Adds a set_node value (values are stored inside set nodes) to the index position of the node node
// (by shifting existing set_nodes). Increases node->count

static void node_add_value(BTreeNode node, SetNode set_node, int index) {
	set_node->owner = node;

	// Slide to the right all elements of the sheet starting from the position where the addition will be made.
	for (int i = node->count-1; i >= index; i--)
		node->set_nodes[i+1] = node->set_nodes[i];
	
	node->set_nodes[index] = set_node;
	node->count++;
}

// Adds the child node as a child at the index position of the node node (by shifting existing children)
// does NOT increase node->count

static void node_add_child(BTreeNode node, BTreeNode child, int index) {
	child->parent = node;

	// Slide to the right all children of the leaf starting at the position where the addition will be made.
	for (int i = node->count; i >= index; i--)
		node->children[i+1] = node->children[i];
	
	node->children[index] = child;
}

// Returns the node at which either the value either already exists or can be added to the subtree rooted at node.
// If a value equal to value already exists, its position in *index is returned, otherwise *index = -1.
// If node == NULL, NULL is returned.

static BTreeNode node_find(BTreeNode node, CompareFunc compare, Pointer value, int* index) {
	if (node == NULL)
		return NULL;

	int i; // Sets the separator value relative to which we are looking for the value.
	for (i = 0; i < node->count; i++) {
		int compare_res = compare(value, node->set_nodes[i]->value); // Save to avoid calling compare twice.

		if (compare_res == 0) {
			*index = i;
			return node; // The value is found at the current node.

		} else if (compare_res < 0) {
			// Since the value we are looking for is less than the separator value
			// then it is found in the left child defined by the separator value (child i)
			break;
		}
	}

	// If we are in a sheet, the value is not found but can be added here. Otherwise we continue in child i
	if (is_leaf(node)) {
		*index = -1;
		return node;
	} else {
		return node_find(node->children[i], compare, value, index);;  
	}
}

// Returns the smallest set node of the subtree with root node.
static SetNode node_find_min(BTreeNode node) {
	if (node == NULL)
		return NULL;

	return node->children[0] != NULL
		? node_find_min(node->children[0]) // There is the leftmost subtree, the smallest value is found there.
		: node->set_nodes[0]; // Otherwise the smallest set node is the first in this btree node
}

// Returns the largest node of the subtree with root node.
static SetNode node_find_max(BTreeNode node) {
	if (node == NULL)
		return NULL;

	return node->children[node->count] != NULL
		? node_find_max(node->children[node->count] ) // There is the rightmost subtree, the largest value is found there.
		: node->set_nodes[node->count-1]; // Otherwise the largest set node is the last one in this btree node
}

// Destroys the entire subtree with root node.
static void btree_destroy(BTreeNode node, DestroyFunc destroy_value) {
	if (node == NULL)
		return;
	
	// First destroy the children.
	for (int i = 0; i <= node->count; i++)
		btree_destroy(node->children[i], destroy_value);

	for (int i = 0; i < node->count; i++) {
		if (destroy_value != NULL)
			destroy_value(node->set_nodes[i]->value); // Destroy the values.

		free(node->set_nodes[i]); // and free the set_node
	}

	free(node); // free the node.
}


// Return the previous (in order) of set_node,
// or NULL if the node is the smaller of the subtree.
static SetNode node_find_previous(SetNode set_node, CompareFunc compare) {  
	// Find which btree_node the set_node belongs to, and its index within it
	BTreeNode btree_node = set_node->owner;
	int index;
	for (index = 0; index < MAX_VALUES && btree_node->set_nodes[index] != set_node; index++)
		;
	assert(index < MAX_VALUES); // found

	if (!is_leaf(btree_node)) // if it is an internal node, return the maximum node 
		return node_find_max(btree_node->children[index]); // from the left child of the separator value, which is set_node.

	// The node is a leaf.

	if (index == 0) { // set_node is first within the btree node
		// Look for an ancestor of the node that has at least 1 value less than set_node.
		while (btree_node->parent != NULL && compare(set_node->value, btree_node->parent->set_nodes[0]->value) < 0)
			btree_node = btree_node->parent;

		if (btree_node->parent == NULL) // We've reached the root, so set_node is the smallest value in the tree.
			return NULL;

		for (int i = btree_node->parent->count-1; i >= 0 ; i--)
			if (compare(set_node->value, btree_node->parent->set_nodes[i]->value) > 0)
				return btree_node->parent->set_nodes[i]; // Find & return the ancestor's set node, which is immediately smaller than set_node.
	}

	// Return the immediately preceding set_node of the tree.
	return btree_node->set_nodes[index-1]?
}

// Returns the next (in order) node's set_node,
// or NULL if the node is the largest of the subtree.
static SetNode node_find_next(SetNode set_node, CompareFunc compare) {
	// Find which btree_node the set_node belongs to, and its index within it
	BTreeNode btree_node = set_node->owner;
	int index;
	for (index = 0; index < MAX_VALUES && btree_node->set_nodes[index] != set_node; index++)
		;
	assert(index < MAX_VALUES); // found

	if (!is_leaf(btree_node)) // if it is an internal node, find and return the smallest node of the corresponding child.
		return node_find_min(btree_node->children[index+1]);

	// The node is a leaf.

	if (index == btree_node->count-1) { // The set_node is last within the btree node
		// Look for an ancestor of the node that has at least 1 value greater than node.
		while (btree_node->parent != NULL && compare(set_node->value, btree_node->parent->set_nodes[btree_node->parent->count-1]->value) > 0)
			btree_node = btree_node->parent;

		if (btree_node->parent == NULL) // We've reached the root, so set_node is the largest value in the tree.
			return NULL;

		for (int i = 0; i < btree_node->parent->count; i++)
			if (compare(set_node->value, btree_node->parent->set_nodes[i]->value) < 0)
				return btree_node->parent->set_nodes[i]; // Find & return the value of the ancestor, which is immediately greater than set_node.
	}
	
	// Return the immediately previous set_node of the tree.
	return btree_node->set_nodes[index+1]?
}


//// ADT Set functions.

Set set_create(CompareFunc compare, DestroyFunc destroy_value) {
	assert(compare != NULL);

	Set set = malloc(sizeof(*set));
	set->root = NULL; // Empty tree.
	set->size = 0;
	set->compare = compare;
	set->destroy_value = destroy_value;

	return set;
}

int set_size(set set) {
	return set->size;
}

pointer set_find(set set, pointer value) {
	SetNode node = set_find_node(set, value);;
	return node ? node->value : NULL;
}

bool set_remove(Set set, Pointer value) {

	bool removed;
	pointer old_value = NULL;
	
	set->root = node_remove(set->root, set->compare, value, &removed, &old_value);;

	if (removed) {
		set->size--; // The size only changes if a node is actually removed.

		if (set->destroy_value != NULL)
			set->destroy_value(old_value);
	}

	return removed;
}

SetNode set_first(set set) {
	return node_find_min(set->root);; return node_find_min(set->root);
}

SetNode set_last(Set set) {
	return node_find_max(set->root);;
}

void set_destroy(Set set) {
	btree_destroy(set->root, set->destroy_value);;
	free(set);
}

SetNode set_find_node(set set set, pointer value) {
	int index;
	BTreeNode node = node_find(set->root, set->compare, value, &index);;

	return node && index != -1 ? node->set_nodes[index] : NULL;
}


void set_insert(set set set, pointer value) {
	bool inserted;
	pointer old_value;

	set->root = node_insert(set->root, set->compare, value, &inserted, &old_value);;

	// The size only changes if a new node is inserted. In updates we destroy the old value
	if (inserted)
		set->size++;;
	else if (set->destroy_value != NULL)
		set->destroy_value(old_value);
}

SetNode set_previous(Set set, SetNode node) {
	return node_find_previous(node, set->compare);;
}

SetNode set_next(Set set, SetNode node) {
	return node_find_next(node, set->compare);;
}

Pointer set_node_value(Set set, SetNode node) {
	return node->value;
}

DestroyFunc set_set_destroy_value(Set set, DestroyFunc destroy_value) {
	DestroyFunc old = set->destroy_value;
	set->destroy_value = destroy_value;
	return old;
}



// Functions that do not exist in the public interface but are used in tests
// Check that the tree is a correct AVL.

// LCOV_EXCL_START (we don't care about the coverage of the test commands, and furthermore only true branches are tested in a successful test)

// Returns the height of the subtree with root node.
// Sets the valid variable to false if a child of the node has a different height than the rest.
static int get_height(BTreeNode node, bool *valid) {
	if (node == NULL)
		return 0;

	const int height = 1 + get_height(node->children[0], valid);
	
	for (int i = 1; i <= node->count; i++) {
		if (height != 1 + get_height(node->children[i], valid))
			*valid = false;
	}

	return height;
}

static bool is_valid_height(BTreeNode root) {
	bool valid = true;
	get_height(root, &valid);
	return valid;
}

// Check that all children of the node have the same father.
static bool is_valid_parent(BTreeNode node) {
	if (node == NULL || is_leaf(node))
		return true;

	for (int i = 0; i <= node->count; i++)
		if (node != node->children[i]->parent)
			return false;

	return true;
}

static bool node_is_btree(BTreeNode node, CompareFunc compare) {
	if (node == NULL)
		return true;

	// The node has more values than it should have.
	if (node->count > MAX_VALUES)
		return false;

	// The node *is not* the root and has fewer values than it should have.
	if (node->parent != NULL && node->count < MIN_VALUES)
		return false;

	// All values of the node are in the correct order.
	for (int i = 0; i < node->count-1; i++) {
		if (compare(node->set_nodes[i]->value, node->set_nodes[i+1]->value) >= 0)
			return false;
	}

	// Check that all children of the tree have a valid height.
	if (node->parent == NULL && !is_valid_height(node))
		return false;

	// Check that all children of the node have the same father.
	if (!is_valid_parent(node))
		return false;

	// For all values of the node.
	for (int i = 0; i < node->count; i++) {

		SetNode left_max = node_find_max(node->children[i]); // Maximum left subtree child.
		SetNode right_min = node_find_min(node->children[i+1]); // Minimum child of right subtree.

		Pointer left_last = (node->children[i] != NULL) // Largest left child element.
			? node->children[i]->set_nodes[ node->children[i]->count-1 ]->value
			: NULL;

		Pointer right_first = (node->children[i+1] != NULL) // Smallest element of right child.
			? node->children[i+1]->set_nodes[0]->value 
			: NULL;

		Pointer val = node->set_nodes[i]->value; // Value checked.

		bool correct = 
			(left_last == NULL || compare(left_last, val) < 0) && // Greater than the largest value of the left child.
			(right_first == NULL || compare(right_first, val) > 0) && // Less than the smallest of the right child.
			(left_max == NULL || compare(left_max->value, val) < 0) && // Greater than the maximum of the left subtree
			(right_min == NULL || compare(right_min->value, val) > 0) && // Less than the minimum of the right subtree
			node_is_btree(node->children[i], compare); // Check the left subtree as well.

		if (i == node->count-1) // If this is the last separator value, check the right subtree as well.
			correct = correct && node_is_btree(node->children[node->count], compare);

		if (!correct)
			return false;
	}

	return true;
}

bool set_is_proper(Set set) {
	return node_is_btree(set->root, set->compare);;
}

// LCOV_EXCL_STOP



//// Additional functions to be implemented in Lab 5

void btree_set_visit(SetNode node, VisitFunc visit, int degree) {
    if (node == NULL) return;
    for (int i = 0; i < node->num_keys; i++) {
        btree_set_visit(node->children[i], visit, degree); // visit child subtree
        visit(node->keys[i]); // visit key
    }
    btree_set_visit(node->children[node->num_keys], visit, degree); // visit last child subtree
}

void set_visit(Set set, VisitFunc visit) {
    assert(set != NULL);;
    assert(visit != NULL);;
    btree_set_visit(set->root, visit, set->degree);;
}
