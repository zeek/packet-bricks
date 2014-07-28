#ifndef __LUA_INTERFACE_H__
#define __LUA_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for lua interface */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
/*---------------------------------------------------------------------*/
/**
 * Struct that accepts lua commands corresponding to the 
 * PktEngine module. All resource management pertaining
 * to this struct is governed by the LUA interpreter.
 *
 * Garbage collection is implemented in LUA...
 */
typedef struct PktEngine {
	const char *eng_name;
	const char *ifname;
	const char *type;
	int cpu;
	int batch;
	int qid;
} PktEngine;
/*---------------------------------------------------------------------*/
/** 
 * Register multiple lua modules in this func
 */
int register_lua_procs(lua_State *L);

/**
 * called by shutdown function...
 * this will free up all preprocessors etc.
 */
extern void clean_exit(int);
/*---------------------------------------------------------------------*/
#endif /* !__LUA_INTERFACE_H__ */
