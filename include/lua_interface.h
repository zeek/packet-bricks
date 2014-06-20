#ifndef __LUA_INTERFACE_H__
#define __LUA_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
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
