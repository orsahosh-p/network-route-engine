#include <stdlib.h>
#include "hash_table.h"

static size_t hash(uint64_t key, size_t capacity){
	return key % capacity;
}

HashTable* hash_table_create(size_t capacity){
	if(capacity == 0){
		return NULL;
	}

	HashTable* table = malloc(sizeof(*table));
	if(table == NULL){
		return NULL;
	}

	table->buckets = calloc(capacity, sizeof(*table->buckets));
	if(table->buckets == NULL){
		free(table);
		return NULL;
	}

	table->capacity = capacity;
	table->size = 0;
	return table;
}

bool hash_table_insert(HashTable* table, uint64_t key, void* value){
	if(table == NULL){
		return false;
	}

	size_t index = hash(key, table->capacity);
	for(HashEntry* entry = table->buckets[index]; entry != NULL; entry = entry->next){
		if(entry->key == key){
			entry->value = value;
			return true;
		}
	}

	HashEntry* entry = malloc(sizeof(*entry));
	if(entry == NULL){
		return false;
	}

	entry->key = key;
	entry->value = value;
	entry->next = table->buckets[index];
	table->buckets[index] = entry;
	table->size++;
	return true;
}

bool hash_table_remove(HashTable* table, uint64_t key){
	if(table == NULL){
		return false;
	}

	HashEntry** link = &table->buckets[hash(key, table->capacity)];
	while(*link != NULL){
		HashEntry* entry = *link;
		if(entry->key == key){
			*link = entry->next;
			free(entry);
			table->size--;
			return true;
		}
		link = &entry->next;
	}

	return false;
}

void* hash_table_get(HashTable* table, uint64_t key){
	if(table == NULL){
		return NULL;
	}

	for(HashEntry* entry = table->buckets[hash(key, table->capacity)]; entry != NULL; entry = entry->next){
		if(entry->key == key){
			return entry->value;
		}
	}

	return NULL;
}

void hash_table_destroy(HashTable* table){
	if(table == NULL){
		return;
	}

	for(size_t i = 0; i < table->capacity; i++){
		HashEntry* entry = table->buckets[i];
		while(entry != NULL){
			HashEntry* next = entry->next;
			free(entry);
			entry = next;
		}
	}

	free(table->buckets);
	free(table);
}
