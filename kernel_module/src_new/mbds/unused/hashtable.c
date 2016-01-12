/*
 *  hashtable.c - a hash table with singly-linked list overflow chains
 *  Copyright (C) 2010 Martin Broadhurst 
 *  www.martinbroadhurst.com
 */

#include <stdlib.h>

#include <hashtable.h>

hashnode * hashnode_create(void * data)
{
	hashnode * node = malloc(sizeof(hashnode));
	if (node) {
		node->data = data;
		node->next = NULL;
	}
	return node;
}

void hashnode_delete(hashnode * node)
{
	free(node);
}

MBhashtable * MBhashtable_create(unsigned int size, MBhashfn hashfn, MBcmpfn cmpfn)
{
	MBhashtable * table = malloc(sizeof (MBhashtable));
	if (table) {
		table->size = size;
		table->hashfn = hashfn;
		table->cmpfn = cmpfn;
		table->count = 0;
		table->buckets = malloc(size * sizeof(hashnode *));
		if (table->buckets) {
			unsigned int b;
			for (b = 0; b < size; b++) {
				table->buckets[b] = NULL;
			}
		}
		else {
			free(table);
			table = NULL;
		}
	}
	return table;
}

void MBhashtable_empty(MBhashtable * table)
{
	unsigned int i;
	hashnode * temp;
	for (i = 0; i < table->size; i++) {
		hashnode * current = table->buckets[i];
		while (current != NULL) {
			temp = current->next;
			hashnode_delete(current);
			current = temp;
		}
	}
	table->count = 0;
}

void MBhashtable_delete(MBhashtable * table)
{
	if (table) {
		MBhashtable_empty(table);
		free(table->buckets);
		free(table);
	}
}

static void * MBhashtable_add_internal(hashnode ** buckets, size_t size, 
		MBhashfn hashfn, MBcmpfn cmpfn, void * data)
{
	const unsigned int bucket = hashfn(data) % size;
	void * found = NULL;
	unsigned int added = 0;

	if (buckets[bucket] == NULL) {
		/* An empty bucket */
		buckets[bucket] = hashnode_create(data);
		added = 1;
	}
	else {
		hashnode * current, * previous = NULL;
		for (current = buckets[bucket]; 
				current != NULL && !found && !added; current = current->next) {
			const int result = cmpfn(current->data, data);
			if (result == 0) {
				/* Changing an existing entry */
				found = current->data;
				current->data = data;
			}
			else if (result > 0) {
				/* Add before current */
				hashnode * node = hashnode_create(data);
				node->next = current;
				if (previous == NULL) {
					/* Adding at the front */
					buckets[bucket] = node;
				}
				else {
					previous->next = node;
				}
				added = 1;
			}
			previous = current;
		}
		if (!added && current == NULL) {
			/* Adding at the end */
			previous->next = hashnode_create(data);
		}
	}

	return found;
}

static void MBhashtable_resize(MBhashtable * table)
{
	size_t size = table->size * 2;
	hashnode ** buckets = malloc(size * sizeof(hashnode));
	if (buckets) {
		unsigned int b;
		for (b = 0; b < size; b++) {
			buckets[b] = NULL;
		}
		for (b = 0; b < table->size; b++) {
			const hashnode *node;
			for (node = table->buckets[b]; node != NULL; node = node->next) {
				MBhashtable_add_internal(buckets, size, table->hashfn, table->cmpfn, node->data);
			}
		}
		free(table->buckets);
		table->buckets = buckets;
		table->size = size;
	}
}

void * MBhashtable_add(MBhashtable * table, void * data)
{
	void * found;
	if (table->size == table->count) {
		MBhashtable_resize(table);
	}
	found = MBhashtable_add_internal(table->buckets, table->size, 
			table->hashfn, table->cmpfn, data);
	if (found == NULL) {
		table->count++;
	}

	return found;
}

unsigned int MBhashtable_find_count(const MBhashtable *table)
{
	unsigned int b;
	hashnode *node;
	unsigned int count = 0;
	for (b = 0; b < table->size; b++) {
		for (node = table->buckets[b]; node != NULL; node = node->next) {
			count++;
		}
	}
	return count;
}

void * MBhashtable_find(const MBhashtable * table, const void * data)
{
	hashnode * current;
	const unsigned int bucket = table->hashfn(data) % table->size;
	void * found = NULL;
	unsigned int passed = 0;
	for (current = table->buckets[bucket]; current != NULL && !found && !passed; current = current->next) {
		const int result = table->cmpfn(current->data, data);
		if (result == 0) {
			found = current->data;
		}
		else if (result > 0) {
			passed = 1;
		}
	}
	return found;
}

void * MBhashtable_remove(MBhashtable * table, const void * data)
{
	hashnode * current, * previous = NULL;
	const unsigned int bucket = table->hashfn(data) % table->size;
	void * found = NULL;
	unsigned int passed = 0;

	current = table->buckets[bucket];
	while (current != NULL && !found && !passed) {
		const int result = table->cmpfn(current->data, data);
		if (result == 0) {
			found = current->data;
			if (previous == NULL) {
				/* Removing the first entry */
				table->buckets[bucket] = current->next;
			}
			else {
				previous->next = current->next;
			}
			hashnode_delete(current);
			table->count--;
		}
		else if (result > 0) {
			passed = 1;
		}
		else {
			previous = current;
	 		current = current->next;
		}
	}
	return found;
}


float MBhashtable_get_load_factor(const MBhashtable * table)
{
	unsigned int touched = 0;
	float loadfactor;
	unsigned int b;
	for (b = 0; b < table->size; b++) {
		if (table->buckets[b] != NULL) {
			touched++;
		}
	}
	loadfactor = (float)touched / (float)table->size;
	return loadfactor;
}

unsigned int MBhashtable_get_count(const MBhashtable * table)
{
	return table->count;
}

void MBhashtable_for_each(const MBhashtable * table, MBforfn forfn)
{
	unsigned int b;

	for (b = 0; b < table->size; b++) {
		const hashnode *node;
		for (node = table->buckets[b]; node != NULL; node = node->next) {
			forfn(node->data);
		}
	}
}

typedef struct {
	const MBhashtable * table;
	unsigned int bucket;
	hashnode * node;
} hashtable_iterator;

hashtable_iterator * hashtable_iterator_create(const MBhashtable * table)
{
	hashtable_iterator * it = malloc(sizeof(hashtable_iterator));
	if (it) {
		it->table = table;
		it->node = NULL;
		if (table->count) {
			it->bucket = 0;
			while (it->node == NULL) {
				it->node = table->buckets[it->bucket];
				if (it->node == NULL) {
					it->bucket++;
				}	
			}
		}
		else {
			it->bucket = it->table->size; /* Sentinel value for "finished" */
		}
	}
	return it;
}

void hashtable_iterator_delete(hashtable_iterator * it)
{
	free(it);
}

void * hashtable_iterator_get(hashtable_iterator * it)
{
	void * data = NULL;
	if (it->bucket < it->table->size) {
		data = it->node->data;
		it->node = it->node->next;
		while (it->node == NULL && it->bucket < it->table->size)  {
			it->bucket++;
			if (it->bucket < it->table->size) {
				it->node = it->table->buckets[it->bucket];
			}
		}
	}
	return data;
}

MBiterator * MBhashtable_iterator(const MBhashtable * table)
{
	return MBiterator_create(hashtable_iterator_create(table), (MBgetfn)hashtable_iterator_get,
		   	(MBdeletefn)hashtable_iterator_delete);
}
