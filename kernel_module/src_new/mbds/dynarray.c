/*
 *  dynarray.c - a dynamic array
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <linux/vmalloc.h>

#include "iterator.h"
#include "dynarray.h"

#define START_SIZE 4 /* Initial buffer size if not specified */

MBdynarray * MBdynarray_create(unsigned int size)
{
	MBdynarray * array = vmalloc(sizeof(MBdynarray));
	if (array != NULL) {
		if (size) {
			array->buffer = vmalloc(size * sizeof(void*));
			if (array->buffer) {
				array->size = size;
			}
			else {
				vfree(array);
				array = NULL;
			}
		}
		else {
			array->buffer = NULL;
			array->size = 0;
		}
		array->count = 0;
	}
	return array;
}

void MBdynarray_empty(MBdynarray * array)
{
	array->count = 0;
}

void MBdynarray_delete(MBdynarray * array)
{
	if (array) {
		vfree(array->buffer);
		vfree(array);
	}
}

void MBdynarray_add_tail(MBdynarray * array, void * data)
{
	if (array->count == array->size) {
		/* No more space */
		if (array->buffer != NULL) {
			void *new_buffer = vmalloc(array->size * 2 * sizeof(void*));
			memcpy(new_buffer, array->buffer, array->size * sizeof(void*));
			vfree(array->buffer);
			array->buffer = new_buffer;
			//array->buffer = realloc(array->buffer, array->size * 2 * sizeof(void*));
			array->size *= 2;
		}
		else {
			array->buffer = vmalloc(START_SIZE * sizeof(void*));
			array->size = START_SIZE;
		}
	}
	if (array->buffer != NULL) {
		array->buffer[array->count] = data;
		array->count++;
	}
}

void MBdynarray_add_head(MBdynarray * array, void * data)
{
	unsigned int i;
	if (array->count == array->size) {
		/* No more space */
		if (array->buffer != NULL) {
			void **temp = vmalloc(array->size * 2 * sizeof(void*));
			if (temp) {
				/* Copy the elements one space to the right */
				for (i = 0; i < array->count; i++) {
					temp[i + 1] = array->buffer[i];
				}
				vfree(array->buffer);
				array->buffer = temp;
				array->size *= 2;
			}
		}
		else {
			array->buffer = vmalloc(START_SIZE * sizeof(void*));
			if (array->buffer) {
				array->size = START_SIZE;
			}
		}
	}
	else {
		for (i = array->count; i > 0; i--) {
			/* Move the elements one space to the right */
			array->buffer[i] = array->buffer[i - 1];
		}
	}
	if (array->buffer != NULL) {
		array->buffer[0] = data;
		array->count++;
	}
}

void * MBdynarray_remove_tail(MBdynarray * array)
{
	void * data = NULL;
	if (array->count > 0) {
		data = array->buffer[array->count - 1];
		array->count--;
	}
	return data;
}

void * MBdynarray_remove_head(MBdynarray * array)
{
	void * data = NULL;
	if (array->count > 0) {
		unsigned int i;
		data = array->buffer[0];
		for (i = 1; i < array->count; i++) {
			/* Move the elements one space to the left */
			array->buffer[i - 1] = array->buffer[i];
		}
		array->count--;
	}
	return data;
}

void MBdynarray_insert(MBdynarray *array, unsigned int pos, void *data)
{
	if (pos == 0) {
		MBdynarray_add_head(array, data);
	}
	else if (pos == array->count) {
		MBdynarray_add_tail(array, data);
	}
	else if (pos < array->count) {
		unsigned int i;
		if (array->count == array->size) {
			/* Reallocate the buffer and copy, leaving a space */
			void **temp = vmalloc(array->size * 2 * sizeof(void*));
			if (temp) {
				memcpy(temp, array->buffer, pos * sizeof(void*));
				memcpy(temp + pos + 1, array->buffer + pos, (array->count - pos) * sizeof(void*));
				vfree(array->buffer);
				array->buffer = temp;
				array->size *= 2;
			}
		}
		else {
			/* Move the elements after to the right */
			for (i = array->count - 1; i >= pos; i--) {
				array->buffer[i + 1] = array->buffer[i];
			}
		}
		array->buffer[pos] = data;
		array->count++;
	}
}

void * MBdynarray_remove(MBdynarray *array, unsigned int index)
{
	void *data;
	if (array->count < index + 1) {
		data = NULL;
	}
	else if (index == 0) {
		data = MBdynarray_remove_head(array);
	}
	else if (index == array->count - 1) {
		data = MBdynarray_remove_tail(array);
	}
	else {
		unsigned int i;
		data = array->buffer[index];
		/* Move the following elements left */
		for (i = index; i < array->count - 1; i++) {
			array->buffer[i] = array->buffer[i + 1];
		}
		array->count--;
	}
	return data;
}

void * MBdynarray_get(const MBdynarray * array, unsigned int pos)
{
	void * data = NULL;
	if (pos < array->count) {
		data = array->buffer[pos];
	}
	return data;
}

void * MBdynarray_set(MBdynarray * array, unsigned int pos, void * data)
{
	void * temp = NULL;
	if (pos == array->count) {
		MBdynarray_add_tail(array, data);
	}
	else if (pos < array->count) {
		temp = array->buffer[pos];
		array->buffer[pos] = data;
	}
	return temp;
}

void MBdynarray_set_size(MBdynarray * array, unsigned int size)
{
	void *new_buffer = vmalloc(size * sizeof(void*));
	memcpy(new_buffer, array->buffer, ((array->size > size) ? size : array->size) * sizeof(void*));
	vfree(array->buffer);
	array->buffer = new_buffer;
	//array->buffer = realloc(array->buffer, size);
	if (array->buffer) {
		array->size = size;
		if (array->size < array->count) {
			array->count = array->size;
		}
	}
	else {
		array->size = 0;
		array->count = 0;
	}
}

void MBdynarray_for_each(const MBdynarray * array, MBforfn forfn)
{
	unsigned int i;

	for (i = 0; i < array->count; i++) {
		forfn(array->buffer[i]);
	}
}

unsigned int MBdynarray_get_count(const MBdynarray * array)
{
	return array->count;
}

typedef struct {
	const MBdynarray *array;
	unsigned int current_element;
} dynarray_iterator;

static dynarray_iterator *dynarray_iterator_create(const MBdynarray *array)
{
	dynarray_iterator *it = vmalloc(sizeof(dynarray_iterator));
	if (it) {
		it->array = array;
		it->current_element = 0;
	}
	return it;
}

static void dynarray_iterator_delete(dynarray_iterator *it)
{
	vfree(it);
}

static void *dynarray_iterator_get(dynarray_iterator *it)
{
	void *data = NULL;
	if (it->current_element < it->array->count) {
		data = MBdynarray_get(it->array, it->current_element);
		it->current_element++;
	}
	return data;
}

MBiterator *MBdynarray_iterator(const MBdynarray *array)
{
	return MBiterator_create(dynarray_iterator_create(array), (MBgetfn)dynarray_iterator_get, 
		(MBdeletefn)dynarray_iterator_delete);
}
