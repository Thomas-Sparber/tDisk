/*
 *  toptree.h - a move-to-top binary tree
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef TOPTREE_H
#define TOPTREE_H

#include <stddef.h> /* For size_t */

#include <mbcommon.h>
#include <iterator.h>

struct MBttnode {
	struct MBttnode * left;
	struct MBttnode * right;
	void * data;
};

typedef struct MBttnode MBttnode;

struct MBtoptree {
	MBttnode * root;
	MBcmpfn cmpfn;
	unsigned int count;
};

typedef struct MBtoptree MBtoptree;

MBtoptree * MBtoptree_create(MBcmpfn cmpfn);
void MBtoptree_delete(MBtoptree * tree);
void MBtoptree_for_each(const MBtoptree * tree, MBforfn forfn);
void* MBtoptree_add(MBtoptree * tree, void * data);
void* MBtoptree_find(const MBtoptree * tree, const void * data);
void* MBtoptree_remove(MBtoptree * tree, const void * data);
unsigned int MBtoptree_get_count(const MBtoptree *tree);
void MBtoptree_load(MBtoptree * tree, void ** list, size_t size);
void *MBtoptree_get_at(const MBtoptree *tree, unsigned int index);
int MBtoptree_find_index(const MBtoptree *tree, const void * data);
MBiterator *MBtoptree_iterator(const MBtoptree *tree);

#endif /* TOPTREE_H */
