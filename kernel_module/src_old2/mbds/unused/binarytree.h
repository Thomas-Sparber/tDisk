/*
 *  binarytree.h - an unbalanced binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef BINARYTREE_H
#define BINARYTREE_H

#include <stddef.h> /* For size_t */

#include <mbcommon.h>
#include <iterator.h>

struct MBbtnode {
	struct MBbtnode * left;
	struct MBbtnode * right;
	void * data;
};

typedef struct MBbtnode MBbtnode;

struct MBbinarytree {
	MBbtnode * root;
	MBcmpfn cmpfn;
	unsigned int count;
};

typedef struct MBbinarytree MBbinarytree;

MBbinarytree * MBbinarytree_create(MBcmpfn cmpfn);
void MBbinarytree_delete(MBbinarytree * tree);
void MBbinarytree_for_each(const MBbinarytree * tree, MBforfn forfn);
void* MBbinarytree_add(MBbinarytree * tree, void * data);
void* MBbinarytree_find(const MBbinarytree * tree, const void * data);
void* MBbinarytree_remove(MBbinarytree * tree, const void * data);
unsigned int MBbinarytree_get_count(const MBbinarytree *tree);
void MBbinarytree_load(MBbinarytree * tree, void ** list, size_t size);
void *MBbinarytree_get_at(const MBbinarytree *tree, unsigned int index);
int MBbinarytree_find_index(const MBbinarytree *tree, const void * data);
MBiterator *MBbinarytree_iterator(const MBbinarytree *tree);

#endif /* BINARYTREE_H */
