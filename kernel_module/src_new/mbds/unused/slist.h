/*
 *  slist.h - a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef SLIST_H
#define SLIST_H

#include <mbcommon.h>
#include <iterator.h>

struct MBsnode {
	void * data;
	struct MBsnode * next;
};
typedef struct MBsnode MBsnode;

typedef struct {
	MBsnode * head;
	MBsnode * tail;
	unsigned int count;
} MBslist;

MBslist * MBslist_create(void);
void MBslist_empty(MBslist * list);
void MBslist_delete(MBslist * list);
void MBslist_add_tail(MBslist * list, void * data);
void MBslist_add_head(MBslist * list, void * data);
void * MBslist_remove_head(MBslist * list);
void * MBslist_remove_tail(MBslist * list);
void * MBslist_remove(MBslist *list, MBsnode *node, MBsnode *previous);
void MBslist_insert_before(MBslist * list, MBsnode * node, MBsnode * previous, void * data);
MBsnode * MBslist_insert_after(MBslist * list, MBsnode * node, void * data);
void MBslist_for_each(const MBslist * list, MBforfn forfn);
unsigned int MBslist_get_count(const MBslist * list);
MBiterator * MBslist_iterator(const MBslist * list);

#endif /* SLIST_H */
