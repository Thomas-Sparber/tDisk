/*
 *  accesstree.h - a binary tree balanced by access frequency
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef ACCESSTREE_H
#define ACCESSTREE_H

#include <stddef.h> /* For size_t */

#include <mbcommon.h>
#include <iterator.h>

struct MBatnode {
	struct MBatnode * left;
	struct MBatnode * right;
	unsigned int accesses;
	void * data;
};

typedef struct MBatnode MBatnode;

struct MBaccesstree {
	MBatnode * root;
	MBcmpfn cmpfn;
	unsigned int count;
};

typedef struct MBaccesstree MBaccesstree;

MBaccesstree * MBaccesstree_create(MBcmpfn cmpfn);
void MBaccesstree_delete(MBaccesstree * tree);
void MBaccesstree_for_each(const MBaccesstree * tree, MBforfn forfn);
void* MBaccesstree_add(MBaccesstree * tree, void * data);
void* MBaccesstree_find(MBaccesstree * tree, const void * data);
void* MBaccesstree_remove(MBaccesstree * tree, const void * data);
unsigned int MBaccesstree_get_count(const MBaccesstree *tree);
void *MBaccesstree_get_at(MBaccesstree *tree, unsigned int index);
int MBaccesstree_find_index(MBaccesstree *tree, const void * data);
unsigned int MBaccesstree_check_accesses(const MBaccesstree *tree);
void MBaccesstree_dump(const MBaccesstree *tree);
MBiterator *MBaccesstree_iterator(const MBaccesstree *tree);

#endif /* ACCESSTREE_H */
