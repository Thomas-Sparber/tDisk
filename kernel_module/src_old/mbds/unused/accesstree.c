/*
 *  accesstree.c - a binary tree balanced by access frequency
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <stack.h>

#include <accesstree.h>

typedef struct {
	MBatnode *parent;
 	MBatnode *node;
	unsigned int index;
	unsigned int depth;
	int direction;
	MBstack * stack;
} atfind_info;


static MBatnode * MBatnode_create(void * data)
{
	MBatnode * node = malloc(sizeof(MBatnode));
	if (node) {
		node->left = NULL;
		node->right = NULL;
		node->accesses = 0;
		node->data = data;
	}
	return node;
}

static atfind_info *atfind_info_create(void)
{
	atfind_info *info = malloc(sizeof(atfind_info));
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

void atfind_info_delete(atfind_info *info)
{
	if (info) {
		MBstack_delete(info->stack);
		free(info);
	}
}

MBaccesstree * MBaccesstree_create(MBcmpfn cmpfn)
{
	MBaccesstree * tree = malloc(sizeof(MBaccesstree));
	if (tree != NULL) {
		tree->root = NULL;
		tree->cmpfn = cmpfn;
		tree->count = 0;
	}
	return tree;
}


static void MBaccesstree_clear_recursive(MBatnode * root)
{
	if (root != NULL) {
		MBatnode * left = root->left;
		MBatnode * right = root->right;
		free(root);
		MBaccesstree_clear_recursive(left);
		MBaccesstree_clear_recursive(right);
	}
}

static void MBaccesstree_clear(MBaccesstree * tree)
{
	MBaccesstree_clear_recursive(tree->root);
	tree->root = NULL;
	tree->count = 0;
}

void MBaccesstree_delete(MBaccesstree * tree)
{
	if (tree) {
		MBaccesstree_clear(tree);
		free(tree);
	}
}

static void MBaccesstree_for_each_recursive(const MBatnode * root, MBforfn forfn)
{
	if (root) {
		MBaccesstree_for_each_recursive(root->left, forfn);
		forfn(root->data);
		MBaccesstree_for_each_recursive(root->right, forfn);
	}
}

void MBaccesstree_for_each(const MBaccesstree * tree, MBforfn forfn)
{
	MBaccesstree_for_each_recursive(tree->root, forfn);
}
static unsigned int MBaccesstree_count_recursive(const MBatnode *root)
{
	unsigned int count = 0;
	if (root != NULL) {
		count = 1 + MBaccesstree_count_recursive(root->left) + MBaccesstree_count_recursive(root->right);
	}
	return count;
}

static void MBaccesstree_rotate_left(MBaccesstree *tree, MBatnode *node, MBatnode *parent)
{
	MBatnode *right = node->right;
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

static void MBaccesstree_rotate_right(MBaccesstree *tree, MBatnode *node, MBatnode *parent)
{
	MBatnode *left = node->left;
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

static void MBaccesstree_arrange(MBaccesstree *tree, MBstack *stack, MBatnode *node)
{
	unsigned int finished = 0;
	while (node != tree->root && !finished) {
		MBatnode *parent = MBstack_pop(stack);
		if (node->accesses > parent->accesses) {
			if (node == parent->left) {
				MBaccesstree_rotate_right(tree, parent, MBstack_peek(stack));
			}
			else if (node == parent->right) {
				MBaccesstree_rotate_left(tree, parent, MBstack_peek(stack));
			}
			else {
				fprintf(stdout, "node was not child of parent!\n");
			}
		}
		else {
			finished = 1;
		}
	}
}

static atfind_info * MBaccesstree_atfind_info(MBaccesstree * tree, const void * data)
{
	MBatnode * current, * previous = NULL;
	int result = 0;
	unsigned int found = 0;
	unsigned int depth = 0;
	atfind_info *info = atfind_info_create();
	unsigned int pos = tree->root ? MBaccesstree_count_recursive(tree->root->left) : 0;

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
				   pos -= MBaccesstree_count_recursive(current->right);
			   }
			}
			else {
				current = current->right;
				pos += 1;
			    if (current) {
					pos += MBaccesstree_count_recursive(current->left);
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

void* MBaccesstree_add(MBaccesstree * tree, void * data)
{
	MBatnode * node;
	void * temp = NULL;
	atfind_info * info = MBaccesstree_atfind_info(tree, data); 
	if (info->node == NULL) {
		/* Adding a new item */
		node = MBatnode_create(data);
		if (tree->root == NULL) {
			tree->root = node;
		}
		else {
			/*const int result = tree->cmpfn(data, info.parent->data);*/
			if (info->direction > 0) {
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
	atfind_info_delete(info);
	return temp;
}

void* MBaccesstree_find(MBaccesstree * tree, const void * data)
{
	void * found = NULL;

	atfind_info * info = MBaccesstree_atfind_info(tree, data);
	if (info->node) {
		found = info->node->data;
		info->node->accesses++;
		MBaccesstree_arrange(tree, info->stack, info->node);
	}
	atfind_info_delete(info);
	return found;
}

static int MBatnode_max(const MBatnode *first, const MBatnode *second)
{
	int max = 0;
	if ((first && !second) || (first && second && first->accesses >= second->accesses)) {
		max = first->accesses;
	}
	else if ((second && !first) || (first && second && second->accesses >= first->accesses)) { 
		max = second->accesses;
	}
	return max;
}

static MBatnode * MBatnode_max_child(const MBatnode *node)
{
	MBatnode * child = NULL;
	if (node->left && node->right) {
		child = node->left->accesses > node->right->accesses ? node->left : node->right;
	}
	else if (node->left || node->right) {
		child = node->left ? node->left : node->right;
	}
	return child;
}

static void MBaccesstree_dump_recursive(const MBatnode *node)
{
	if (node == NULL) return;
	printf("%d<-%d->%d", node->left != NULL ? node->left->accesses : 0, node->accesses, node->right != NULL ? node->right->accesses : 0);
	if (node->accesses < MBatnode_max(node->left, node->right)) {
		printf(" <-----Wrong");
	}
	putchar('\n');
	MBaccesstree_dump_recursive(node->left);
	MBaccesstree_dump_recursive(node->right);
}

void MBaccesstree_dump(const MBaccesstree *tree)
{
	MBaccesstree_dump_recursive(tree->root);
}

static unsigned int MBaccesstree_check_accesses_recursive(const MBatnode *root);

static void MBaccesstree_remove_node(MBaccesstree *tree, MBatnode * node, MBatnode * parent)
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
		MBatnode *current = node->left, *previous = NULL;
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
		/* Rotate current down to the correct position */
		if (current->left || current->right) {
			unsigned int finished = 0;
			MBatnode * p = parent, * next;
			while (!finished) {
				MBatnode * maxchild = MBatnode_max_child(current);
				const unsigned int max = MBatnode_max(current->left, current->right);
				if (max > current->accesses) {
					if (maxchild == current->left) {
						next = current->left;
						MBaccesstree_rotate_right(tree, current, p);
						p = next;
					}
					else  {
						next = current->right;
						MBaccesstree_rotate_left(tree, current, p);
						p = next;
					}
				}
				else {
					finished = 1;
				}
			}
		}
	}
	free(node);
}

