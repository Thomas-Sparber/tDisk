/*
 *  randomtree.c - a randomised binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <stack.h>

#include <randomtree.h>

#define KEY_INCREMENT 100 /* How much to add to an existing key to create a new key */

typedef struct {
	MBrtnode *parent;
 	MBrtnode *node;
	unsigned int index;
	unsigned int depth;
	int direction;
	MBstack *stack;
} rtfind_info;

static rtfind_info *rtfind_info_create(void)
{
	rtfind_info *info = malloc(sizeof(rtfind_info));
	if (info) {
		info->parent = NULL;
		info->node = NULL;
		info->index = 0;
		info->depth = 0;
		info->direction = 0;
		info->stack = MBstack_create();
	}
	return info;
}

void rtfind_info_delete(rtfind_info *info)
{
	if (info) {
		MBstack_delete(info->stack);
		free(info);
	}
}

static MBrtnode * MBrtnode_create(void * data)
{
	MBrtnode * node = malloc(sizeof(MBrtnode));
	if (node) {
		node->left = NULL;
		node->right = NULL;
		node->key = 0;
		node->data = data;
	}
	return node;
}

MBrandomtree * MBrandomtree_create(MBcmpfn cmpfn)
{
	MBrandomtree * tree = malloc(sizeof(MBrandomtree));
	if (tree != NULL) {
		tree->root = NULL;
		tree->cmpfn = cmpfn;
		tree->count = 0;
	}
	return tree;
}


static void MBrandomtree_clear_recursive(MBrtnode * root)
{
	if (root != NULL) {
		MBrtnode * left = root->left;
		MBrtnode * right = root->right;
		free(root);
		MBrandomtree_clear_recursive(left);
		MBrandomtree_clear_recursive(right);
	}
}

static void MBrandomtree_clear(MBrandomtree * tree)
{
	MBrandomtree_clear_recursive(tree->root);
	tree->root = NULL;
	tree->count = 0;
}

void MBrandomtree_delete(MBrandomtree * tree)
{
	if (tree) {
		MBrandomtree_clear(tree);
		free(tree);
	}
}

static void MBrandomtree_for_each_recursive(const MBrtnode * root, MBforfn forfn)
{
	if (root) {
		MBrandomtree_for_each_recursive(root->left, forfn);
		forfn(root->data);
		MBrandomtree_for_each_recursive(root->right, forfn);
	}
}

void MBrandomtree_for_each(const MBrandomtree * tree, MBforfn forfn)
{
	MBrandomtree_for_each_recursive(tree->root, forfn);
}
static unsigned int MBrandomtree_count_recursive(const MBrtnode *root)
{
	unsigned int count = 0;
	if (root != NULL) {
		count = 1 + MBrandomtree_count_recursive(root->left) + MBrandomtree_count_recursive(root->right);
	}
	return count;
}

static void MBrandomtree_rotate_left(MBrandomtree *tree, MBrtnode *node, MBrtnode *parent)
{
	MBrtnode *right = node->right;
	if (node == tree->root) {
		tree->root = right;
	}
	else if (node == parent->left) {
		parent->left = right;
	}
	else {
		parent->right = right;
	}
	if (right->left) {
		node->right = right->left;
	}
	else {
		node->right = NULL;
	}
	right->left = node;
}

static void MBrandomtree_rotate_right(MBrandomtree *tree, MBrtnode *node, MBrtnode *parent)
{
	MBrtnode *left = node->left;
	if (node == tree->root) {
		tree->root = left;
	}
	else if (node == parent->left) {
		parent->left = left;
	}
	else {
		parent->right = left;
	}
	if (left->right) {
		node->left = left->right;
	}
	else {
		node->left = NULL;
	}
	left->right = node;
}

