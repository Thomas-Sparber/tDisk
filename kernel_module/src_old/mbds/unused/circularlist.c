/*
 *  circularlist.c - a circular linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <circularlist.h>

static MBcnode * MBcnode_create(void * data)
{
	MBcnode * node = malloc(sizeof(MBcnode));
	if(node) {
		node->next = node;
		node->previous = node;
		node->data = data;
	}

	return node;
}

static void MBcnode_delete(MBcnode * node)
{
	free(node);
}

MBcircularlist * MBcircularlist_create(void)
{
	MBcircularlist * list = malloc(sizeof(MBcircularlist));
	if (list) {
		list->head = NULL;
		list->count = 0;
	}

	return list;
}


void MBcircularlist_delete(MBcircularlist * list)
{
	if (list) {
		MBcircularlist_empty(list);
		free(list);
	}
}

void MBcircularlist_add_head(MBcircularlist * list, void * data)
{
	MBcircularlist_insert_before(list, list->head, data);
	list->head = list->head->previous;
}

void MBcircularlist_add_tail(MBcircularlist * list, void * data)
{
	MBcircularlist_insert_before(list, list->head, data);
}

void * MBcircularlist_remove_head(MBcircularlist * list)
{
	return MBcircularlist_remove_node(list, list->head);
}

void * MBcircularlist_remove_tail(MBcircularlist * list)
{
	void * data = NULL;
	if (list->head) {
		data = MBcircularlist_remove_node(list, list->head->previous);
	}
	return data;
}

void * MBcircularlist_remove_node(MBcircularlist * list, MBcnode * node)
{
	void * data = NULL;
	if (list->count > 0) {
		if (node != node->next) { /* Or, list->count > 1 */
			node->next->previous = node->previous;
			node->previous->next = node->next;
			if (node == list->head)
				list->head = list->head->next;
		}
		else {
			/* Removing the last element */
			list->head = NULL;
		}
		data = node->data;
		MBcnode_delete(node);
		list->count--;
	}
	return data;
}

void MBcircularlist_empty(MBcircularlist * list)
{
	while (MBcircularlist_remove_tail(list) != NULL);
}

void MBcircularlist_insert_before(MBcircularlist * list, MBcnode * node, void * data)
{
	MBcnode * newnode = MBcnode_create(data);

	if (newnode) {
		if (list->count > 0) {
			newnode->next = node;
			newnode->previous = node->previous;
			newnode->previous->next = newnode;
			node->previous = newnode;
		}
		else {
			/* Adding the first element */
			list->head = newnode;
		}
		list->count++;
	}
}

void MBcircularlist_insert_after(MBcircularlist * list, MBcnode * node, void * data)
{
	MBcircularlist_insert_before(list, node->next, data);
}

void MBcircularlist_for_each(const MBcircularlist * list, MBforfn forfn)
{
	MBcnode * node = list->head;

	if (node != NULL) {
		do {
			forfn(node->data);
			node = node->next;
		} while (node != list->head);
	}
}

unsigned int MBcircularlist_get_count(const MBcircularlist *list)
{
	return list->count;
}

typedef struct {
	MBcnode *head;
	MBcnode *node;
	unsigned int started;
} circularlist_iterator;

static circularlist_iterator *circularlist_iterator_create(MBcnode *node)
{
	circularlist_iterator *it = malloc(sizeof(circularlist_iterator));
	if (it) {
		it->head = node;
		it->node = node;
		it->started = 0;
	}

	return it;
}

static void circularlist_iterator_delete(circularlist_iterator *it)
{
	free(it);
}

static void *circularlist_iterator_get(circularlist_iterator *it)
{
	void *data = NULL;
	if (it->node && (it->node != it->head || !it->started)) {
		data = it->node->data;
		it->node = it->node->next;
		it->started = 1;
	}

	return data;
}

MBiterator *MBcircularlist_iterator(const MBcircularlist *list)
{
	MBiterator *it = MBiterator_create(circularlist_iterator_create(list->head),
		(MBgetfn)circularlist_iterator_get, (MBdeletefn)circularlist_iterator_delete);

	return it;
}



