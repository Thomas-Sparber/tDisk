/*
 *  sflist.h - a singly-linked list with free nodes at the head
 *
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#ifndef MBSFLIST_H
#define MBSFLIST_H

#include <mbcommon.h>
#include <iterator.h>

struct MBsfnode {
	void * data;
	struct MBsfnode * next;
};
typedef struct MBsfnode MBsfnode;

typedef struct {
	MBsfnode * start;
	MBsfnode * head;
	unsigned int count;
} MBsflist;

MBsflist * MBsflist_create(void);
void MBsflist_empty(MBsflist * list);
void MBsflist_delete(MBsflist * list);
void MBsflist_add_tail(MBsflist * list, void * data);
void MBsflist_add_head(MBsflist * list, void * data);
void * MBsflist_remove_head(MBsflist * list);
void * MBsflist_remove_tail(MBsflist * list);
void * MBsflist_remove(MBsflist *list, MBsfnode *node, MBsfnode *previous);
void MBsflist_insert_before(MBsflist * list, MBsfnode * node, MBsfnode * previous, void * data);
MBsfnode * MBsflist_insert_after(MBsflist * list, MBsfnode * node, void * data);
void MBsflist_for_each(const MBsflist * list, MBforfn forfn);
unsigned int MBsflist_get_count(const MBsflist * list);
MBiterator * MBsflist_iterator(const MBsflist * list);

#endif /* MBSFLIST_H */
