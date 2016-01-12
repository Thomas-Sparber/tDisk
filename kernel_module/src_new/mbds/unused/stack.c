/*
 *  stack.c - a stack as a singly-linked list
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <stack.h>

MBstack *MBstack_create(void)
{
	MBstack *stack = malloc(sizeof(MBstack));
	if (stack) {
		stack->list = MBslist_create();
	}
	return stack;
}

void MBstack_delete(MBstack *stack)
{
	if (stack) {
		MBslist_delete(stack->list);
		free(stack);
	}
}

void MBstack_push(const MBstack *stack, void *data)
{
	MBslist_add_head(stack->list, data);
}

void *MBstack_pop(const MBstack *stack)
{
	return MBslist_remove_head(stack->list);
}

void *MBstack_peek(const MBstack *stack)
{
	void *data = NULL;
	if (MBslist_get_count(stack->list)) {
		data = stack->list->head->data;
	}
	return data;
}

unsigned int MBstack_get_count(const MBstack *stack)
{
	return MBslist_get_count(stack->list);
}

void MBstack_for_each(const MBstack *stack, MBforfn forfn)
{
	MBslist_for_each(stack->list, forfn);
}

MBiterator *MBstack_iterator(const MBstack * stack)
{
	return MBslist_iterator(stack->list);
}
