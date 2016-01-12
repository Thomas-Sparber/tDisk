/*
 *  linkedarray.c - a doubly-linked list embedded in an array with compaction
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <linkedarray.h>

#define START_SIZE 4 /* Initial size */

MBlinkedarray * MBlinkedarray_create(void)
{
	MBlinkedarray * list;
	list = malloc(sizeof(MBlinkedarray));
	if (list != NULL) {
		list->buf = malloc(START_SIZE * sizeof(MBlanode));
		if (list->buf) {
			list->len = START_SIZE;
			list->count = 0;
			list->head = LINKEDARRAY_NIL;
			list->tail = LINKEDARRAY_NIL;
		}
		else {
			free(list);
			list = NULL;
		}
	}
	return list;
}

void MBlinkedarray_delete(MBlinkedarray * list)
{
	if (list) {
		free(list->buf);
		free (list);
	}
}

static unsigned int MBlinkedarray_newnode(MBlinkedarray *list, void *data)
{
	unsigned int node;
	if (list->count == list->len) {
		list->len *= 2;
		list->buf = realloc(list->buf, list->len * sizeof(MBlanode));
	}
	node = list->count;
	list->buf[node].data = data;
	list->buf[node].previous = LINKEDARRAY_NIL;
	list->buf[node].next = LINKEDARRAY_NIL;
	return node;
}

void MBlinkedarray_add_head(MBlinkedarray * list, void * data)
{ 
	const unsigned int node = MBlinkedarray_newnode(list, data);;
	if (list->head != LINKEDARRAY_NIL) {
		/* Not adding the first element */
		list->buf[list->head].previous = node;
		list->buf[node].next = list->head;
	}
	else {
		list->tail = node;
	}
	list->head = node;
	list->count++;
}

void MBlinkedarray_add_tail(MBlinkedarray * list, void * data)
{
	const unsigned int node = MBlinkedarray_newnode(list, data);
	if (list->tail != LINKEDARRAY_NIL) {
		/* Not adding the first element */
		list->buf[list->tail].next = node;
		list->buf[node].previous = list->tail;
	}
	else {
		list->head = node;
	}
	list->tail = node;
	list->count++;
}

void MBlinkedarray_insert_before(MBlinkedarray *list, unsigned int node, void * data)
{
	if (node == list->head) {
		MBlinkedarray_add_head(list, data);
	}
	else {
		int newnode = MBlinkedarray_newnode(list, data);
		list->buf[newnode].next = node;
		list->buf[newnode].previous = list->buf[node].previous;
		list->buf[list->buf[node].previous].next = newnode;
		list->buf[node].previous = newnode;
		list->count++;
	}
}

void MBlinkedarray_insert_after(MBlinkedarray *list, unsigned int node, void * data)
{
	if (node == list->tail) {
		MBlinkedarray_add_tail(list, data);
	}
	else {
		int newnode = MBlinkedarray_newnode(list, data);
		list->buf[newnode].previous = node;
		list->buf[newnode].next = list->buf[node].next;
		list->buf[list->buf[node].next].previous = newnode;
		list->buf[node].next = newnode;
		list->count++;
	}
}

unsigned int MBlinkedarray_get_count(const MBlinkedarray * list)
{
	return list->count;
}

unsigned int MBlinkedarray_get_head(const MBlinkedarray * list)
{
	return list->head;
}

unsigned int MBlinkedarray_get_tail(const MBlinkedarray * list)
{
	return list->tail;
}

unsigned int MBlinkedarray_get_next(const MBlinkedarray *list, unsigned int node)
{
	int next = LINKEDARRAY_NIL;
	if (node != LINKEDARRAY_NIL) {
		next = list->buf[node].next;
	}
	return next;
}

unsigned int MBlinkedarray_get_previous(const MBlinkedarray *list, unsigned int node)
{
	unsigned int previous = LINKEDARRAY_NIL;
	if (node != LINKEDARRAY_NIL) {
		previous = list->buf[node].previous;
	}
	return previous;
}

void * MBlinkedarray_get_data(const MBlinkedarray * list, unsigned int node)
{
	void * data = NULL;
	if (node != LINKEDARRAY_NIL) {
		data = list->buf[node].data;
	}
	return data;
}

void MBlinkedarray_for_each(const MBlinkedarray * list, MBforfn forfn)
{
	int node;
	for (node = list->head; node != LINKEDARRAY_NIL; node = list->buf[node].next)
		forfn(list->buf[node].data);
}

