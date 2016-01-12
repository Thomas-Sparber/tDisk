/* 
 *  MBcentarray.h - a dynamic array with centring of the used block
 * 
 *  Copyright (C) 2010 Martin Broadhurst (martin@martinbroadhurst.com) 
 *
 */

#ifndef MBCENTARRAY_H
#define MBCENTARRAY_H

#include <iterator.h>
#include <mbcommon.h>

struct MBcentarray {
	void ** buffer;
	unsigned int size;
	unsigned int count;
	unsigned int incr;
	unsigned int origin;
};

typedef struct MBcentarray MBcentarray;

MBcentarray * MBcentarray_create(void);
void MBcentarray_empty(MBcentarray * array);
void MBcentarray_delete(MBcentarray * array);
void MBcentarray_add_tail(MBcentarray * array, void * data);
void MBcentarray_add_head(MBcentarray * array, void * data);
void * MBcentarray_remove_tail(MBcentarray * array);
void * MBcentarray_remove_head(MBcentarray * array);
void MBcentarray_insert(MBcentarray *array, unsigned int pos, void *data);
void * MBcentarray_remove(MBcentarray *array, unsigned int index);
void * MBcentarray_get(const MBcentarray * array, unsigned int pos);
void * MBcentarray_set(MBcentarray * array, unsigned int pos, void * data);
void MBcentarray_for_each(const MBcentarray * array, MBforfn forfn);
unsigned int MBcentarray_get_count(const MBcentarray * array);
MBiterator *MBcentarray_iterator(const MBcentarray *array);

#endif /* MBCENTARRAY_H */
