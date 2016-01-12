/*
 *  hashtable.h - a hash table with singly-linked list overflow chains
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <mbcommon.h>
#include <iterator.h>

struct hashnode {
	void * data;
	struct hashnode * next;
};
typedef struct hashnode hashnode;

typedef unsigned int (*MBhashfn)(const void*);

struct MBhashtable {
	unsigned int size;
	MBhashfn hashfn;
	MBcmpfn cmpfn;
	hashnode ** buckets;
	unsigned int count;
};
typedef struct MBhashtable MBhashtable;

MBhashtable * MBhashtable_create(unsigned int size, MBhashfn hashfn, MBcmpfn cmpfn);
void MBhashtable_empty(MBhashtable * table);
void MBhashtable_delete(MBhashtable * table);
void * MBhashtable_add(MBhashtable * table, void * data);
void * MBhashtable_find(const MBhashtable * table, const void * data);
void * MBhashtable_remove(MBhashtable * table, const void * data);
float MBhashtable_get_load_factor(const MBhashtable * table);
unsigned int MBhashtable_get_count(const MBhashtable * table);
void MBhashtable_for_each(const MBhashtable * table, MBforfn forfn);
MBiterator * MBhashtable_iterator(const MBhashtable * table);
unsigned int MBhashtable_find_count(const MBhashtable *table);

#endif /* HASHTABLE_H */



