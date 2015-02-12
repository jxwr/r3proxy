#ifndef _NC_SCRIPT_H_
#define _NC_SCRIPT_H_

struct server_pool;

rstatus_t script_init(struct server_pool *pool);
rstatus_t script_call(struct server_pool *pool, const uint8_t *body, int len, const char *func_name);

/* avoid compiler noise */

rstatus_t ffi_server_table_set(struct server_pool *pool, const char *name, struct server *server);
void ffi_server_table_delete(struct server_pool *pool, const char *name);

struct server* ffi_server_new(struct server_pool *pool, char *name, char *id, char *ip, int port);
rstatus_t ffi_server_connect(struct server *server);
rstatus_t ffi_server_disconnect(struct server *server);
struct string ffi_pool_get_zone(struct server_pool *pool);

struct replicaset* ffi_replicaset_new(void);
void ffi_replicaset_deinit(struct replicaset *rs);
void ffi_replicaset_delete(struct replicaset *rs);

void ffi_replicaset_set_master(struct replicaset *rs, struct server *server);
void ffi_replicaset_add_tagged_server(struct replicaset *rs, int tag_idx, struct server *server);
void ffi_slots_set_replicaset(struct server_pool *pool, struct replicaset *rs, int left, int right);

#endif
