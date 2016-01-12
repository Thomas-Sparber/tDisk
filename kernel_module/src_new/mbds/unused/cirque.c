/*
 *  cirque.c - a circular queue
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <cirque.h>

MBcirque * MBcirque_create(unsigned int size)
{
	MBcirque * cirque = malloc(sizeof(MBcirque));
	if (cirque) {
		cirque->entries = malloc(size * sizeof(void *));
		if (cirque->entries) {
			cirque->size = size;
			cirque->head = 0;
			cirque->tail = 0;
			cirque->is_full = 0;
		}
		else {
			free(cirque);
			cirque = NULL;
		}
	}
	return cirque;
}

void MBcirque_delete(MBcirque * cirque)
{
	if (cirque) {
		free(cirque->entries);
		free(cirque);
	}
}

static void MBcirque_resize(MBcirque * cirque)
{
	void **temp = malloc(cirque->size * 2 * sizeof(void *));
	if (temp) {
		unsigned int i = 0;
		unsigned int h = cirque->head;
		do {
			temp[i] = cirque->entries[h++];
			if (h == cirque->size) {
				h = 0;
			}
			i++;
		} while (h != cirque->tail);
		free(cirque->entries);
		cirque->entries = temp;
		cirque->head = 0;
		cirque->tail = cirque->size;
		cirque->size *= 2;
		cirque->is_full = 0;
	}
}

static unsigned int MBcirque_is_empty(const MBcirque * cirque)
{
	return (cirque->head == cirque->tail) && !cirque->is_full;
}

unsigned int MBcirque_insert(MBcirque * cirque, void * data)
{
	unsigned int result;
	if (cirque->is_full) {
		MBcirque_resize(cirque);
		if (cirque->is_full) {
			result = 0;
		}
	}
	if (!cirque->is_full) {
		cirque->entries[cirque->tail++] = data;
		if (cirque->tail == cirque->size) {
			cirque->tail = 0;
		}
		if (cirque->tail == cirque->head) {
			cirque->is_full = 1;
		}
		result = 1;
	}
	return result;
}

void * MBcirque_remove(MBcirque * cirque)
{
	void * data = NULL;
	if (!MBcirque_is_empty(cirque)) {
		if (cirque->is_full) {
			cirque->is_full = 0;
		} 
		data = cirque->entries[cirque->head++];
		if (cirque->head == cirque->size) {
			cirque->head = 0;
		}
	}
	return data;
}
  
void *MBcirque_peek(const MBcirque * cirque)
{
	void *data = NULL;
	if (!MBcirque_is_empty(cirque)) {
		data = cirque->entries[cirque->head];
	}
	return data;
}

unsigned int MBcirque_get_count(const MBcirque * cirque)
{
	unsigned int count;
   	if (MBcirque_is_empty(cirque)) {
		count = 0;
	}
	else if (cirque->is_full) {
		count = cirque->size;
	}
	else if (cirque->tail > cirque->head) {
		count = cirque->tail - cirque->head;
	}
	else {
		count = cirque->size - cirque->head;
		if (cirque->tail > 0) {
			count += cirque->tail - 1;
		}
	}
	return count;
}

void MBcirque_for_each(const MBcirque * cirque, MBforfn forfn)
{
	if (!MBcirque_is_empty(cirque)) {
		unsigned int h = cirque->head;
		do {
			forfn(cirque->entries[h++]);
			if (h == cirque->size) {
				h = 0;
			}
		} while (h != cirque->tail);
	}
}

typedef struct {
	const MBcirque * cirque;
	unsigned int pos;
	unsigned int finished;
} cirque_iterator;

cirque_iterator * cirque_iterator_create(const MBcirque * cirque)
{
	cirque_iterator * it = malloc(sizeof(cirque_iterator));
	if (it) {
		it->cirque = cirque;
		it->pos = cirque->head;
		it->finished = MBcirque_is_empty(cirque);
	}
	return it;
}

void cirque_iterator_delete(cirque_iterator * it)
{
	free(it);
}

void * cirque_iterator_get(cirque_iterator * it)
{
	void * data = NULL;
	if (!it->finished) {
		data = it->cirque->entries[it->pos];
		it->pos++;
		if (it->pos == it->cirque->size) {
			it->pos = 0;
		}
		if (it->pos == it->cirque->tail) {
			it->finished = 1;
		}
	}
	return data;
}

MBiterator * MBcirque_iterator(const MBcirque * cirque)
{
	return MBiterator_create(cirque_iterator_create(cirque), (MBgetfn)cirque_iterator_get,
		   	(MBdeletefn)cirque_iterator_delete);
}
