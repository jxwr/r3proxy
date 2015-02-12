#include <nc_core.h>

#define HASHSIZE(_n) (1 << (_n))
#define HASHMASK(_n) (HASHSIZE(_n) - 1)

struct item {
    SLIST_ENTRY(item) h_sle;    /* link in hash */
    struct string  key;         /* key */
    void          *data;        /* pointer to data */
};


static struct item *
assoc_create_item(const char *key, size_t nkey, void *data)
{
    struct item *it;

    ASSERT(key != NULL && nkey != 0 && data != NULL);

    it = nc_alloc(sizeof(*it));
    if (it == NULL) {
        return NULL;
    }
    
    it->key.data = (uint8_t*)key;
    it->key.len = (uint32_t)nkey;
    it->data = data;

    return it;
}

static void
assoc_destroy_item(struct item *it)
{
    ASSERT(it != NULL);

    nc_free(it);
}

struct hash_table *
assoc_create_table(hash_func_t hash, uint32_t sz)
{
    struct hash_table *table;
    struct item_slh *buckets;
    uint32_t i, hash_power;
    
    ASSERT(sz != 0);

    table = nc_alloc(sizeof(*table));
    if (table == NULL) {
        return NULL;
    }

    for (hash_power = 0; HASHSIZE(hash_power) < sz; hash_power++);
    
    sz = HASHSIZE(hash_power);
    
    buckets = nc_alloc(sizeof(*buckets) * sz);
    if (buckets == NULL) {
        return NULL;
    }
    
    for (i = 0; i < sz; i++) {
        SLIST_INIT(&buckets[i]);
    }

    table->buckets = buckets;
    table->nbuckets = sz;
    table->mask = HASHMASK(hash_power);
    table->hash = hash;

    return table;
}

void
assoc_destroy_table(struct hash_table *table)
{
    struct item_slh *bucket;
    struct item *it, *next;
    uint32_t i;
    
    ASSERT(table != NULL && table->buckets != NULL && table->nbuckets != 0);

    for (i = 0; i < table->nbuckets; i++) {
        bucket = &table->buckets[i];
        SLIST_FOREACH_SAFE(it, bucket, h_sle, next) {
            SLIST_REMOVE(bucket, it, item, h_sle);
            assoc_destroy_item(it);
         }
    }
    
    nc_free(table->buckets);
    nc_free(table);
}

static struct item_slh *
assoc_find_bucket(struct hash_table *table, const char *key, size_t nkey)
{
    struct item_slh *bucket;
    uint32_t hv;

    ASSERT(table != NULL && table->buckets != NULL && table->nbuckets != 0);
    ASSERT(key != NULL && nkey != 0);

    hv = table->hash(key, nkey);
    hv &= table->mask;
    bucket = &table->buckets[hv];
    
    return bucket;
}

void *
assoc_find(struct hash_table *table, const char *key, size_t nkey)
{
    struct item_slh *bucket;
    struct item *it;
    
    ASSERT(table != NULL && table->buckets != NULL && table->nbuckets != 0);
    ASSERT(key != NULL && nkey != 0);

    bucket = assoc_find_bucket(table, key, nkey);

    SLIST_FOREACH(it, bucket, h_sle) {
        if (nkey == it->key.len && (nc_strncmp(key, it->key.data, nkey) == 0)) {
            break;
        }
    }
    
    if (it) {
        return it->data;
    } else {
        return NULL;
    }
}

rstatus_t
assoc_set(struct hash_table *table, const char *key, size_t nkey, void *data)
{
    struct item_slh *bucket;
    struct item *it;
    
    ASSERT(table != NULL && table->buckets != NULL && table->nbuckets != 0);
    ASSERT(key != NULL && nkey != 0);

    bucket = assoc_find_bucket(table, key, nkey);

    SLIST_FOREACH(it, bucket, h_sle) {
        if (nkey == it->key.len && (nc_strncmp(key, it->key.data, nkey) == 0)) {
            break;
        }
    }
    
    if (it) {
        it->data = data;
        return NC_OK;
    } else {
        it = assoc_create_item(key, nkey, data);
        if (it == NULL) {
            return NC_ENOMEM;
        }
        SLIST_INSERT_HEAD(bucket, it, h_sle);
    }
    return NC_OK;
}

rstatus_t
assoc_insert(struct hash_table *table, const char *key, size_t nkey, void *data)
{
    struct item_slh *bucket;
    struct item *it;

    ASSERT(assoc_find(table, key, nkey) == NULL);
    
    bucket = assoc_find_bucket(table, key, nkey);
    
    it = assoc_create_item(key, nkey, data);
    if (it == NULL) {
        return NC_ENOMEM;
    }

    SLIST_INSERT_HEAD(bucket, it, h_sle);
    return NC_OK;
}


void
assoc_delete(struct hash_table *table, const char *key, size_t nkey)
{
    struct item_slh *bucket;
    struct item *it, *next;
    
    ASSERT(table != NULL && table->buckets != NULL && table->nbuckets != 0);
    ASSERT(key != NULL && nkey != 0);
    
    bucket = assoc_find_bucket(table, key, nkey);

    SLIST_FOREACH_SAFE(it, bucket, h_sle, next) {
        if (nkey == it->key.len && (nc_strncmp(key, it->key.data, nkey) == 0)) {
            /* FIXME: don't use this code in critical path */
            SLIST_REMOVE(bucket, it, item, h_sle);
            assoc_destroy_item(it);
            break;
        }
    }
}
