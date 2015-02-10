#ifndef _NC_SCRIPT_H_
#define _NC_SCRIPT_H_

struct server_pool;

int script_init(struct server_pool *pool);
int script_call(struct server_pool *pool, const char *body, int len, const char *func_name);

struct replicaset;
struct server;
struct server_pool;

/* avoid warning */
int ffi_slots_set_replicaset(struct server_pool *pool, struct replicaset *rs, int left, int right);

struct replicaset* ffi_replicaset_new(void);
struct replicaset* ffi_replicaset_deinit(struct replicaset *rs);
struct replicaset* ffi_replicaset_delete(struct replicaset *rs);
void ffi_replicaset_set_master(struct replicaset *rs, struct server *server);
void ffi_replicaset_add_slave(struct replicaset *rs, int tag_idx, struct server *server);

struct string ffi_pool_get_region(struct server_pool *pool);
struct string ffi_pool_get_zone(struct server_pool *pool);
struct string ffi_pool_get_room(struct server_pool *pool);
struct string ffi_pool_get_failover_zones(struct server_pool *pool);

bool ffi_server_connect(struct server *server);
bool ffi_server_disconnect(struct server *server);
struct server* ffi_server_new(struct server_pool *pool, 
                              const char *name, const char *id, const char *ip, int port);

#endif
