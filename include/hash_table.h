#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct HashEntry{
	uint64_t key;
	void* value;
	struct HashEntry* next;
} HashEntry;

typedef struct {
	HashEntry** buckets;
	size_t capacity;
	size_t size;
} HashTable;

HashTable *hash_table_create(size_t capacity);

bool hash_table_insert(HashTable* table, uint64_t key, void *value);

void* hash_table_get(HashTable* table, uint64_t key);

bool hash_table_remove(HashTable* table, uint64_t key);

void hash_table_destroy(HashTable* table);

#endif
