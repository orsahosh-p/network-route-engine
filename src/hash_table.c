#include <stdlib.h>
#include "hash_table.h"

static size_t hash(uint64_t key, size_t capacity){
	return key % capacity;
}

HashTable* hash_table_create(size_t capacity){
	if(capacity == 0){
		return NULL;
	}

	HashTable* table = malloc(sizeof(HashTable));

	if(table == NULL){
		return NULL;
	}

	HashEntry** buckets = calloc(capacity, sizeof(HashEntry*));

	if(buckets == NULL){
		free(table);
		return NULL;
	}

	table->buckets = buckets;
	table->capacity = capacity;
	table->size = 0;

	return table;
}

bool hash_table_insert(HashTable* table, uint64_t key, void* value){
	size_t index = hash(key, table->capacity);

	if(table->buckets[index] == NULL){
		HashEntry* entry = malloc(sizeof(HashEntry));
		if(entry == NULL){
			return false;
		}
		entry->value = value;
		entry->key = key;
		entry->next = NULL;
		table->size++;
		table->buckets[index] = entry;
	}else{
		HashEntry* temp = table->buckets[index];
		HashEntry* next = table->buckets[index];

		while(next != NULL && next->key != key){
			temp = next;
			next = next->next;
		}

		if(next == NULL){
			HashEntry* entry = malloc(sizeof(HashEntry));
			if(entry == NULL){
				return false;
			}
			entry->value = value;
			entry->key = key;
			entry->next = NULL;
			table->size++;
			temp->next = entry;
		}else if(next->key == key){
			next->value = value;
		}

	}

	return true;
}

bool hash_table_remove(HashTable* table, uint64_t key){
	if(table == NULL){
		return false;
	}

	size_t index = hash(key, table->capacity);

	if(table->buckets[index] == NULL){
		return false;
	}else if(table->buckets[index]->key == key){
		HashEntry* entry = table->buckets[index];
		table->buckets[index] = entry->next;
		free(entry);
		table->size--;
	}else{
		HashEntry* parent = table->buckets[index];
		HashEntry* current = table->buckets[index];

		while(current != NULL && current->key != key){
			parent = current;
			current = current->next;
		}

		if(current == NULL){
			return false;
		}else if(current->key == key){
			parent->next = current->next;
			free(current);
			table->size--;
		}
	}

	return true;

}


void* hash_table_get(HashTable* table, uint64_t key){
	if(table == NULL){
		return NULL;
	}

	size_t index = hash(key, table->capacity);

	HashEntry* temp = table->buckets[index];

	while(temp != NULL && temp->key != key){
		temp = temp->next;
	}

	if(temp == NULL){
		return NULL;
	}

	return temp->value;
}

void hash_table_destroy(HashTable* table){
	if(table == NULL){
		return;
	}

	for(size_t i = 0; i < table->capacity; i++){
		HashEntry* temp = table->buckets[i];
		while(temp != NULL){
			HashEntry* remove = temp;
			temp = temp->next;
			free(remove);
		}
	}
	free(table->buckets);
	free(table);
}