static unsigned int MBaccesstree_check_accesses_recursive(const MBatnode *root)
{
	if (root == NULL) {
		return 1;
	}
	else {
		return MBaccesstree_check_accesses_recursive(root->left)
			&& MBaccesstree_check_accesses_recursive(root->right)
			&& root->accesses >= MBatnode_max(root->left, root->right);
	}
}

unsigned int MBaccesstree_check_accesses(const MBaccesstree *tree)
{
	return MBaccesstree_check_accesses_recursive(tree->root);
}

void* MBaccesstree_remove(MBaccesstree * tree, const void* data)
{
	void *temp = NULL;
	atfind_info * info = MBaccesstree_atfind_info(tree, data);

	if (info->node) {
		temp = info->node->data;
		MBaccesstree_remove_node(tree, info->node, info->parent);
		tree->count--;
	}
	atfind_info_delete(info);
	return temp;
}

unsigned int MBaccesstree_get_count(const MBaccesstree *tree)
{
	return tree->count;
}

static void MBaccesstree_load_recursive(MBatnode ** node, void ** list, size_t size)
{
	if (size > 0) {
		unsigned int left, right, middle = size / 2;
		*node = MBatnode_create(list[middle]);
		left = middle;
		right = size - middle - 1;
		MBaccesstree_load_recursive(&((*node)->left), list, left);
		MBaccesstree_load_recursive(&((*node)->right), &list[middle + 1], right);
	}
}

void MBaccesstree_load(MBaccesstree * tree, void ** list, size_t size)
{
	MBaccesstree_load_recursive(&(tree->root), list, size);
	tree->count = size;
}

void *MBaccesstree_get_at(MBaccesstree *tree, unsigned int index)
{
	void *result = NULL;
	MBstack *stack = MBstack_create();
	if (index < tree->count) {
		MBatnode *node = tree->root;
		unsigned int pos = MBaccesstree_count_recursive(tree->root->left);
		while (pos != index) {
			MBstack_push(stack, node);
			if (index > pos) {
				node = node->right;
				pos = pos + 1 + MBaccesstree_count_recursive(node->left);
			}
			else {
				node = node->left;
				pos = pos - 1 - MBaccesstree_count_recursive(node->right);
			}
		}
		result = node->data;
		node->accesses++;
		MBaccesstree_arrange(tree, stack, node);
		MBstack_delete(stack);
	}
	return result;
}

int MBaccesstree_find_index(MBaccesstree *tree, const void * data)
{
	int result = -1;
	atfind_info * info = MBaccesstree_atfind_info(tree, data);
	if (info->node) {
		result = info->index;
	}
	atfind_info_delete(info);
	return result;
}

typedef struct {
	MBstack *stack;
	MBatnode *current;
} accesstree_iterator;

static accesstree_iterator *accesstree_iterator_create(const MBaccesstree *tree)
{
	accesstree_iterator *bi = malloc(sizeof(accesstree_iterator));
	if (bi) {
		bi->stack = MBstack_create();
		bi->current = tree->root;
	}
	return bi;
}

static void accesstree_iterator_delete(accesstree_iterator *bi)
{
	if (bi) {
		MBstack_delete(bi->stack);
		free(bi);	
	}
}

static void *accesstree_iterator_get(accesstree_iterator *bi)
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

MBiterator *MBaccesstree_iterator(const MBaccesstree *tree)
{
	MBiterator *it = MBiterator_create(accesstree_iterator_create(tree),
		(MBgetfn)accesstree_iterator_get, (MBdeletefn)accesstree_iterator_delete);

	return it;
}

