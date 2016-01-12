/*
 *  slist.c - a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <linux/vmalloc.h>

#include "slist.h"

static MBsnode * MBsnode_create(void * data)
{
	MBsnode * node = vmalloc(sizeof(MBsnode));
	if (node) {
		node->data = data;
		node->next = NULL;
	}

	return node;
}

MBslist * MBslist_create(void)
{
	MBslist * list = vmalloc(sizeof(MBslist));
	if (list) {
		list->head = NULL;
		list->tail = NULL;
		list->count = 0;
	}

	return list;
}

void MBslist_empty(MBslist * list)
{
	MBsnode * node, * temp;
	node = list->head;
	while (node != NULL) {
		temp = node->next;
		vfree(node);
		node = temp;
	}
}

void MBslist_delete(MBslist * list)
{
	if (list) {
		MBslist_empty(list);
		vfree(list);
	}
}

void MBslist_add_tail(MBslist * list, void * data)
{
	MBsnode * node = MBsnode_create(data);
	if (list->head == NULL) {
		/* Adding the first node */
		list->head = node;
		list->tail = node;
	}
	else {
		list->tail->next = node;
		list->tail = node;
	}
	list->count++;
}

void MBslist_add_head(MBslist * list, void * data)
{
	MBsnode * node = MBsnode_create(data);
	if (list->tail == NULL) {
		/* Adding the first node */
		list->head = node;
		list->tail = node;
	}
	else {
		node->next = list->head;
		list->head = node;
	}
	list->count++;
}

void * MBslist_remove_head(MBslist * list)
{
	void *data = NULL;

	if (list->head) {
		MBsnode *temp = list->head;
		list->head = list->head->next;
		if (list->head == NULL) {
			/* List is now empty */
			list->tail = NULL;
		}
		data = temp->data;
		vfree(temp);
		list->count--;
		if (list->count == 1) {
			list->tail = list->head;
		}
	}
	
	return data;
}

void * MBslist_remove_tail(MBslist * list)
{
	void *data = NULL;

	if (list->tail) {
		MBsnode *current, *previous = NULL;
		current = list->head;
		while (current->next) {
			previous = current;
			current = current->next;
		}
		data = current->data;
		vfree(current);
		if (previous) {
			previous->next = NULL;
			list->tail = previous;
		}
		else {
			/* List is now empty */
			list->head = NULL;
			list->tail = NULL;
		}
		list->count--;
		if (list->count == 1) {
			list->head = list->tail;
		}
	}

	return data;
}

void * MBslist_remove(MBslist *list, MBsnode *node, MBsnode *previous)
{
	void *data;
	if (node == list->head) {
		data = MBslist_remove_head(list);
	}
	else {
		previous->next = node->next;
		data = node->data;
		list->count--;
		if (list->count == 1) {
			list->tail = list->head;
		}
		else if (node == list->tail) {
			list->tail = previous;
		}
		vfree(node);
	}
	return data;
}

void MBslist_insert_before(MBslist * list, MBsnode * node, MBsnode * previous, void * data)
{
	if (node == list->head) {
		MBslist_add_head(list, data);
	}
	else {
		MBsnode * newnode = MBsnode_create(data);
		newnode->next = node;
		previous->next = newnode;
		list->count++;
	}
}

MBsnode * MBslist_insert_after(MBslist * list, MBsnode * node, void * data)
{
	MBsnode * newnode;
	if (node == NULL) {
		MBslist_add_head(list, data);
		newnode = list->head;
	}
	else {
		newnode = MBsnode_create(data);
		if (newnode) {
			newnode->next = node->next;
			node->next = newnode;
			if (node == list->tail) {
				list->tail = newnode;
			}
		}
		list->count++;
	}
	return newnode;
}

void MBslist_for_each(const MBslist * list, MBforfn forfn)
{
	MBsnode * node;
	for (node = list->head; node != NULL; node = node->next) {
		forfn(node->data);
	}
}

unsigned int MBslist_get_count(const MBslist * list)
{
	return list->count;
}

typedef struct {
	MBsnode * node;
} slist_iterator;

slist_iterator * slist_iterator_create(MBsnode * head)
{
	slist_iterator * it = vmalloc(sizeof(slist_iterator));
	if (it) {
		it->node = head;
	}
	return it;
}

void slist_iterator_delete(slist_iterator * it)
{
	vfree(it);
}

void * slist_iterator_get(slist_iterator * it)
{
	void * data = NULL;
	if (it->node != NULL) {
		data = it->node->data;
		it->node = it->node->next;
	}
	return data;
}

MBiterator * MBslist_iterator(const MBslist * list)
{
	return MBiterator_create(slist_iterator_create(list->head), (MBgetfn)slist_iterator_get, (MBdeletefn)slist_iterator_delete);
}
