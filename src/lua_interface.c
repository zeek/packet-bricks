/* for header definitions */
#include "lua_interface.h"
/* for logging */
#include "pacf_log.h"
/* for basic configuration */
#include "config.h"
/*---------------------------------------------------------------------*/
extern volatile uint32_t stop_processing;
#define METATABLE		PLATFORM_NAME" Metatable"
#define PKTENG_METATABLE	"Packet Engine Metatable"
/*---------------------------------------------------------------------*/
/**
 * PLATFORM LUA INTERFACE 
 */
static int
platform_help_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	fprintf(stdout, PLATFORM_NAME" Commands:\n"
		"    help()\n"
		"    print_status()\n"
		"    shutdown()\n"
		"  Available subsystems within "PLATFORM_NAME" have their own help() methods:\n"
		"    pkteng <under_construction> \n"
		);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
platform_print_status(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	fprintf(stdout, PLATFORM_NAME" is offline.\n");
	fprintf(stdout, "Nothing more here yet."
		"It will start supporting netmap I/O soon...\n");
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
shutdown_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
        stop_processing = 1;
        clean_exit(EXIT_SUCCESS);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static const struct luaL_reg 
platformlib[] = {
        {"help", platform_help_wrap},
        {"print_status", platform_print_status},
        {"shutdown", shutdown_wrap},
        {NULL, NULL},
};
/*---------------------------------------------------------------------*/
static int
platform_dir_create_meta(lua_State *L)
{
	TRACE_LUA_FUNC_START();

        luaL_newmetatable(L, METATABLE);
        /* set its __gc field */
        lua_pushstring(L, "__gc");
        lua_settable(L, -2);

	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
static void
platform_set_info(lua_State *L)
{
	TRACE_LUA_FUNC_START();

        lua_pushliteral(L, "_COPYRIGHT");
        lua_pushliteral(L, "Copyright (C) 2014");
        lua_settable(L, -3);
        lua_pushliteral(L, "_DESCRIPTION");
        lua_pushliteral(L, "PACF command line interface");
        lua_settable(L, -3);
        lua_pushliteral(L, "_VERSION");
        lua_pushliteral(L, PLATFORM_NAME"-"VERSION);
        lua_settable(L, -3);

	TRACE_LUA_FUNC_END();
}
/*---------------------------------------------------------------------*/
static int
luaopen_platform(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	platform_dir_create_meta(L);
        TRACE_LOG("%s", "Loading "PLATFORM_NAME" command metatable\n");
        luaL_openlib(L, PLATFORM_PROMPT, platformlib, 0);
        platform_set_info(L);

	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
/**
 * Packet engine interface
 */
/*---------------------------------------------------------------------*/
static int
pktengine_help_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	fprintf(stdout, "Packet Engine Commands:\n"
		"    help()\n"
		"    new( {name=<ioengine_name>, [cpu=<cpu number>]} )\n"
		"    delete( ioengine_name )\n"
		"    link( {name=<ioengine_name>, ifname=<interface>, batch=<chunk_size>} )\n"
		"    unlink( {name=<ioengine_name>, ifname=<interface>} )\n"
		"    start( ioengine_name )\n"
		"    stop( ioengine_name )\n"
		"    show_stats( ioengine_name )\n"
		);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static void
pkteng_set_info(lua_State *L)
{
	TRACE_LUA_FUNC_START();

        lua_pushliteral (L, "_COPYRIGHT");
        lua_pushliteral (L, "Copyright (C) 2014");
        lua_settable (L, -3);
        lua_pushliteral (L, "_DESCRIPTION");
        lua_pushliteral (L, PLATFORM_NAME" Packet Engine command interface");
        lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "Engine "VERSION);
        lua_settable (L, -3);
	lua_pushinteger(L, 0);
        lua_setglobal(L, "analyze");
        lua_pushinteger(L, 1);
        lua_setglobal(L, "forward");
        lua_pushinteger(L, 2);
        lua_setglobal(L, "block");
        lua_pushinteger(L, 3);
	lua_setglobal(L, "reset");

	TRACE_LUA_FUNC_START();
}
/*---------------------------------------------------------------------*/
static int
pkteng_dir_create_meta(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	luaL_newmetatable (L, PKTENG_METATABLE);
	/* set its __gc field */
        lua_pushstring (L, "__gc");
        lua_settable (L, -2);
	
	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
static const struct luaL_reg
pktenglib[] = {
        {"help", pktengine_help_wrap},
#if 0
        {"new", pktengine_new_wrap},
        {"delete", pktengine_delete_wrap},
        {"link", pktengine_link_dsrc_wrap},
	{"unlink", pktengine_unlink_dsrc_wrap},
	{"start", pktengine_start_wrap},
        {"stop", pktengine_stop_wrap},
        {"show", pktengine_show_wrap},
        {"stats", pktengine_dump_stats_wrap},
#endif
        {NULL, NULL}
};
/*---------------------------------------------------------------------*/
static int
luaopen_pkteng(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	pkteng_dir_create_meta(L);
        TRACE_LOG("%s", "Loading packet engine command metatable\n");
        luaL_openlib(L, "pkteng", pktenglib, 0);
        pkteng_set_info(L);

	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
int
register_lua_procs(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	luaopen_platform(L);
	luaopen_pkteng(L);

	TRACE_LUA_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
