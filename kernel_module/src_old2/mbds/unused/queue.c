/*
 *  queue.c - a queue as a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <linux/vmalloc.h>

#include "queue.h"

MBqueue * MBqueue_create(void)
{
	MBqueue *queue = vmalloc(sizeof(MBqueue));
	if (queue) {
		queue->list = MBslist_create();
	}
	return queue;
}

void MBqueue_delete(MBqueue * queue)
{
	if (queue) {
		MBslist_delete(queue->list);
		vfree(queue);
	}
}

void MBqueue_add(MBqueue * queue, void * data)
{
	MBslist_add_tail(queue->list, data);
}

void * MBqueue_remove(MBqueue * queue)
{
	return MBslist_remove_head(queue->list);
}

void * MBqueue_peek(MBqueue * queue)
{
	void * data = NULL;
	if (MBslist_get_count(queue->list)) {
		data = queue->list->head->data;
	}
	return data;
}

unsigned int MBqueue_get_count(const MBqueue * queue)
{
	return MBslist_get_count(queue->list);
}

void MBqueue_for_each(const MBqueue * queue, MBforfn forfn)
{
	MBslist_for_each(queue->list, forfn);
}

MBiterator * MBqueue_iterator(const MBqueue * queue)
{
	return MBslist_iterator(queue->list);
}
