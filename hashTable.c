#include "hashTable.h"

const static Item DELETED_ITEM = {NULL, NULL};

static Item* createItem(const char* key, struct hostent* value)
{

  Item* item = malloc(sizeof(Item));
  item->key = strdup(key);
  item->value = malloc(sizeof(value));
  item->value = value;
  return item;

}

HashTable* createTable(const size_t size)
{

  HashTable* ht = malloc(sizeof(HashTable));
  ht->size = size;
  ht->count = 0;
  ht->items = calloc(size, sizeof(Item*));
  return ht;

}

static void deleteItem(Item* item)
{

  free(item->key);
  free(item->value);
  free(item);

}

void deleteTable(HashTable* ht)
{

  int i;
  for(i = 0; i < ht->size; i++) {
    Item* item = ht->items[i];
    if(item != NULL) {
      deleteItem(item);
    }
  }
  free(ht->items);
  free(ht);

}

static int hashFunction(const char* s, const int a, const int m)
{

    long hash = 0;
    const int len_s = strlen(s);
    for (int i = 0; i < len_s; i++) {
        hash += (long)pow(a, len_s - (i+1)) * s[i];
        hash = hash % m;
    }
    return (int)hash;

}

static int getHash(const char* s, const int num_buckets, const int attempt)
{

    const int hash_a = hashFunction(s, 1783, num_buckets);
    const int hash_b = hashFunction(s, 2281, num_buckets);
    return (hash_a + (attempt * (hash_b + 1))) % num_buckets;

}

void insert(HashTable* ht, char* key, struct hostent* value)
{

  int attempts = 1;
  Item* item = createItem(key, value);
  int index = getHash(item->key, ht->size, 0);
  Item* currentItem = ht->items[index];

  while(currentItem != NULL && currentItem != &DELETED_ITEM) {
    index = getHash(item->key, ht->size, attempts);
    currentItem = ht->items[index];
    attempts++;
  }
  ht->items[index] = item;
  ht->count++;

}

struct hostent* search(HashTable* ht, char* key)
{

  int attempts = 1;
  int index = getHash(key, ht->size, 0);
  Item* item = ht->items[index];
  while(item != NULL) {
    if(item != &DELETED_ITEM) {
      if(!strcmp(item->key, key)) {
        return item->value;
      }
    }
    index = getHash(key, ht->size, attempts);
    item = ht->items[index];
    attempts++;
  }
  return NULL;

}

void delete(HashTable* ht, char* key)
{

  int attempts = 1;
  int index = getHash(key, ht->size, 0);
  Item* item = ht->items[index];
  while(item != NULL) {
    if(item != &DELETED_ITEM) {
      if(!strcmp(item->key, key)) {
        deleteItem(item);
        ht->items[index] = &DELETED_ITEM;
      }
    }
    index = getHash(key, ht->size, attempts);
    item = ht->items[index];
    attempts++;
  }
  ht->count--;

}
