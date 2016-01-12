/*
 *  deque.c - a deque as a dynamic array of fixed-size arrays
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <linux/vmalloc.h>

#include "deque.h"

MBdeque * MBdeque_create(void)
{
	MBdeque *deque = vmalloc(sizeof(MBdeque));
	if (deque) {
		deque->arrays = MBdynarray_create(0);
		deque->arraysize = 4;
		deque->firstempty = 1;
		deque->lastempty = 1;
		deque->count = 0;
		MBdynarray_add_head(deque->arrays, vmalloc(deque->arraysize * sizeof(void*)));
	}
	return deque;
}

void MBdelete_element(void *element)
{
	//printk(KERN_INFO "tDisk: Deleting %p\n", element);
	vfree(element);
}

void MBdeque_delete(MBdeque * deque)
{
	if (deque) {
		MBdynarray_for_each(deque->arrays, &MBdelete_element);
		MBdynarray_delete(deque->arrays);
		vfree(deque);
	}
}

void MBdeque_push_front(MBdeque * deque, void * data)
{
	unsigned int index;
	if (deque->count == 0) {
		/* Adding the first element */
		index = deque->arraysize / 2;
	}
	else if (deque->firstempty) {
		/* There is an empty array at the beginning */
		index = deque->arraysize - 1;
	}
	else if (deque->front == 0) {
		/* Beginning array is full - add another */
		MBdynarray_add_head(deque->arrays, vmalloc(deque->arraysize * sizeof(void*)));
		index = deque->arraysize - 1;
	}
	else {
		index = deque->front - 1;
	}
	((void**)MBdynarray_get(deque->arrays, 0))[index] = data;
	deque->front = index;
	deque->firstempty = 0;
	if (deque->arrays->count == 1) {
		deque->lastempty = 0;
	}
	deque->count++;
	if (deque->count == 1) {
		deque->back = deque->front;
	}
}

void MBdeque_push_back(MBdeque * deque, void * data)
{
	unsigned int index;
	if (deque->count == 0) {
		/* Adding the first element */
		index = deque->arraysize / 2;
	}
	else if (deque->lastempty) {
		/* There is an empty array at the end */
		index = 0;
	}
	else if (deque->back == deque->arraysize - 1) {
		/* End array is full - add another */
		MBdynarray_add_tail(deque->arrays, vmalloc(deque->arraysize * sizeof(void*)));
		index = 0;
	}
	else {
		index = deque->back + 1;
	}
	((void**)MBdynarray_get(deque->arrays, deque->arrays->count - 1))[index] = data;
	deque->back = index;
	deque->lastempty = 0;
	if (deque->arrays->count == 1) {
		deque->firstempty = 0;
	}
	deque->count++;
	if (deque->count == 1) {
		deque->front = deque->back;
	}
}

void * MBdeque_pop_front(MBdeque * deque)
{
	void *data = NULL;
	if (deque->count) {
		if (deque->firstempty) {
			/* There is an empty array at the beginning */
			vfree(MBdynarray_remove_head(deque->arrays));
			deque->firstempty = 0;
		}
		data = ((void**)MBdynarray_get(deque->arrays, 0))[deque->front];
		deque->front++;
		if (deque->front == deque->arraysize) {
			/* First array is now empty */
			deque->firstempty = 1;
			deque->front = 0;
		}
		deque->count--;
		if (deque->count == 1) {
			deque->back = deque->front;
		}
		else if (deque->count == 0 && deque->arrays->count == 2) {
			/* Both arrays are empty - remove either one */
			vfree(MBdynarray_remove_head(deque->arrays));
		}
	}
	return data;
}

void * MBdeque_pop_back(MBdeque * deque)
{
	void *data = NULL;
	if (deque->count) {
		if (deque->lastempty) {
			/* There is an empty array at the end */
			vfree(MBdynarray_remove_tail(deque->arrays));
			deque->lastempty = 0;
		}
		data = ((void**)MBdynarray_get(deque->arrays, deque->arrays->count - 1))[deque->back];
		if (deque->back == 0) {
			/* Last array is now empty */
			deque->lastempty = 1;
			deque->back = deque->arraysize - 1;
		}
		else {
			deque->back--;
		}
		deque->count--;
		if (deque->count == 1) {
			deque->front = deque->back;
		}
		else if (deque->count == 0 && deque->arrays->count == 2) {
			/* Both arrays are empty - remove either one */
			vfree(MBdynarray_remove_tail(deque->arrays));
		}
	}
	return data;
}

void * MBdeque_get_at(const MBdeque *deque, unsigned int index)
{
	void * data = NULL;
	if (index < deque->count) {
		const unsigned int pos = index + deque->front;
		data = ((void**)MBdynarray_get(deque->arrays, 
			pos / deque->arraysize + deque->firstempty))[pos % deque->arraysize];
	}
	return data;
}

void * MBdeque_set_at(MBdeque *deque, unsigned int index, void * data)
{
	void * result = NULL;
	if (index == deque->count) {
		MBdeque_push_back(deque, data);
	}
	else if (index < deque->count) {
		const unsigned int pos = index + deque->front;
		result = MBdeque_get_at(deque, index);
		((void**)MBdynarray_get(deque->arrays, 
			pos / deque->arraysize + deque->firstempty))[pos % deque->arraysize] = data;
	}
	return result;
}

void * MBdeque_peek_front(const MBdeque * deque)
{
	void *data = NULL;
	if (deque->count > 0) {
		data = ((void**)MBdynarray_get(deque->arrays, deque->firstempty))[deque->front];
	}
	return data;
}

void * MBdeque_peek_back(const MBdeque * deque)
{
	void *data = NULL;
	if (deque->count > 0) {
		data = ((void**)MBdynarray_get(deque->arrays, 
					MBdynarray_get_count(deque->arrays) - 1 - deque->lastempty))[deque->back];
	}
	return data;
}

void MBdeque_for_each(const MBdeque * deque, MBforfn forfn)
{
	unsigned int i;
	for (i = 0; i < deque->count; i++) {
		forfn(MBdeque_get_at(deque, i));
	}
}

unsigned int MBdeque_get_count(const MBdeque *deque)
{
	return deque->count;
}

typedef struct {
	const MBdeque * deque;
	unsigned int pos;
} deque_iterator;

deque_iterator * deque_iterator_create(const MBdeque * deque)
{
	deque_iterator * it = vmalloc(sizeof(deque_iterator));
	if (it) {
		it->deque = deque;
		it->pos = 0;
	}
	return it;
}

void deque_iterator_delete(deque_iterator * it)
{
	vfree(it);
}

void * deque_iterator_get(deque_iterator * it)
{
	void * data = NULL;
	if (it->pos < it->deque->count) {
		data = MBdeque_get_at(it->deque, it->pos);
		it->pos++;
	}
	return data;
}

MBiterator * MBdeque_iterator(const MBdeque * deque)
{
	return MBiterator_create(deque_iterator_create(deque), (MBgetfn)deque_iterator_get, (MBdeletefn)deque_iterator_delete);
}
