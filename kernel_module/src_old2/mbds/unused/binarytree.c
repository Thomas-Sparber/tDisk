/*
 *  binarytree.c - an unbalanced binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>
#include <stdio.h>

#include <stack.h>

#include <binarytree.h>

typedef struct {
	MBbtnode *parent;
 	MBbtnode *node;
	unsigned int index;
	unsigned int depth;
	int direction;
} find_info;


static MBbtnode * MBbtnode_create(void * data)
{
	MBbtnode * node = malloc(sizeof(MBbtnode));
	if (node) {
		node->left = NULL;
		node->right = NULL;
		node->data = data;
	}
	return node;
}

MBbinarytree * MBbinarytree_create(MBcmpfn cmpfn)
{
	MBbinarytree * tree = malloc(sizeof(MBbinarytree));
	if (tree != NULL) {
		tree->root = NULL;
		tree->cmpfn = cmpfn;
		tree->count = 0;
	}
	return tree;
}


static void MBbinarytree_clear_recursive(MBbtnode * root)
{
	if (root != NULL) {
		MBbtnode * left = root->left;
		MBbtnode * right = root->right;
		free(root);
		MBbinarytree_clear_recursive(left);
		MBbinarytree_clear_recursive(right);
	}
}

static void MBbinarytree_clear(MBbinarytree * tree)
{
	MBbinarytree_clear_recursive(tree->root);
	tree->root = NULL;
	tree->count = 0;
}

void MBbinarytree_delete(MBbinarytree * tree)
{
	if (tree) {
		MBbinarytree_clear(tree);
		free(tree);
	}
}

static void MBbinarytree_for_each_recursive(const MBbtnode * root, MBforfn forfn)
{
	if (root) {
		MBbinarytree_for_each_recursive(root->left, forfn);
		forfn(root->data);
		MBbinarytree_for_each_recursive(root->right, forfn);
	}
}

void MBbinarytree_for_each(const MBbinarytree * tree, MBforfn forfn)
{
	MBbinarytree_for_each_recursive(tree->root, forfn);
}
static unsigned int MBbinarytree_count_recursive(const MBbtnode *root)
{
	unsigned int count = 0;
	if (root != NULL) {
		count = 1 + MBbinarytree_count_recursive(root->left) + MBbinarytree_count_recursive(root->right);
	}
	return count;
}

static find_info MBbinarytree_find_info(const MBbinarytree * tree, const void * data)
{
	MBbtnode * current, * previous = NULL;
	int result = 0;
	unsigned int found = 0;
	unsigned int depth = 0;
	find_info info;
	unsigned int pos = tree->root ? MBbinarytree_count_recursive(tree->root->left) : 0;

	current = tree->root;
	while (current != NULL && !found) {
		result = tree->cmpfn(current->data, data);
		if (result == 0) {
			found = 1;
		}
		else {
			previous = current;
			if (result > 0) {
				current = current->left;
				pos -= 1;
			   if (current)	{
				   pos -= MBbinarytree_count_recursive(current->right);
			   }
			}
			else {
				current = current->right;
				pos += 1;
			    if (current) {
					pos += MBbinarytree_count_recursive(current->left);
				}
			}
			depth++;
		}
	}
	info.parent = previous;
	info.node = current;
	info.index = pos;
	info.depth = depth;
	info.direction = result; 

	return info;
}

void* MBbinarytree_add(MBbinarytree * tree, void * data)
{
	MBbtnode * node;
	void * temp = NULL;
	find_info info = MBbinarytree_find_info(tree, data); 
	if (info.node == NULL) {
		/* Adding a new item */
		node = MBbtnode_create(data);
		if (tree->root == NULL) {
			tree->root = node;
		}
		else {
			/*const int result = tree->cmpfn(data, info.parent->data);*/
			if(info.direction > 0) {
				info.parent->left = node;
			}
			else {
				info.parent->right = node;
			}
		}
		tree->count++;
	}
	else {
		/* Replacing an existing item */
		temp = info.node->data;
		info.node->data = data;
	}
	return temp;
}

void* MBbinarytree_find(const MBbinarytree * tree, const void * data)
{
	void * found = NULL;

	find_info info = MBbinarytree_find_info(tree, data);
	if (info.node) {
		found = info.node->data;
	}
	return found;
}

static void MBbinarytree_remove_node(MBbinarytree *tree, MBbtnode * node, MBbtnode * parent)
{
	const unsigned int root = parent == NULL;
	if (node->right == NULL) {
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
		MBbtnode *current = node->left, *previous = NULL;
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
	}
	free(node);
}


void* MBbinarytree_remove(MBbinarytree * tree, const void* data)
{
	void *temp = NULL;
	find_info info = MBbinarytree_find_info(tree, data);

	if (info.node) {
		temp = info.node->data;
		MBbinarytree_remove_node(tree, info.node, info.parent);
		tree->count--;
	}
	return temp;
}

unsigned int MBbinarytree_get_count(const MBbinarytree *tree)
{
	return tree->count;
}

static void MBbinarytree_load_recursive(MBbtnode ** node, void ** list, size_t size)
{
	if (size > 0) {
		unsigned int left, right, middle = size / 2;
		*node = MBbtnode_create(list[middle]);
		left = middle;
		right = size - middle - 1;
		MBbinarytree_load_recursive(&((*node)->left), list, left);
		MBbinarytree_load_recursive(&((*node)->right), &list[middle + 1], right);
	}
}

void MBbinarytree_load(MBbinarytree * tree, void ** list, size_t size)
{
	MBbinarytree_load_recursive(&(tree->root), list, size);
	tree->count = size;
}

void *MBbinarytree_get_at(const MBbinarytree *tree, unsigned int index)
{
	void *result = NULL;
	if (index < tree->count) {
		MBbtnode *node = tree->root;
		unsigned int pos = MBbinarytree_count_recursive(tree->root->left);
		while (pos != index) {
			if (index > pos) {
				node = node->right;
				pos = pos + 1 + MBbinarytree_count_recursive(node->left);
			}
			else {
				node = node->left;
				pos = pos - 1 - MBbinarytree_count_recursive(node->right);
			}
		}
		result = node->data;
	}
	return result;
}

int MBbinarytree_find_index(const MBbinarytree *tree, const void * data)
{
	int result = -1;
	find_info info = MBbinarytree_find_info(tree, data);
	if (info.node) {
		result = info.index;
	}
	return result;
}

typedef struct {
	MBstack *stack;
	MBbtnode *current;
} binarytree_iterator;

static binarytree_iterator *binarytree_iterator_create(const MBbinarytree *tree)
{
	binarytree_iterator *bi = malloc(sizeof(binarytree_iterator));
	if (bi) {
		bi->stack = MBstack_create();
		bi->current = tree->root;
	}
	return bi;
}

static void binarytree_iterator_delete(binarytree_iterator *bi)
{
	if (bi) {
		MBstack_delete(bi->stack);
		free(bi);	
	}
}

static void *binarytree_iterator_get(binarytree_iterator *bi)
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

MBiterator *MBbinarytree_iterator(const MBbinarytree *tree)
{
	MBiterator *it = MBiterator_create(binarytree_iterator_create(tree),
		(MBgetfn)binarytree_iterator_get, (MBdeletefn)binarytree_iterator_delete);

	return it;
}

