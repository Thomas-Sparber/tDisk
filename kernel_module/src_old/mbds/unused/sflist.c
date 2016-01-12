/*
 *  MBsflist - a singly-linked list
 *
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#include <stdlib.h>

#include <sflist.h>

static MBsfnode * MBsfnode_create(void * data)
{
	MBsfnode * node = malloc(sizeof(MBsfnode));
	if (node) {
		node->data = data;
		node->next = NULL;
	}
	return node;
}

MBsflist * MBsflist_create(void)
{
	MBsflist * list = malloc(sizeof(MBsflist));
	if (list) {
		list->start = NULL;
		list->head = NULL;
		list->count = 0;
	}

	return list;
}

void MBsflist_empty(MBsflist * list)
{
	list->head = NULL;
}

void MBsflist_delete(MBsflist * list)
{
	if (list) {
		unsigned int count = 0;
		MBsfnode *node, *temp;
		node = list->start;
		while (node != NULL) {
			count++;
			temp = node->next;
			free(node);
			node = temp;
		}
		free(list);
	}
}

void MBsflist_add_tail(MBsflist * list, void * data)
{
	if (list->head == NULL) {
		MBsflist_add_head(list, data);
	}
	else {
		MBsfnode * node = MBsfnode_create(data);
		MBsfnode *current = list->head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = node;
		list->count++;
	}
}

void MBsflist_add_head(MBsflist * list, void * data)
{
	MBsfnode * node;
	if (list->start != list->head) {
		node = list->start;
		while (node->next != list->head) {
			node = node->next;
		}
		node->data = data;
	}
	else {
		node = MBsfnode_create(data);
		list->start = node;
	}
	node->next = list->head;
	list->head = node;
	list->count++;
}

void * MBsflist_remove_head(MBsflist * list)
{
	void *data = NULL;

	if (list->head) {
		data = list->head->data;
		list->head = list->head->next;

		list->count--;
	}
	
	return data;
}

void * MBsflist_remove_tail(MBsflist * list)
{
	void *data = NULL;

	if (list->count == 1) {
		data = MBsflist_remove_head(list);
	}
	else if (list->count) {
		MBsfnode *current, *previous = NULL;
		current = list->head;
		while (current->next) {
			previous = current;
			current = current->next;
		}
		data = current->data;
		free(current);
		previous->next = NULL;
		list->count--;
	}

	return data;
}

void * MBsflist_remove(MBsflist *list, MBsfnode *node, MBsfnode *previous)
{
	void *data;
	if (node == list->head) {
		/* Or equivalently, previous == NULL */
		data = MBsflist_remove_head(list);
	}
	else {
		previous->next = node->next;
		data = node->data;
		free(node);
		list->count--;
	}
	return data;
}

void MBsflist_insert_before(MBsflist * list, MBsfnode * node, MBsfnode * previous, void * data)
{
	if (node == list->head) {
		MBsflist_add_head(list, data);
	}
	else {
		MBsfnode * newnode = MBsfnode_create(data);
		newnode->next = node;
		previous->next = newnode;
	}
}

MBsfnode * MBsflist_insert_after(MBsflist * list, MBsfnode * node, void * data)
{
	MBsfnode * newnode;
	if (node == NULL) {
		MBsflist_add_head(list, data);
		newnode = list->head;
	}
	else {
		newnode = MBsfnode_create(data);
		if (newnode) {
			newnode->next = node->next;
			node->next = newnode;
		}
	}
	return newnode;
}

void MBsflist_for_each(const MBsflist * list, MBforfn forfn)
{
	MBsfnode * node;
	for (node = list->head; node != NULL; node = node->next) {
		forfn(node->data);
	}
}

unsigned int MBsflist_get_count(const MBsflist * list)
{
	return list->count;
}

typedef struct {
	MBsfnode * node;
} sflist_iterator;

sflist_iterator * sflist_iterator_create(MBsfnode * head)
{
	sflist_iterator * it = malloc(sizeof(sflist_iterator));
	if (it) {
		it->node = head;
	}
	return it;
}

void sflist_iterator_delete(sflist_iterator * it)
{
	free(it);
}

void * sflist_iterator_get(sflist_iterator * it)
{
	void * data = NULL;
	if (it->node != NULL) {
		data = it->node->data;
		it->node = it->node->next;
	}
	return data;
}

MBiterator * MBsflist_iterator(const MBsflist * list)
{
	return MBiterator_create(sflist_iterator_create(list->head), (MBgetfn)sflist_iterator_get, (MBdeletefn)sflist_iterator_delete);
}
