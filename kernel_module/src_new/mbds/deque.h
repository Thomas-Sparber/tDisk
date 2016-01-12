/*
 *  deque.h - a deque as a dynamic array of fixed-size arrays
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef DEQUE_H
#define DEQUE_H

#include "dynarray.h"
#include "mbcommon.h"
#include "iterator.h"

typedef struct MBdeque {
	MBdynarray *arrays;
	unsigned int arraysize;
	unsigned int front;
	unsigned int back;
	unsigned int firstempty;
	unsigned int lastempty;
	unsigned int count;
} MBdeque;

MBdeque * MBdeque_create(void);
void MBdeque_delete(MBdeque * deque);
void MBdeque_push_front(MBdeque * deque, void * data);
void MBdeque_push_back(MBdeque * deque, void * data);
void * MBdeque_pop_front(MBdeque * deque);
void * MBdeque_pop_back(MBdeque * deque);
void * MBdeque_get_at(const MBdeque *deque, unsigned int index);
void * MBdeque_set_at(MBdeque *deque, unsigned int index, void * data);
void * MBdeque_peek_front(const MBdeque * deque);
void * MBdeque_peek_back(const MBdeque * deque);
void MBdeque_for_each(const MBdeque * deque, MBforfn forfn);
MBiterator * MBdeque_iterator(const MBdeque * deque);
unsigned int MBdeque_get_count(const MBdeque *deque);

#endif /* DEQUE_H */
