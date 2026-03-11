#include "../include/num_redefs.h"

#ifndef HASH_H_
#define HASH_H_

typedef struct {
	char key[128];
	int val;

} HashNode;

typedef struct {
	HashNode *nodes;

	u32 count;
	u32 capacity;

} HashMap;

u64 Hash(char *key);
u32 LinearProbe(uint32_t index, uint32_t attempt, uint32_t size);
void HashResize(HashMap *map);
void HashInsert(HashMap *map, char *key, int val);
int HashFetch(HashMap *map, char *key);
void DisplayNodes(HashMap *map);

#endif
