#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct Item {
  char* key;
  struct hostent* value;
} Item;

typedef struct HashTable {
  int size;
  int count;
  Item** items;
} HashTable;

HashTable* createTable(const size_t size);
void deleteTable(HashTable* ht);
void insert(HashTable* ht, char* key, struct hostent* value);
struct hostent* search(HashTable* ht, char* key);
void delete(HashTable* ht, char* key);
