#ifndef _NC_SCRIPT_H_
#define _NC_SCRIPT_H_

struct server_pool;

int script_init(struct server_pool *pool);
int script_call(struct server_pool *pool, const char *body, int len, const char *func_name);

#endif
