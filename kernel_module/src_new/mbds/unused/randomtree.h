/*
 *  randomtree.h - a randomised binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef RANDOMTREE_H
#define RANDOMTREE_H

#include <stddef.h> /* For size_t */

#include <mbcommon.h>
#include <iterator.h>

struct MBrtnode {
	struct MBrtnode * left;
	struct MBrtnode * right;
	int key;
	void * data;
};

typedef struct MBrtnode MBrtnode;

struct MBrandomtree {
	MBrtnode * root;
	MBcmpfn cmpfn;
	unsigned int count;
};

typedef struct MBrandomtree MBrandomtree;

MBrandomtree * MBrandomtree_create(MBcmpfn cmpfn);
void MBrandomtree_delete(MBrandomtree * tree);
void MBrandomtree_for_each(const MBrandomtree * tree, MBforfn forfn);
void* MBrandomtree_add(MBrandomtree * tree, void * data);
void* MBrandomtree_find(MBrandomtree * tree, const void * data);
void* MBrandomtree_remove(MBrandomtree * tree, const void * data);
unsigned int MBrandomtree_get_count(const MBrandomtree *tree);
void MBrandomtree_load(MBrandomtree * tree, void ** list, size_t size);
void *MBrandomtree_get_at(MBrandomtree *tree, unsigned int index);
int MBrandomtree_find_index(MBrandomtree *tree, const void * data);
unsigned int MBrandomtree_check_keys(const MBrandomtree *tree);
MBiterator *MBrandomtree_iterator(const MBrandomtree *tree);

#endif /* RANDOMTREE_H */