static void MBrandomtree_arrange(MBrandomtree *tree, MBstack *stack, MBrtnode *node)
{
	unsigned int finished = 0;
	while (node != tree->root && !finished) {
		MBrtnode *parent = MBstack_pop(stack);
		if (node->key > parent->key) {
			if (node == parent->left) {
				MBrandomtree_rotate_right(tree, parent, MBstack_peek(stack));
			}
			else {
				MBrandomtree_rotate_left(tree, parent, MBstack_peek(stack));
			}
		}
		else {
			finished = 1;
		}
	}
}

static rtfind_info *MBrandomtree_rtfind_info(const MBrandomtree * tree, const void * data)
{
	MBrtnode * current, * previous = NULL;
	int result = 0;
	unsigned int found = 0;
	unsigned int depth = 0;
	rtfind_info *info = rtfind_info_create();
	unsigned int pos = tree->root ? MBrandomtree_count_recursive(tree->root->left) : 0;

	current = tree->root;
	while (current != NULL && !found) {
		result = tree->cmpfn(current->data, data);
		if (result == 0) {
			found = 1;
		}
		else {
			MBstack_push(info->stack, current);
			previous = current;
			if (result > 0) {
				current = current->left;
				pos -= 1;
			   if (current)	{
				   pos -= MBrandomtree_count_recursive(current->right);
			   }
			}
			else {
				current = current->right;
				pos += 1;
			    if (current) {
					pos += MBrandomtree_count_recursive(current->left);
				}
			}
			depth++;
		}
	}
	info->parent = previous;
	info->node = current;
	info->index = pos;
	info->depth = depth;
	info->direction = result;

	return info;
}

void* MBrandomtree_add(MBrandomtree * tree, void * data)
{
	MBrtnode * node;
	void * temp = NULL;
	rtfind_info *info = MBrandomtree_rtfind_info(tree, data); 

	if (info->node == NULL) {
		/* Adding a new item */
		node = MBrtnode_create(data);
		if (tree->root == NULL) {
			tree->root = node;
		}
		else {
			if(info->direction > 0) {
				info->parent->left = node;
			}
			else {
				info->parent->right = node;
			}
		}
		node->key = rand();
		MBrandomtree_arrange(tree, info->stack, node);
		tree->count++;
	}
	else {
		/* Replacing an existing item */
		temp = info->node->data;
		info->node->data = data;
	}
	rtfind_info_delete(info);

	return temp;
}

void* MBrandomtree_find(MBrandomtree * tree, const void * data)
{
	void * found = NULL;

	rtfind_info *info = MBrandomtree_rtfind_info(tree, data);
	if (info->node) {
		found = info->node->data;
	}
	rtfind_info_delete(info);
	return found;
}

static int MBrtnode_max(const MBrtnode *first, const MBrtnode *second)
{
	int max = 0;
	if ((first && !second) || (first && second && first->key >= second->key)) {
		max = first->key;
	}
	else if ((second && !first) || (first && second && second->key >= first->key)) { 
		max = second->key;
	}
	return max;
}

static void MBrandomtree_remove_node(MBrandomtree *tree, MBrtnode * node, MBrtnode * parent)
{
	const unsigned int root = parent == NULL;
	if (node->right == NULL) {
		/* Only a left child or no children */
		if (root) {
			tree->root = node->left;
		}
		else if (node == parent->left) {
			parent->left = node->left;
		}
		else {
			parent->right = node->left;
		}
	}
	else if(node->left == NULL) {
		/* Only a right child */
		if (root) {
			tree->root = node->right;
		}
		else if (node == parent->left) {
			parent->left = node->right;
		}
		else {
			parent->right = node->right;
		}
	}
	else {
		/* 2 children */
		MBrtnode *current = node->left, *previous = NULL;
		/* Find the highest element in the node's left subtree */
		while (current->right != NULL) {
			previous = current;
			current = current->right;
		}
		if (previous != NULL) {
			/* Attach its left subtree in its place */
			previous->right = current->left;
		}
		/* Attach it as a child of parent */
		if (root) {
			tree->root = current;
		}
		else if (node == parent->left) {
			parent->left = current;
		}
		else {
			parent->right = current;
		}
		/* Attach the left and right subtrees */
		if (node->left != current) {
			current->left = node->left;
		}
		else {
			current->left = node->left->left;
		}
		current->right = node->right;
		/* Fix the key */
		if (current->left || current->right) {
			if (root) {
				/* Key must be greater than both child keys */
				current->key = MBrtnode_max(current->left, current->right) + KEY_INCREMENT;
			}
			else {
				/* Key must be between parent key and greatest child key */
				current->key = parent->key + 
					((MBrtnode_max(current->left, current->right) - parent->key) / 2);
			}
		}
	}
	free(node);
}


