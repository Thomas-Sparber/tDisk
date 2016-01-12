/*
 *  linkedlist.h - a doubly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <mbcommon.h>
#include <iterator.h>

struct MBlistnode {
	struct MBlistnode * next;
	struct MBlistnode * previous;
	void * data;
};
typedef struct MBlistnode MBlistnode;

struct MBlinkedlist {
	MBlistnode * head;
	MBlistnode * tail;
	unsigned int count;
};
typedef struct MBlinkedlist MBlinkedlist;

MBlinkedlist * MBlinkedlist_create(void);
void MBlinkedlist_empty(MBlinkedlist * list);
void MBlinkedlist_delete(MBlinkedlist * list);
void MBlinkedlist_add_head(MBlinkedlist * list, void * data);
void MBlinkedlist_add_tail(MBlinkedlist * list, void * data);
void MBlinkedlist_insert_before(MBlinkedlist * list, MBlistnode * node, void * data);
void MBlinkedlist_insert_after(MBlinkedlist * list, MBlistnode * node, void * data);
void * MBlinkedlist_remove_head(MBlinkedlist * list);
void * MBlinkedlist_remove_tail(MBlinkedlist * list);
void * MBlinkedlist_remove_node(MBlinkedlist * list, MBlistnode * node);
void MBlinkedlist_for_each(const MBlinkedlist * list, MBforfn forfn);
unsigned int MBlinkedlist_get_count(const MBlinkedlist * list);
MBiterator * MBlinkedlist_iterator(const MBlinkedlist *list);

#endif /* LINKEDLIST_H */
