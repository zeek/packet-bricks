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
 * PktEngine abstract. All resource management pertaining
 * to this struct is governed by the LUA interpreter.
 *
 * Garbage collection is implemented in LUA...
 */
typedef struct PktEngine_Intf {
	const char *eng_name;			/* engine name */
	const char *type;			/* netmap */
	int cpu;				/* engine runs on this thread */
	int batch;				/* batch size */
	int qid;				/* queue id */
	int buffer_sz;				/* buffer size */
} PktEngine_Intf;

/**
 * Struct that accepts lua commands corresponding to the 
 * LoadBalance/Duplicate/Merge/Filter/? abstract. All resource management 
 * pertaining to this struct is governed by the LUA interpreter.
 *
 * Garbage collection is implemented in LUA...
 */
#define MAX_INLINKS			20
#define MAX_OUTLINKS			MAX_INLINKS

typedef struct Linker_Intf {
	int type;				/* lb/dup/merge/filter? */
	int hash_split;				/* 2-tuple or 4-tuple split? */
	const char *input_link[MAX_INLINKS];	/* ingress interface name */
	const char *output_link[MAX_OUTLINKS];	/* outgress interface names */
	int output_count;			/* output links count */
	/* 
	 * input links count (multiple count with merge) 
	 */
	int input_count;			
	struct Linker_Intf *next_linker;	/* pointer to next link */
} Linker_Intf;
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
