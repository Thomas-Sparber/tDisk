/*
 *  circularlist.h - a circular linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef CIRCULARLIST_H
#define CIRCULARLIST_H

#include <mbcommon.h>
#include <iterator.h>

struct MBcnode {
	struct MBcnode * next;
	struct MBcnode * previous;
	void * data;
};
typedef struct MBcnode MBcnode;

struct MBcircularlist {
	MBcnode * head;
	unsigned int count;
};
typedef struct MBcircularlist MBcircularlist;

MBcircularlist * MBcircularlist_create(void);
void MBcircularlist_delete(MBcircularlist * list);

void MBcircularlist_add_head(MBcircularlist * list, void * data);
void MBcircularlist_add_tail(MBcircularlist * list, void * data);
void * MBcircularlist_remove_head(MBcircularlist * list);
void * MBcircularlist_remove_tail(MBcircularlist * list);
void * MBcircularlist_remove_node(MBcircularlist * list, MBcnode * node);
void MBcircularlist_empty(MBcircularlist * list);

void MBcircularlist_insert_before(MBcircularlist * list, MBcnode * node, void * data);
void MBcircularlist_insert_after(MBcircularlist * list, MBcnode * node, void * data);

void MBcircularlist_for_each(const MBcircularlist * list, MBforfn forfn);
MBiterator *MBcircularlist_iterator(const MBcircularlist *list);
unsigned int MBcircularlist_get_count(const MBcircularlist * list);

#endif /* CIRCULARLIST_H */
