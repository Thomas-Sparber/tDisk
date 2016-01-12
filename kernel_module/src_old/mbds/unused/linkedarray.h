/*
 *  linkedarray.h - a doubly-linked list embedded in an arraya with compaction
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef LINKEDARRAY_H
#define LINKEDARRAY_H

#include <limits.h>

#include <mbcommon.h>
#include <iterator.h>

#define LINKEDARRAY_NIL UINT_MAX

struct MBlanode {
	unsigned int next;
	unsigned int previous;
	void * data;
};
typedef struct MBlanode MBlanode;

struct MBlinkedarray {
	MBlanode* buf;
	unsigned int len;
	unsigned int count;
	unsigned int head;
	unsigned int tail;
};
typedef struct MBlinkedarray MBlinkedarray;

MBlinkedarray * MBlinkedarray_create(void);
void MBlinkedarray_delete(MBlinkedarray * list);
void MBlinkedarray_add_head(MBlinkedarray * list, void * data);
void MBlinkedarray_add_tail(MBlinkedarray * list, void * data);
void MBlinkedarray_insert_before(MBlinkedarray *list, unsigned int node, void * data);
void MBlinkedarray_insert_after(MBlinkedarray *list, unsigned int node, void * data);
void * MBlinkedarray_remove_head(MBlinkedarray * list);
void * MBlinkedarray_remove_tail(MBlinkedarray * list);
void * MBlinkedarray_remove_node(MBlinkedarray * list, unsigned int node);
void MBlinkedarray_for_each(const MBlinkedarray * list, MBforfn forfn);
void MBlinkedarray_for_each_unordered(const MBlinkedarray * list, MBforfn forfn);
unsigned int MBlinkedarray_get_count(const MBlinkedarray * list);
unsigned int MBlinkedarray_get_head(const MBlinkedarray * list);
unsigned int MBlinkedarray_get_tail(const MBlinkedarray * list);
void * MBlinkedarray_get_data(const MBlinkedarray * list, unsigned int node);
unsigned int MBlinkedarray_get_next(const MBlinkedarray *list, unsigned int node);
unsigned int MBlinkedarray_get_previous(const MBlinkedarray *list, unsigned int node);
unsigned int MBlinkedarray_get_count(const MBlinkedarray * list);
MBiterator * MBlinkedarray_iterator(const MBlinkedarray *list);

#endif /* linkedarray_H */
