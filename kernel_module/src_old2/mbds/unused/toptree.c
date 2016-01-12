/*
 *  toptree.c - a move-to-top binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <stack.h>

#include <toptree.h>

typedef struct {
	MBttnode *parent;
 	MBttnode *node;
	unsigned int index;
	unsigned int depth;
	int direction;
	MBstack *stack;
} ttfind_info;

static ttfind_info *ttfind_info_create(void)
{
	ttfind_info *info = malloc(sizeof(ttfind_info));
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

void ttfind_info_delete(ttfind_info *info)
{
	if (info) {
		MBstack_delete(info->stack);
		free(info);
	}
}

static MBttnode * MBttnode_create(void * data)
{
	MBttnode * node = malloc(sizeof(MBttnode));
	if (node) {
		node->left = NULL;
		node->right = NULL;
		node->data = data;
	}
	return node;
}

MBtoptree * MBtoptree_create(MBcmpfn cmpfn)
{
	MBtoptree * tree = malloc(sizeof(MBtoptree));
	if (tree != NULL) {
		tree->root = NULL;
		tree->cmpfn = cmpfn;
		tree->count = 0;
	}
	return tree;
}


static void MBtoptree_clear_recursive(MBttnode * root)
{
	if (root != NULL) {
		MBttnode * left = root->left;
		MBttnode * right = root->right;
		free(root);
		MBtoptree_clear_recursive(left);
		MBtoptree_clear_recursive(right);
	}
}

static void MBtoptree_clear(MBtoptree * tree)
{
	MBtoptree_clear_recursive(tree->root);
	tree->root = NULL;
	tree->count = 0;
}

void MBtoptree_delete(MBtoptree * tree)
{
	if (tree) {
		MBtoptree_clear(tree);
		free(tree);
	}
}

static void MBtoptree_for_each_recursive(const MBttnode * root, MBforfn forfn)
{
	if (root) {
		MBtoptree_for_each_recursive(root->left, forfn);
		forfn(root->data);
		MBtoptree_for_each_recursive(root->right, forfn);
	}
}

void MBtoptree_for_each(const MBtoptree * tree, MBforfn forfn)
{
	MBtoptree_for_each_recursive(tree->root, forfn);
}
static unsigned int MBtoptree_count_recursive(const MBttnode *root)
{
	unsigned int count = 0;
	if (root != NULL) {
		count = 1 + MBtoptree_count_recursive(root->left) + MBtoptree_count_recursive(root->right);
	}
	return count;
}

static ttfind_info *MBtoptree_ttfind_info(const MBtoptree * tree, const void * data)
{
	MBttnode * current, * previous = NULL;
	int result = 0;
	unsigned int found = 0;
	unsigned int depth = 0;
	ttfind_info *info = ttfind_info_create();
	unsigned int pos = tree->root ? MBtoptree_count_recursive(tree->root->left) : 0;

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
				   pos -= MBtoptree_count_recursive(current->right);
			   }
			}
			else {
				current = current->right;
				pos += 1;
			    if (current) {
					pos += MBtoptree_count_recursive(current->left);
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

static void MBtoptree_rotate_left(MBtoptree *tree, MBttnode *node, MBttnode *parent)
{
	MBttnode *right = node->right;
	if (node == tree->root) {
		/* parent is NULL */
		tree->root = right;
	}
	else if (node == parent->left) {
		parent->left = right;
	}
	else {
		parent->right = right;
	}
	if (right) {
		if (right->left) {
			node->right = right->left;
		}
		else {
			node->right = NULL;
		}
		right->left = node;
	}
	else {
		node->right = NULL;
	}
}

static void MBtoptree_rotate_right(MBtoptree *tree, MBttnode *node, MBttnode *parent)
{
	MBttnode *left = node->left;
	if (node == tree->root) {
		/* parent is NULL */
		tree->root = left;
	}
	else if (node == parent->left) {
		parent->left = left;
	}
	else {
		parent->right = left;
	}
	if (left) {
		if (left->right) {
			node->left = left->right;
		}
		else {
			node->left = NULL;
		}
		left->right = node;
	}
	else {
		node->left = NULL;
	}
}

