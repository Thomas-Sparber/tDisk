/*
 *  linkedlist.c - a doubly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <linkedlist.h>

MBlistnode * MBlistnode_create(void * data)
{
	MBlistnode * node;
	node = malloc(sizeof (MBlistnode));
	if (node) {
		node->next = NULL;
		node->previous = NULL;
		node->data = data;
	}
	return node;
}

MBlinkedlist * MBlinkedlist_create(void)
{
	MBlinkedlist * list;
	list = malloc(sizeof(MBlinkedlist));
	if (list) {
		list->head = NULL;
		list->tail = NULL;
		list->count = 0;
	}
	return list;
}

void MBlinkedlist_empty(MBlinkedlist * list)
{
	while(list->head != NULL)
		MBlinkedlist_remove_tail(list);
}

void MBlinkedlist_delete(MBlinkedlist * list)
{
	if (list) {
		MBlinkedlist_empty(list);
		free(list);
	}
}

void MBlinkedlist_add_head(MBlinkedlist * list, void * data)
{
	MBlistnode * node;
	node = MBlistnode_create(data);
	if (list->head != NULL) {
		list->head->previous = node;
		node->next = list->head;
		list->head = node;
	}
	else {
		list->head = node;
		list->tail = node;
	}
	list->count++;
}

void MBlinkedlist_add_tail(MBlinkedlist * list, void * data)
{
	MBlistnode * node;
	node = MBlistnode_create(data);
	if (list->tail != NULL) {
		list->tail->next = node;
		node->previous = list->tail;
		list->tail = node;
	}
	else {
		list->head = node;
		list->tail = node;
	}
	list->count++;
}

void MBlinkedlist_insert_before(MBlinkedlist * list, MBlistnode * node, void * data)
{
	if (node == list->head) {
		MBlinkedlist_add_head(list, data);
	}
	else {
		MBlinkedlist_insert_after(list, node->previous, data);
	}
}

void MBlinkedlist_insert_after(MBlinkedlist * list, MBlistnode * node, void * data)
{
	MBlistnode * newnode;
	if (node == list->tail) {
		MBlinkedlist_add_tail(list, data);
	}
	else {
		newnode = MBlistnode_create(data);
		newnode->previous = node;
		newnode->next = node->next;
		node->next->previous = newnode;
		node->next = newnode;
		list->count++;
	}
}

void * MBlinkedlist_remove_head(MBlinkedlist * list)
{
	void * data = NULL;
	if (list->head != NULL) {
		MBlistnode * temp = list->head;
		list->head = list->head->next;
		if (list->head == NULL) {
			list->tail = NULL;
		}
		else {
			list->head->previous = NULL;
			if (list->head->next != NULL) {
				list->head->next->previous = list->head;
			}
			else {
				list->tail = list->head;
			}
		}
		data = temp->data;
		free(temp);
		list->count--;
	}
	return data;
}

void * MBlinkedlist_remove_tail(MBlinkedlist * list)
{
	void * data = NULL;
	if (list->tail != NULL) {
		MBlistnode * temp = list->tail;
		list->tail = list->tail->previous;
		if (list->tail == NULL) {
			list->head = NULL;
		}
		else {
			list->tail->next = NULL;
			if (list->tail->previous != NULL) {
				list->tail->previous->next = list->tail;
			}
			else {
				list->head = list->tail;
			}
		}
		data = temp->data;
		free(temp);
		list->count--;
	}
	
	return data;
}

void * MBlinkedlist_remove_node(MBlinkedlist * list, MBlistnode * node)
{
	void * data;
	if (node == list->head) {
		data = MBlinkedlist_remove_head(list);
	}
	else if (node == list->tail) {
		data = MBlinkedlist_remove_tail(list);
	}
	else {
		node->previous->next = node->next;
		node->next->previous = node->previous;
		data = node->data;
		free(node);
		list->count--;
	}
	return data;
}

void MBlinkedlist_for_each(const MBlinkedlist * list, MBforfn forfn)
{
	MBlistnode * node;
	for (node = list->head; node != NULL; node = node->next)
		forfn(node->data);
}

unsigned int MBlinkedlist_get_count(const MBlinkedlist * list)
{
	return list->count;
}

typedef struct {
	MBlistnode *node;
} linkedlist_iterator;

static linkedlist_iterator *linkedlist_iterator_create(MBlistnode *node)
{
	linkedlist_iterator *it = malloc(sizeof(linkedlist_iterator));
	if (it) {
		it->node = node;
	}

	return it;
}

static void linkedlist_iterator_delete(linkedlist_iterator *it)
{
	free(it);
}

static void *linkedlist_iterator_get(linkedlist_iterator *it)
{
	void *data = NULL;
	if (it->node) {
		data = it->node->data;
		it->node = it->node->next;
	}

	return data;
}

MBiterator *MBlinkedlist_iterator(const MBlinkedlist *list)
{
	MBiterator *it = MBiterator_create(linkedlist_iterator_create(list->head),
		(MBgetfn)linkedlist_iterator_get, (MBdeletefn)linkedlist_iterator_delete);

	return it;
}


