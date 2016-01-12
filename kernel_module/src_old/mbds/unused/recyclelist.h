/* 
 *  MBrecyclelist.h - a doubly-linked list with node recycling
 *
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#ifndef RECYCLELIST_H
#define RECYCLELIST_H

#include <mbcommon.h>
#include <iterator.h>

struct MBrlnode {
	struct MBrlnode * next;
	struct MBrlnode * previous;
	void * data;
};
typedef struct MBrlnode MBrlnode;

struct MBrecyclelist {
	MBrlnode * head;
	MBrlnode * tail;
	unsigned int count;
	MBrlnode * bin; /* The recycle bin */
};
typedef struct MBrecyclelist MBrecyclelist;

MBrecyclelist * MBrecyclelist_create(void);
void MBrecyclelist_empty(MBrecyclelist * list);
void MBrecyclelist_delete(MBrecyclelist * list);
void MBrecyclelist_add_head(MBrecyclelist * list, void * data);
void MBrecyclelist_add_tail(MBrecyclelist * list, void * data);
void MBrecyclelist_insert_before(MBrecyclelist * list, MBrlnode * node, void * data);
void MBrecyclelist_insert_after(MBrecyclelist * list, MBrlnode * node, void * data);
void * MBrecyclelist_remove_head(MBrecyclelist * list);
void * MBrecyclelist_remove_tail(MBrecyclelist * list);
void * MBrecyclelist_remove_node(MBrecyclelist * list, MBrlnode * node);
void MBrecyclelist_for_each(const MBrecyclelist * list, MBforfn forfn);
unsigned int MBrecyclelist_get_count(const MBrecyclelist * list);
MBiterator * MBrecyclelist_iterator(const MBrecyclelist *list);

#endif /* RECYCLELIST_H */
