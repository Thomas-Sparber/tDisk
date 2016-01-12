/*
 *  stack.h - a stack as a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef STACK_H
#define STACK_H

#include <mbcommon.h>
#include <slist.h>
#include <iterator.h>

typedef struct {
	MBslist *list;
} MBstack;

MBstack *MBstack_create(void);
void MBstack_delete(MBstack *stack);
void MBstack_push(const MBstack *stack, void *data);
void *MBstack_pop(const MBstack *stack);
void *MBstack_peek(const MBstack *stack);
unsigned int MBstack_get_count(const MBstack *stack);
void MBstack_for_each(const MBstack *stack, MBforfn forfn);
MBiterator *MBstack_iterator(const MBstack * stack);

#endif /* STACK_H */