void* MBrandomtree_remove(MBrandomtree * tree, const void* data)
{
	void *temp = NULL;
	rtfind_info *info = MBrandomtree_rtfind_info(tree, data);

	if (info->node) {
		temp = info->node->data;
		MBrandomtree_remove_node(tree, info->node, info->parent);
		tree->count--;
	}
	rtfind_info_delete(info);

	return temp;
}

unsigned int MBrandomtree_get_count(const MBrandomtree *tree)
{
	return tree->count;
}

void *MBrandomtree_get_at(MBrandomtree *tree, unsigned int index)
{
	void *result = NULL;
	if (index < tree->count) {
		MBrtnode *node = tree->root;
		unsigned int pos = MBrandomtree_count_recursive(tree->root->left);
		while (pos != index) {
			if (index > pos) {
				node = node->right;
				pos = pos + 1 + MBrandomtree_count_recursive(node->left);
			}
			else {
				node = node->left;
				pos = pos - 1 - MBrandomtree_count_recursive(node->right);
			}
		}
		result = node->data;
	}
	return result;
}

int MBrandomtree_find_index(MBrandomtree *tree, const void * data)
{
	int result = -1;
	rtfind_info *info = MBrandomtree_rtfind_info(tree, data);
	if (info->node) {
		result = info->index;
	}
	rtfind_info_delete(info);
	return result;
}

static unsigned int MBrandomtree_check_keys_recursive(const MBrtnode *root) {
	if (root == NULL) {
		return 1;
	}
	else {
		return MBrandomtree_check_keys_recursive(root->left)
			&& MBrandomtree_check_keys_recursive(root->right)
			&& root->key >= MBrtnode_max(root->left, root->right);
	}
}

unsigned int MBrandomtree_check_keys(const MBrandomtree *tree)
{
	return MBrandomtree_check_keys_recursive(tree->root);
}

typedef struct {
	MBstack *stack;
	MBrtnode *current;
} randomtree_iterator;

static randomtree_iterator *randomtree_iterator_create(const MBrandomtree *tree)
{
	randomtree_iterator *bi = malloc(sizeof(randomtree_iterator));
	if (bi) {
		bi->stack = MBstack_create();
		bi->current = tree->root;
	}
	return bi;
}

static void randomtree_iterator_delete(randomtree_iterator *bi)
{
	if (bi) {
		MBstack_delete(bi->stack);
		free(bi);	
	}
}

static void *randomtree_iterator_get(randomtree_iterator *bi)
{
	void *data = NULL;

	while (bi->current) {
		MBstack_push(bi->stack, bi->current);
		bi->current = bi->current->left;
	}
	if (MBstack_peek(bi->stack)) {
		bi->current = MBstack_pop(bi->stack);
		data = bi->current->data;
		bi->current = bi->current->right;
	}

	return data;
}

MBiterator *MBrandomtree_iterator(const MBrandomtree *tree)
{
	MBiterator *it = MBiterator_create(randomtree_iterator_create(tree),
		(MBgetfn)randomtree_iterator_get, (MBdeletefn)randomtree_iterator_delete);

	return it;
}

