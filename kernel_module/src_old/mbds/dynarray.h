/*
 *  dynarray.h - a dynamic array
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef DYNARRAY_H
#define DYNARRAY_H

#include "iterator.h"
#include "mbcommon.h"

struct MBdynarray {
	void ** buffer;
	unsigned int size;
	unsigned int count;
};

typedef struct MBdynarray MBdynarray;

MBdynarray * MBdynarray_create(unsigned int startsize);
void MBdynarray_empty(MBdynarray * array);
void MBdynarray_delete(MBdynarray * array);
void MBdynarray_add_tail(MBdynarray * array, void * data);
void MBdynarray_add_head(MBdynarray * array, void * data);
void * MBdynarray_remove_tail(MBdynarray * array);
void * MBdynarray_remove_head(MBdynarray * array);
void MBdynarray_insert(MBdynarray *array, unsigned int pos, void *data);
void * MBdynarray_remove(MBdynarray *array, unsigned int index);
void * MBdynarray_get(const MBdynarray * array, unsigned int pos);
void * MBdynarray_set(MBdynarray * array, unsigned int pos, void * data);
void MBdynarray_for_each(const MBdynarray * array, MBforfn forfn);
unsigned int MBdynarray_get_count(const MBdynarray * array);
void MBdynarray_set_size(MBdynarray * array, unsigned int size);
MBiterator *MBdynarray_iterator(const MBdynarray *array);

#endif /* DYNARRAY_H */
