/*
 *  cirque.h - a circular queue
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef CIRQUE_H
#define CIRQUE_H

#include <mbcommon.h>
#include <iterator.h>

struct MBcirque {
	unsigned int head; /* First element */
	unsigned int tail; /* 1 past the last element */
	unsigned int is_full;
	void ** entries;
	unsigned int size;
};

typedef struct MBcirque MBcirque;

MBcirque * MBcirque_create(unsigned int size);
void MBcirque_delete(MBcirque * cirque);
unsigned int MBcirque_insert(MBcirque * cirque, void * data);
void *  MBcirque_remove(MBcirque * cirque);
void *MBcirque_peek(const MBcirque * cirque);
unsigned int MBcirque_get_count(const MBcirque * cirque);
void MBcirque_for_each(const MBcirque * cirque, MBforfn forfn);
MBiterator *MBcirque_iterator(const MBcirque * cirque);

#endif /* CIRQUE_H */
