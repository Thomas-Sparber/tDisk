/*
 *  queue.h - a queue as a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "mbcommon.h"
#include "slist.h"
#include "iterator.h"

typedef struct MBqueue{
	MBslist *list;
} MBqueue;

MBqueue * MBqueue_create(void);
void MBqueue_delete(MBqueue * queue);
void MBqueue_add(MBqueue * queue, void * data);
void * MBqueue_remove(MBqueue * queue);
void * MBqueue_peek(MBqueue * queue);
unsigned int MBqueue_get_count(const MBqueue * queue);
void MBqueue_for_each(const MBqueue * queue, MBforfn forfn);
MBiterator * MBqueue_iterator(const MBqueue * queue);

#endif /* QUEUE_H */
