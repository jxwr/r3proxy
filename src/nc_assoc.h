#ifndef _NC_ASSOC_H_
#define _NC_ASSOC_H_

#include <nc_core.h>

typedef uint32_t (*hash_func_t)(const char *, size_t);

SLIST_HEAD(item_slh, item);

struct hash_table {
    struct item_slh *buckets;
    uint32_t nbuckets;
    uint32_t mask;
    hash_func_t hash;
};

struct hash_table * assoc_create_table(hash_func_t hash, uint32_t sz);
void assoc_destroy_table(struct hash_table *table);

void* assoc_find(struct hash_table *table, const char *key, size_t nkey);
rstatus_t assoc_insert(struct hash_table *table, const char *key, size_t nkey, void *data);
rstatus_t assoc_set(struct hash_table *table, const char *key, size_t nkey, void *data);
void assoc_delete(struct hash_table *table, const char *key, size_t nkey);

#endif
