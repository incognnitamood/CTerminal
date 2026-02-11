#include "hashmap.h"
#include "utils.h"

static unsigned long hm_hash(const char *s) {
    unsigned long h = 5381;
    int i = 0;
    int c;
    if (!s) return 0;
    while ((c = s[i]) != 0) {
        h = ((h << 5) + h) + c;
        i++;
    }
    return h;
}

void hm_init(HashMap *map, int bucket_count) {
    int i;
    map->bucket_count = bucket_count;
    map->buckets = (VarEntry **)u_malloc(sizeof(VarEntry *) * bucket_count);
    for (i = 0; i < bucket_count; i++) {
        map->buckets[i] = 0;
    }
}

void hm_set(HashMap *map, const char *key, const char *value) {
    unsigned long h = hm_hash(key);
    int idx = (int)(h % map->bucket_count);
    VarEntry *e = map->buckets[idx];
    VarEntry *prev = 0;
    while (e) {
        if (u_strcmp(e->key, key) == 0) {
            if (e->value) u_free(e->value);
            e->value = u_strdup(value);
            return;
        }
        prev = e;
        e = e->next;
    }
    e = (VarEntry *)u_malloc(sizeof(VarEntry));
    e->key = u_strdup(key);
    e->value = u_strdup(value);
    e->next = 0;
    if (prev) {
        prev->next = e;
    } else {
        map->buckets[idx] = e;
    }
}

char *hm_get(HashMap *map, const char *key) {
    unsigned long h = hm_hash(key);
    int idx = (int)(h % map->bucket_count);
    VarEntry *e = map->buckets[idx];
    while (e) {
        if (u_strcmp(e->key, key) == 0) {
            return e->value;
        }
        e = e->next;
    }
    return 0;
}

void hm_unset(HashMap *map, const char *key) {
    unsigned long h = hm_hash(key);
    int idx = (int)(h % map->bucket_count);
    VarEntry *e = map->buckets[idx];
    VarEntry *prev = 0;
    while (e) {
        if (u_strcmp(e->key, key) == 0) {
            if (prev) prev->next = e->next;
            else map->buckets[idx] = e->next;
            if (e->key) u_free(e->key);
            if (e->value) u_free(e->value);
            u_free(e);
            return;
        }
        prev = e;
        e = e->next;
    }
}

void hm_list(HashMap *map, char ***pairs, int *count) {
    int total = 0;
    int i;
    VarEntry *e;
    char **arr;
    int k = 0;

    for (i = 0; i < map->bucket_count; i++) {
        e = map->buckets[i];
        while (e) {
            total++;
            e = e->next;
        }
    }
    if (total == 0) {
        *pairs = 0;
        *count = 0;
        return;
    }
    arr = (char **)u_malloc(sizeof(char *) * total);
    for (i = 0; i < map->bucket_count; i++) {
        e = map->buckets[i];
        while (e) {
            int lenk = u_strlen(e->key);
            int lenv = u_strlen(e->value);
            char *s = (char *)u_malloc(lenk + 1 + lenv + 1);
            int j;
            for (j = 0; j < lenk; j++) s[j] = e->key[j];
            s[lenk] = '=';
            for (j = 0; j < lenv; j++) s[lenk + 1 + j] = e->value[j];
            s[lenk + 1 + lenv] = 0;
            arr[k++] = s;
            e = e->next;
        }
    }
    *pairs = arr;
    *count = total;
}

