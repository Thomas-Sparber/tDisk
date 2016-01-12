/* 
 *  centarray.c - a dynamic array with centring of the used block
 * 
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#include <stdlib.h>
#include <string.h> /* For memcpy */

#include <centarray.h>

#define START_SIZE 4 /* Initial size */

static void MBcentarray_resize(MBcentarray * array, unsigned int left, unsigned int pos);

MBcentarray * MBcentarray_create(void)
{
	MBcentarray * array = malloc(sizeof(MBcentarray));
	if (array != NULL) {
		array->buffer = malloc(START_SIZE * sizeof(void*));
		array->size = START_SIZE;
		array->count = 0;
		array->incr = START_SIZE;
		array->origin = 0;
	}
	return array;
}

void MBcentarray_empty(MBcentarray * array)
{
	array->count = 0;
}

void MBcentarray_delete(MBcentarray * array)
{
	if (array) {
		free(array->buffer);
	}
	free(array);
}

void MBcentarray_add_tail(MBcentarray * array, void * data)
{
	if (array->count == array->size - array->origin) {
		MBcentarray_resize(array, 0, 0);
	}
	if (array->buffer != NULL) {
		array->buffer[array->count + array->origin] = data;
		array->count++;
	}
}

void MBcentarray_add_head(MBcentarray * array, void * data)
{
	if (array->count != 0) {
		if (array->origin == 0) {
			MBcentarray_resize(array, 1, 0);
		}
		array->origin--;
	}
	if (array->buffer != NULL) {	
		array->buffer[array->origin] = data;
		array->count++;
	}
}

void * MBcentarray_remove_tail(MBcentarray * array)
{
	void * data;

	if (array->count == 0) {
		return NULL;
	}
	data = array->buffer[array->count + array->origin - 1];
	array->count--;

	return data;
}

void * MBcentarray_remove_head(MBcentarray * array)
{
	void * data;

	if (array->count == 0) {
		return NULL;
	}
	data = array->buffer[array->origin];
	array->origin++;
	array->count--;

	return data;
}

void MBcentarray_insert(MBcentarray *array, unsigned int pos, void *data)
{
	if (pos == 0) {
		MBcentarray_add_head(array, data);
	}
	else if (pos == array->count) {
		MBcentarray_add_tail(array, data);
	}
	else if (pos < array->count) {
		unsigned int i;
		if (pos < array->count / 2) {
			/* Move the elements before to the left */
			if (array->origin == 0) {
				MBcentarray_resize(array, 1, pos);
			}
			else {
				for (i = array->origin; i < array->origin + pos; i++) {
					array->buffer[i - 1] = array->buffer[i];
				}
				array->origin--;
			}
		}
		else {
			/* Move the elements after to the right */
			if (array->count == array->size - array->origin) {
				MBcentarray_resize(array, 0, pos);
			}
			else {
				for (i = array->origin + array->count - 1; i >= array->origin + pos; i--) {
					array->buffer[i + 1] = array->buffer[i];
				}
			}
		}
		array->buffer[array->origin + pos] = data;
		array->count++;
	}
}

void * MBcentarray_remove(MBcentarray *array, unsigned int index)
{
	void *data;
	if (array->count < index + 1) {
		data = NULL;
	}
	else if (index == 0) {
		data = MBcentarray_remove_head(array);
	}
	else if (index == array->count - 1) {
		data = MBcentarray_remove_tail(array);
	}
	else {
		unsigned int i;
		data = array->buffer[array->origin + index];
		for (i = array->origin + index; i < array->origin + array->count - 1; i++) {
			array->buffer[i] = array->buffer[i + 1];
		}
		array->count--;
	}
	return data;
}


void * MBcentarray_get(const MBcentarray * array, unsigned int pos)
{
	if (pos >= array->count) {
		return NULL;
	}
	return array->buffer[pos + array->origin];
}

void * MBcentarray_set(MBcentarray * array, unsigned int pos, void * data)
{
	void * temp;
	if (pos > array->count) {
		return NULL;
	}
	if (pos == array->count) {
		MBcentarray_add_tail(array, data);
		return NULL;
	}
	temp = array->buffer[pos + array->origin];
	array->buffer[pos + array->origin] = data;
	return temp;
}

void MBcentarray_for_each(const MBcentarray * array, MBforfn forfn)
{
	unsigned int i;

	for (i = 0; i < array->count; i++) {
		forfn(array->buffer[i + array->origin]);
	}
}

static void MBcentarray_resize(MBcentarray * array, unsigned int left, unsigned int pos)
{
	const unsigned int size = array->size + array->incr;
	void **newbuffer = malloc(size * sizeof(void*));
	if (newbuffer) {
		void **oldbuffer = array->buffer;
		const unsigned int oldorigin = array->origin;
		array->buffer = newbuffer;
		array->size = size;
		array->origin = (size - array->count) / 2;
		if (pos == 0) {
			/* Copy the buffer in one piece */
			memcpy(&(array->buffer[array->origin]), &(oldbuffer[oldorigin]), array->count * sizeof(void*));
		}
		else {
			/* Copy the elements before pos and after it separately */
			unsigned int dest1, dest2;
			if (left) {
				/* The left block goes one space the left */
				dest1 = array->origin - 1;
				dest2 = array->origin + pos;
			}
			else {
				/* The right block goes one space to the right */
				dest1 = array->origin;
				dest2 = array->origin + pos + 1;
			}
			/* Copy the elements before pos */
			memcpy(&(array->buffer[dest1]), &(oldbuffer[oldorigin]), pos * sizeof(void*));
			/* Copy the elements after pos */
			memcpy(&(array->buffer[dest2]), &(oldbuffer[oldorigin + pos]), (array->count - pos ) * sizeof(void*));
			if (left) {
				/* Move the origin one space to the left */
				array->origin--;
			}
		}
		free(oldbuffer);
		array->incr *= 2;
	}
}

unsigned int MBcentarray_get_count(const MBcentarray * array)
{
	return array->count;
}

typedef struct {
	const MBcentarray *array;
	unsigned int current;
} centarray_iterator;

static centarray_iterator *centarray_iterator_create(const MBcentarray *array)
{
	centarray_iterator *it = malloc(sizeof(centarray_iterator));
	if (it) {
		it->array = array;
		it->current = 0;
	}
	return it;
}

static void centarray_iterator_delete(centarray_iterator *it)
{
	free(it);
}

static void *centarray_iterator_get(centarray_iterator *it)
{
	void *data = NULL;
	if (it->current < it->array->count) {
		data = MBcentarray_get(it->array, it->current);
		it->current++;
	}
	return data;
}

MBiterator *MBcentarray_iterator(const MBcentarray *array)
{
	return MBiterator_create(centarray_iterator_create(array), (MBgetfn)centarray_iterator_get, 
		(MBdeletefn)centarray_iterator_delete);
}
