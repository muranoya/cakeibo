#ifndef HASHMAP_H
#define HASHMAP_H

#include "string.h"

typedef struct Hashmap Hashmap;

Hashmap *make_hashmap(int size);
void free_hashmap(Hashmap **h);
int  add_hashmap(Hashmap *h, const String *key, int data);
int  remove_hashmap(Hashmap *h, const String *key);
int  exists_hashmap(Hashmap *h, const String *key);
int  search_hashmap(Hashmap *h, const String *key, int *data);

#endif
