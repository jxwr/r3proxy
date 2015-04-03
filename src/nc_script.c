#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

#include <nc_core.h>
#include <nc_script.h>
#include <nc_stats.h>

static int 
split(lua_State *L) 
{
    const char *string = luaL_checkstring(L, 1);
    const char *sep = luaL_checkstring(L, 2);
    const char *token;
    int i = 1;
    lua_newtable(L);
    while ((token = strchr(string, *sep)) != NULL) {
        lua_pushlstring(L, string, (size_t)(token - string));
        lua_rawseti(L, -2, i++);
        string = token + 1;
    }
    lua_pushstring(L, string);
    lua_rawseti(L, -2, i);
    return 1;
}
 
static int 
strip(lua_State *L) 
{
    const char *front;
    const char *end;
    size_t      size;
 
    front = luaL_checklstring(L, 1, &size);
    end   = &front[size - 1];

    for ( ; size && isspace(*front) ; size-- , front++)
        ;
    for ( ; size && isspace(*end) ; size-- , end--)
        ;
    
    lua_pushlstring(L, front, (size_t)(end - front) + 1);
    return 1;
}   
    
static const luaL_Reg stringext[] = {
    {"split", split},
    {"strip", strip},
    {NULL, NULL}
};

struct replicaset* 
ffi_replicaset_new(void)
{
    int i;
    struct replicaset *rs;

    rs = nc_alloc(sizeof(struct replicaset));
    if (rs == NULL) {
        log_error("failed to allocate memory");
        return NULL;
    }

    for (i = 0; i < NC_MAXTAGNUM; i++) {
        array_init(&rs->tagged_servers[i], 2, sizeof(struct server *));
    }

    return rs;
}

void
ffi_replicaset_set_master(struct replicaset *rs, struct server *server)
{
    rs->master = server;
}

void
ffi_replicaset_add_tagged_server(struct replicaset *rs, int tag_idx, struct server *server)
{
    struct server **s = array_push(&rs->tagged_servers[tag_idx]);
    *s = server;
}

void
ffi_replicaset_deinit(struct replicaset *rs)
{
    int i;

    for (i = 0; i < NC_MAXTAGNUM; i++) {
        uint32_t n = array_n(&rs->tagged_servers[i]);
        while (n--) {
            array_pop(&rs->tagged_servers[i]);
        }
    }
    rs->master = NULL;
}

void
ffi_replicaset_delete(struct replicaset *rs)
{
    ffi_replicaset_deinit(rs);
    nc_free(rs);
}

struct server*
ffi_server_new(struct server_pool *pool, char *name, char *id, char *ip, int port)
{
    struct server *s;
    struct string address;
    rstatus_t status;

    s = nc_alloc(sizeof(struct server));
    if (s == NULL) {
        log_error("failed to allocate memory");
        return NULL;
    }

    s->owner = pool;
    s->idx = 0;
    s->weight = 1;
    /* set name */
    string_init(&s->name);
    string_copy(&s->name, (uint8_t*)name, (uint32_t)nc_strlen(name));
    string_init(&s->pname);
    string_copy(&s->pname, (uint8_t*)name, (uint32_t)nc_strlen(name));
    string_init(&address);
    string_copy(&address, (uint8_t*)ip, (uint32_t)nc_strlen(ip));
    /* set port */
    s->port = (uint16_t)port;

    status = nc_resolve(&address, s->port, &s->sockinfo);
    if (status != NC_OK) {
        log_error("conf: failed to resolve %.*s:%d", address.len, address.data, s->port);
        return NULL;
    }

    s->family = s->sockinfo.family;
    s->addrlen = s->sockinfo.addrlen;
    s->addr = (struct sockaddr *)&s->sockinfo.addr;

    s->ns_conn_q = 0;
    TAILQ_INIT(&s->s_conn_q);

    s->next_retry = 0LL;
    s->failure_count = 0;
    return s;
}

rstatus_t
ffi_server_connect(struct server *server) {
    struct server_pool *pool;
    struct conn *conn;
    rstatus_t status;

    pool = server->owner;
    conn = server_conn(server);
    if (conn == NULL) {
        return NC_ERROR;
    }

    status = server_connect(pool->ctx, server, conn);
    if (status != NC_OK) {
        log_warn("script: connect to server '%.*s' failed, ignored: %s",
                 server->pname.len, server->pname.data, strerror(errno));
        server_close(pool->ctx, conn);
        return NC_ERROR;
    }
    return NC_OK;
}

rstatus_t
ffi_server_disconnect(struct server *server)
{
    struct server_pool *pool;

    pool = server->owner;
    
    while (!TAILQ_EMPTY(&server->s_conn_q)) {
        struct conn *conn;

        ASSERT(server->ns_conn_q > 0);

        conn = TAILQ_FIRST(&server->s_conn_q);
        conn->close(pool->ctx, conn);
    }

    return NC_OK;
}