void MBlinkedarray_for_each_unordered(const MBlinkedarray * list, MBforfn forfn)
{
	unsigned int i;
	for (i = 0; i < list->count; i++) {
		forfn(list->buf[i].data);
	}
}

static void MBlinkedarray_compact(MBlinkedarray *list, unsigned int hole)
{
	if (hole != list->count - 1) {
		list->buf[hole].data = list->buf[list->count - 1].data;
		list->buf[hole].previous = list->buf[list->count - 1].previous;
		list->buf[hole].next = list->buf[list->count - 1].next;
		if (list->count - 1 == list->head) {
			list->head = hole;
		}
		else {
			list->buf[list->buf[hole].previous].next = hole;
		}
		if (list->count - 1 == list->tail) {
			list->tail = hole;
		}
		else {
			list->buf[list->buf[hole].next].previous = hole;
		}
	}
}

void * MBlinkedarray_remove_head(MBlinkedarray * list)
{
	void * data = NULL;
	unsigned int hole = list->head;
	if (list->head != LINKEDARRAY_NIL) {
		data = list->buf[list->head].data;
		list->head = list->buf[list->head].next;
		if (list->head == LINKEDARRAY_NIL) {
			list->tail = LINKEDARRAY_NIL;
		}
		else {
			list->buf[list->head].previous = LINKEDARRAY_NIL;
			if (list->buf[list->head].next != LINKEDARRAY_NIL) {
				list->buf[list->buf[list->head].next].previous = list->head;
			}
			else {
				list->tail = list->head;
			}
		}
		MBlinkedarray_compact(list, hole);
	}
	list->count--;
	return data;
}

void * MBlinkedarray_remove_tail(MBlinkedarray * list)
{
	void * data = NULL;
	unsigned int hole = list->tail;
	if (list->tail != LINKEDARRAY_NIL) {
		data = list->buf[list->tail].data;
		list->tail = list->buf[list->tail].previous;
		if (list->tail == LINKEDARRAY_NIL) {
			list->head = LINKEDARRAY_NIL;
		}
		else {
			list->buf[list->tail].next = LINKEDARRAY_NIL;
			if (list->buf[list->tail].previous != LINKEDARRAY_NIL) {
				list->buf[list->buf[list->tail].previous].next = list->tail;
			}
			else {
				list->head = list->tail;
			}
		}
		MBlinkedarray_compact(list, hole);
	}
	list->count--;
	return data;
}

void * MBlinkedarray_remove_node(MBlinkedarray * list, unsigned int node)
{
	void * data;
	if (node == list->head) {
		data = MBlinkedarray_remove_head(list);
	}
	else if (node == list->tail) {
		data = MBlinkedarray_remove_tail(list);
	}
	else {
		list->buf[list->buf[node].previous].next = list->buf[node].next;
		list->buf[list->buf[node].next].previous = list->buf[node].previous;
		data = list->buf[node].data;
		MBlinkedarray_compact(list, node);
		list->count--;
	}
	return data;
}

void MBlinkedarray_dump(const MBlinkedarray *list)
{
	unsigned int i;
	for (i = 0; i < list->count; i++) {
		printf("%d: %d<-%s->%d\n", i, list->buf[i].previous, (const char*)list->buf[i].data, list->buf[i].next);
	}
}

typedef struct {
	const MBlinkedarray * list;
	unsigned int node;
} linkedarray_iterator;

static linkedarray_iterator *linkedarray_iterator_create(const MBlinkedarray *list)
{
	linkedarray_iterator *it = malloc(sizeof(linkedarray_iterator));
	if (it) {
		it->list = list;
		it->node = list->head;
	}

	return it;
}

static void linkedarray_iterator_delete(linkedarray_iterator *it)
{
	free(it);
}

static void *linkedarray_iterator_get(linkedarray_iterator *it)
{
	void *data = NULL;
	if (it->node != LINKEDARRAY_NIL) {
		data = it->list->buf[it->node].data;
		it->node = it->list->buf[it->node].next;
	}

	return data;
}

MBiterator *MBlinkedarray_iterator(const MBlinkedarray *list)
{
	MBiterator *it = MBiterator_create(linkedarray_iterator_create(list),
		(MBgetfn)linkedarray_iterator_get, (MBdeletefn)linkedarray_iterator_delete);

	return it;
}


