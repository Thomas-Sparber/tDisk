/* 
 *  recyclelist.c - a doubly-linked list with node recycling
 *
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#include <stdlib.h>

#include <recyclelist.h>

static MBrlnode * MBrlnode_create(void * data)
{
	MBrlnode * node;
	node = malloc(sizeof (MBrlnode));
	if (node) {
		node->next = NULL;
		node->previous = NULL;
		node->data = data;
	}
	return node;
}

MBrecyclelist * MBrecyclelist_create(void)
{
	MBrecyclelist * list;
	list = malloc(sizeof(MBrecyclelist));
	if (list != NULL) {
		list->head = NULL;
		list->tail = NULL;
		list->count = 0;
		list->bin = NULL;
	}
	return list;
}

void MBrecyclelist_empty(MBrecyclelist * list)
{
	while(list->head != NULL)
		MBrecyclelist_remove_tail(list);
}

void MBrecyclelist_delete(MBrecyclelist * list)
{
	if (list) {
		MBrecyclelist_empty(list);
		while (list->bin != NULL) {
			MBrlnode* temp = list->bin;
			list->bin = list->bin->next;
			free(temp);
		}
		free (list);
	}
}

static MBrlnode* MBrecyclelist_new_node(MBrecyclelist * list, void * data)
{
	MBrlnode * node;
	if (list->bin == NULL) {
		/* Recycle bin is empty */
		node = MBrlnode_create(data);
	}
	else {
		/* Retrieve the first node in the bin */
		node = list->bin;
		list->bin = list->bin->next; /* Which may be NULL */
		node->next = NULL;
		node->data = data;
	}
	return node;
}

static void MBrecyclelist_recycle_node(MBrecyclelist * list, MBrlnode * node)
{
	node->next = list->bin;
	list->bin = node;
	node->previous = NULL;
	node->data = NULL;
}

void MBrecyclelist_add_head(MBrecyclelist * list, void * data)
{
	MBrlnode * node = MBrecyclelist_new_node(list, data);
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

void MBrecyclelist_add_tail(MBrecyclelist * list, void * data)
{
	MBrlnode * node = MBrecyclelist_new_node(list, data);
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

void MBrecyclelist_insert_before(MBrecyclelist * list, MBrlnode * node, void * data)
{
	if (node == list->head) {
		MBrecyclelist_add_head(list, data);
	}
	else {
		MBrlnode * new_node = MBrecyclelist_new_node(list, data);
		new_node->next = node;
		new_node->previous = node->previous;
		node->previous->next = new_node;
		node->previous = new_node;
		list->count++;
	}
}

void MBrecyclelist_insert_after(MBrecyclelist * list, MBrlnode * node, void * data)
{
	if (node == list->tail) {
		MBrecyclelist_add_tail(list, data);
	}
	else {
		MBrlnode * new_node = MBrecyclelist_new_node(list, data);
		new_node->previous = node;
		new_node->next = node->next;
		node->next->previous = new_node;
		node->next = new_node;
		list->count++;
	}
}

void * MBrecyclelist_remove_head(MBrecyclelist * list)
{
	void * data = NULL;
	if (list->head != NULL) {
		MBrlnode * temp = list->head;
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
		MBrecyclelist_recycle_node(list, temp);
		list->count--;
	}
	return data;
}

void * MBrecyclelist_remove_tail(MBrecyclelist * list)
{
	void * data = NULL;
	if (list->tail != NULL) {
		MBrlnode * temp = list->tail;
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
		MBrecyclelist_recycle_node(list, temp);
		list->count--;
	}
	return data;
}

void * MBrecyclelist_remove_node(MBrecyclelist * list, MBrlnode * node)
{
	void * data;
	if (node == list->head) {
		data = MBrecyclelist_remove_head(list);
	}
	else if (node == list->tail) {
		data = MBrecyclelist_remove_tail(list);
	}
	else {
		node->previous->next = node->next;
		node->next->previous = node->previous;
		data = node->data;
		MBrecyclelist_recycle_node(list, node);
		list->count--;
	}
	return data;
}

void MBrecyclelist_for_each(const MBrecyclelist * list, MBforfn forfn)
{
	MBrlnode * node;
	for (node = list->head; node != NULL; node = node->next) {
		forfn(node->data);
	}
}

unsigned int MBrecyclelist_get_count(const MBrecyclelist * list)
{
	return list->count;
}

typedef struct {
	MBrlnode *node;
} recyclelist_iterator;

static recyclelist_iterator *recyclelist_iterator_create(MBrlnode *node)
{
	recyclelist_iterator *it = malloc(sizeof(recyclelist_iterator));
	if (it) {
		it->node = node;
	}

	return it;
}

static void recyclelist_iterator_delete(recyclelist_iterator *it)
{
	free(it);
}

static void *recyclelist_iterator_get(recyclelist_iterator *it)
{
	void *data = NULL;
	if (it->node) {
		data = it->node->data;
		it->node = it->node->next;
	}

	return data;
}

MBiterator *MBrecyclelist_iterator(const MBrecyclelist *list)
{
	MBiterator *it = MBiterator_create(recyclelist_iterator_create(list->head),
		(MBgetfn)recyclelist_iterator_get, (MBdeletefn)recyclelist_iterator_delete);

	return it;
}