void
ffi_slots_lock(struct server_pool *pool) {
    log_debug(LOG_VERB, "lock slots to update");
    pthread_mutex_lock(&pool->slots_mutex);
}

void
ffi_slots_unlock(struct server_pool *pool) {
    log_debug(LOG_VERB, "unlock slots. update done");
    pthread_mutex_unlock(&pool->slots_mutex);
}

void
ffi_slots_set_replicaset(struct server_pool *pool,
                         struct replicaset *rs,
                         int left, int right)
{
    int i;

    log_debug(LOG_VVERB, "script: update slots %d-%d", left, right);

    for (i = left; i <= right; i++) {
        pool->slots[i] = rs;
    }
}

struct string
ffi_pool_get_zone(struct server_pool *pool) {
    return pool->zone;
}

void
ffi_pool_clear_servers(struct server_pool *pool) {
    uint32_t n = array_n(&pool->server);
    while (n--) {
        array_pop(&pool->server);
    }
}

void
ffi_pool_add_server(struct server_pool *pool, struct server *server) {
    uint32_t n;
    struct server **s;

    n = array_n(&pool->server);
    s = array_push(&pool->server);
    *s = server;
    server->idx = n;

    log_debug(LOG_VERB, "add server idx %d port %d", server->idx, server->port);
}

rstatus_t
ffi_server_table_set(struct server_pool *pool, const char *name, struct server *server)
{
    return assoc_set(pool->server_table, name, strlen(name), server);
}

void
ffi_server_table_delete(struct server_pool *pool, const char *name)
{
    return assoc_delete(pool->server_table, name, strlen(name));
}

void
ffi_stats_reset(struct server_pool *pool) {
    struct context *ctx = pool->ctx;
    struct stats *st = ctx->stats;

    stats_reset(st, &ctx->pool);
}

/* init */
rstatus_t
script_init(struct server_pool *pool)
{
    lua_State *L;

    L = luaL_newstate();                        /* Create Lua state variable */
    pool->L = L;
    luaL_openlibs(L);                           /* Load Lua libraries */
    #define MAX_PATH_LEN 1000
    char path[MAX_PATH_LEN] = {'\0'};
    char current[MAX_PATH_LEN] = {'\0'};
    char *lua_name = "lua/redis.lua";

    if (get_my_path(path, sizeof(path)) == NC_ERROR) {
        log_error("get_my_path filed:%s", strerror(errno));
        return NC_ERROR;
    }

    dirname(path);

    log_debug(LOG_VERB, "lua script path is %s", path);

    getcwd(current,sizeof(current));

    if (chdir(path) < 0) {
        log_error("chdir failed: %s", path);
        return NC_ERROR;
    }

    if (luaL_loadfile(L, lua_name)) {
        log_debug(LOG_VERB, "init lua script failed - %s", lua_tostring(L, -1));
        return NC_ERROR;
    }

    lua_getglobal(L, "string");
    luaL_register(L, 0, stringext);
    lua_setglobal(L, "string");

    lua_pushlightuserdata(L, pool);
    lua_setglobal(L, "__pool");

    if (lua_pcall(L, 0, 0, 0) != 0) {
        log_error("call lua script failed - %s", lua_tostring(L, -1));
    }
    if (chdir(current) < 0) {
        log_error("chdir failed: %s", current);
        return NC_ERROR;
    }

    return NC_OK;
}

rstatus_t
script_call(struct server_pool *pool, const uint8_t *body, int len, const char *func_name)
{
    lua_State *L = pool->L;

    log_debug(LOG_VERB, "script: update redis cluster nodes");

    lua_getglobal(L, func_name);
    lua_pushlstring(L, (const char*)body, (size_t)len);

    /* Call update function */
    if (lua_pcall(L, 1, 0, 0) != 0) {
        log_debug(LOG_VERB, "script: call %s failed - %s", func_name, lua_tostring(L, -1));
        return NC_ERROR;
    }

#if 1
    int i = 0;
    struct replicaset *last_rs = NULL;
    for (i = 0; i < REDIS_CLUSTER_SLOTS; i++) {
        struct replicaset *rs = pool->slots[i];
        if (last_rs != rs) {
            last_rs = rs;
            log_debug(LOG_VERB, "slot %5d master %.*s tags[%d,%d,%d,%d,%d]",
                      i, rs->master->pname.len, rs->master->pname.data,
                      array_n(&rs->tagged_servers[0]),
                      array_n(&rs->tagged_servers[1]),
                      array_n(&rs->tagged_servers[2]),
                      array_n(&rs->tagged_servers[3]),
                      array_n(&rs->tagged_servers[4]));
        }
    }
#endif

    return NC_OK;
}
