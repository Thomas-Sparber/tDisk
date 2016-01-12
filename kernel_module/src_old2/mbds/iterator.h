/*
 *  iterator.h - an iterator interface type
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef ITERATOR_H
#define ITERATOR_H

#include "mbcommon.h"

struct MBiterator {
	void *      body;
	MBgetfn     getfn;
	MBdeletefn  deletefn;
};
typedef struct MBiterator MBiterator;

MBiterator * MBiterator_create(void *body, MBgetfn getfn, MBdeletefn deletefn);
void         MBiterator_delete(MBiterator *it);
void *       MBiterator_get(MBiterator *it);

#endif /* ITERATOR_H */