static void MBtoptree_arrange(MBtoptree *tree, MBstack *stack, MBttnode *node)
{
	while (tree->root != node) {
		MBttnode *parent = MBstack_pop(stack);
		if (node == parent->left) {
			MBtoptree_rotate_right(tree, parent, MBstack_peek(stack));
		}
		else {
			MBtoptree_rotate_left(tree, parent, MBstack_peek(stack));
		}
	}
}

void* MBtoptree_add(MBtoptree * tree, void * data)
{
	MBttnode * node;
	void * temp = NULL;
	ttfind_info * info = MBtoptree_ttfind_info(tree, data); 
	if (info->node == NULL) {
		/* Adding a new item */
		node = MBttnode_create(data);
		if (tree->root == NULL) {
			tree->root = node;
		}
		else {
			/*const int result = tree->cmpfn(data, info.parent->data);*/
			if(info->direction > 0) {
				info->parent->left = node;
			}
			else {
				info->parent->right = node;
			}
		}
		tree->count++;
	}
	else {
		/* Replacing an existing item */
		temp = info->node->data;
		info->node->data = data;
	}
	ttfind_info_delete(info);
	return temp;
}

void* MBtoptree_find(const MBtoptree * tree, const void * data)
{
	void * found = NULL;
	ttfind_info * info = MBtoptree_ttfind_info(tree, data);
	if (info->node) {
		found = info->node->data;
		MBtoptree_arrange((MBtoptree *)tree, info->stack, info->node);
	}
	ttfind_info_delete(info);
	return found;
}

static void MBtoptree_remove_node(MBtoptree *tree, MBttnode * node, MBttnode * parent)
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
		MBttnode *current = node->left, *previous = NULL;
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


void* MBtoptree_remove(MBtoptree * tree, const void* data)
{
	void *temp = NULL;
	ttfind_info * info = MBtoptree_ttfind_info(tree, data);

	if (info->node) {
		temp = info->node->data;
		MBtoptree_remove_node(tree, info->node, info->parent);
		tree->count--;
	}
	ttfind_info_delete(info);
	return temp;
}

unsigned int MBtoptree_get_count(const MBtoptree *tree)
{
	return tree->count;
}

static void MBtoptree_load_recursive(MBttnode ** node, void ** list, size_t size)
{
	if (size > 0) {
		unsigned int left, right, middle = size / 2;
		*node = MBttnode_create(list[middle]);
		left = middle;
		right = size - middle - 1;
		MBtoptree_load_recursive(&((*node)->left), list, left);
		MBtoptree_load_recursive(&((*node)->right), &list[middle + 1], right);
	}
}

void MBtoptree_load(MBtoptree * tree, void ** list, size_t size)
{
	MBtoptree_load_recursive(&(tree->root), list, size);
	tree->count = size;
}

void *MBtoptree_get_at(const MBtoptree *tree, unsigned int index)
{
	void *result = NULL;
	if (index < tree->count) {
		MBttnode *node = tree->root;
		unsigned int pos = MBtoptree_count_recursive(tree->root->left);
		while (pos != index) {
			if (index > pos) {
				node = node->right;
				pos = pos + 1 + MBtoptree_count_recursive(node->left);
			}
			else {
				node = node->left;
				pos = pos - 1 - MBtoptree_count_recursive(node->right);
			}
		}
		result = node->data;
	}
	return result;
}

int MBtoptree_find_index(const MBtoptree *tree, const void * data)
{
	int result = -1;
	ttfind_info *info = MBtoptree_ttfind_info(tree, data);
	if (info->node) {
		result = info->index;
	}
	ttfind_info_delete(info);
	return result;
}

typedef struct {
	MBstack *stack;
	MBttnode *current;
} toptree_iterator;

static toptree_iterator *toptree_iterator_create(const MBtoptree *tree)
{
	toptree_iterator *bi = malloc(sizeof(toptree_iterator));
	if (bi) {
		bi->stack = MBstack_create();
		bi->current = tree->root;
	}
	return bi;
}

static void toptree_iterator_delete(toptree_iterator *bi)
{
	if (bi) {
		MBstack_delete(bi->stack);
		free(bi);	
	}
}

static void *toptree_iterator_get(toptree_iterator *bi)
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

MBiterator *MBtoptree_iterator(const MBtoptree *tree)
{
	MBiterator *it = MBiterator_create(toptree_iterator_create(tree),
		(MBgetfn)toptree_iterator_get, (MBdeletefn)toptree_iterator_delete);

	return it;
}

