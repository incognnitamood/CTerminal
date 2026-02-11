#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct VarEntry {
    char *key;
    char *value;
    struct VarEntry *next;
} VarEntry;

typedef struct {
    VarEntry **buckets;
    int bucket_count;
} HashMap;

void hm_init(HashMap *map, int bucket_count);
void hm_set(HashMap *map, const char *key, const char *value);
char *hm_get(HashMap *map, const char *key);
void hm_unset(HashMap *map, const char *key);
void hm_list(HashMap *map, char ***pairs, int *count);

#endif

#ifndef HASHMAP_H
#define HASHMAP_H

#include "utils.h"

#define HASHMAP_SIZE 128

typedef struct HashNode {
    char* key;
    char* value;
    struct HashNode* next;
} HashNode;

typedef struct {
    HashNode* buckets[HASHMAP_SIZE];
} HashMap;

void hashmap_init(HashMap* map);
void hashmap_set(HashMap* map, const char* key, const char* value);
char* hashmap_get(HashMap* map, const char* key);
int hashmap_unset(HashMap* map, const char* key);
void hashmap_list(HashMap* map, char* output, int output_size);
void hashmap_cleanup(HashMap* map);

#endif
