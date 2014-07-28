/* for header definitions */
#include "lua_interface.h"
/* for LUA_MAXINPUT */
#include "lua_interpreter.h"
/* for logging */
#include "pacf_log.h"
/* for basic configuration */
#include "config.h"
/* for pktengine defn's */
#include "pkt_engine.h"
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
	/* this prints the system's help menu */
	TRACE_LUA_FUNC_START();
	UNUSED(L);
	fprintf(stdout, PLATFORM_NAME" Commands:\n"
		"    help()\n"
		"    print_status()\n"
		"    show_stats()\n"
		"    shutdown()\n"
		"  Available subsystems within "PLATFORM_NAME" have their own help() methods:\n"
		"    pkteng \n"
		);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
platform_print_status(lua_State *L)
{
	/* this prints the system's current status */
	TRACE_LUA_FUNC_START();
	uint8_t rc = 0;

	rc = is_pktengine_online((unsigned char *)"any");
	if (rc == 0)
		fprintf(stdout, PLATFORM_NAME" is offline.\n");
	else 
		fprintf(stdout, PLATFORM_NAME" is online.\n");

	TRACE_LUA_FUNC_END();
	UNUSED(L);
        return 0;
}
/*---------------------------------------------------------------------*/
static int
platform_show_stats(lua_State *L)
{
	/* this prints the system's current status */
	TRACE_LUA_FUNC_START();
	pktengines_list_stats(stdout);
	TRACE_LUA_FUNC_END();
	UNUSED(L);
        return 0;
}
/*---------------------------------------------------------------------*/
static int
shutdown_wrap(lua_State *L)
{
	/* this shut downs the system */
	TRACE_LUA_FUNC_START();
	UNUSED(L);
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
	{"show_stats", platform_show_stats},
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
        lua_pushliteral(L, PLATFORM_NAME" command line interface");
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
	/* load systems' lua interface */
	TRACE_LUA_FUNC_START();

	platform_dir_create_meta(L);
        TRACE_DEBUG_LOG("%s", "Loading "PLATFORM_NAME" command metatable\n");
        luaL_openlib(L, PLATFORM_NAME, platformlib, 0);
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
		"    open_channel(<channel_name>, <action_name>)\n"
	     	"    drop_pkts()\n"
	     	"    redirect_pkts(<interface>)\n"
		"    new(<ioengine_name>, <io_type>, <cpu number>)\n"
		"    delete()\n"
		"    link(<interface>, <chunk_size>)\n"
		"    unlink()\n"
		"    start()\n"
		"    stop()\n"
		"    show_stats()\n"
		);
	UNUSED(L);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static PktEngine *
to_pkteng(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = (PktEngine *)lua_touserdata(L, index);
	if (pe == NULL) luaL_typerror(L, index, "PktEngine");
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static PktEngine *
push_pkteng(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = (PktEngine *)lua_newuserdata(L, sizeof(PktEngine));
	luaL_getmetatable(L, "PktEngine");
	lua_setmetatable(L, -2);
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static PktEngine *
check_pkteng(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe;
	luaL_checktype(L, index, LUA_TUSERDATA);
	pe = (PktEngine *)luaL_checkudata(L, index, "PktEngine");
	if (pe == NULL) luaL_typerror(L, index, "PktEngine");
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static int
pkteng_new(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	const char *ename = luaL_optstring(L, 1, 0);
	const char *type = luaL_optstring(L, 2, 0);
	int cpu = luaL_optint(L, 3, 0);

	PktEngine *pe = push_pkteng(L);
	pe->eng_name = ename;
	pe->type = type;
	pe->cpu = cpu;

	pktengine_new((uint8_t *)pe->eng_name, (uint8_t *)pe->type, pe->cpu);
	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_link(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	pe->ifname = luaL_checkstring(L, 2);
	pe->batch = luaL_checkint(L, 3);
	pe->qid = luaL_checkint(L, 4);
	lua_settop(L, 1);

	TRACE_DEBUG_LOG("Engine info so far...:\n"
			"\tName: %s\n"
			"\tIfname: %s\n"
			"\tType: %s\n"
			"\tCpu: %d\n"
			"\tBatch: %d\n"
			"\tQid: %d\n",
			pe->eng_name,
			pe->ifname,
			pe->type,
			pe->cpu,
			pe->batch,
			pe->qid);
	
	pktengine_link_iface((uint8_t *)pe->eng_name, 
			     (uint8_t *)pe->ifname, 
			     pe->batch, pe->qid);
	
	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_open_channel(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int rc;
	const char *cname;
	const char *action;
	PktEngine *pe = check_pkteng(L, 1);
	cname = luaL_checkstring(L, 2);
	action = luaL_checkstring(L, 3);
	lua_settop(L, 1);

	rc = pktengine_open_channel((uint8_t *)pe->eng_name, 
				    (uint8_t *)cname, 
				    (uint8_t *)action);
	if (rc == -1) {
		TRACE_LOG("Failed to open channel %s\n", cname);
	}
	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_redirect_pkts(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int rc;
	PktEngine *pe = check_pkteng(L, 1);
	const char *oifname = luaL_checkstring(L, 2);
	lua_settop(L, 1);
	rc = pktengine_redirect_pkts((uint8_t *)pe->eng_name, 
				     (uint8_t *)oifname);
	if (rc == -1) {
		TRACE_LOG("Failed to set redirect target for engine %s\n", 
			  pe->eng_name);
	}
	
	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_start(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);

	pktengine_start((uint8_t *)pe->eng_name);
	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_show_stats(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);
	
	pktengine_dump_stats((uint8_t *)pe->eng_name);
	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_delete(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);
	
	pktengine_delete((uint8_t *)pe->eng_name);
	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_unlink(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);

	pktengine_unlink_iface((uint8_t *)pe->eng_name, 
			       (uint8_t *)pe->ifname);	

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_stop(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);
	pktengine_stop((uint8_t *)pe->eng_name);

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_drop_pkts(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int rc;
	PktEngine *pe = check_pkteng(L, 1);
	lua_settop(L, 1);

	rc = pktengine_drop_pkts((uint8_t *)pe->eng_name);
	if (rc == -1) {
		TRACE_LOG("Failed to set drop target for engine %s\n",
			  pe->eng_name);
	}	

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static const luaL_reg pkteng_methods[] = {
        {"new",           pkteng_new},
        {"link",          pkteng_link},
	{"open_channel",  pkteng_open_channel},
	{"redirect_pkts", pkteng_redirect_pkts},
        {"start",         pkteng_start},
	{"show_stats",	  pkteng_show_stats},
        {"delete",	  pkteng_delete},
        {"unlink",        pkteng_unlink},
        {"stop",          pkteng_stop},
	{"drop_pkts",	  pkteng_drop_pkts},
	{"help",	  pktengine_help_wrap},
        {0, 0}
};
/*---------------------------------------------------------------------*/
/**
 * Overloaded garbage collector
 */
static int
pkteng_gc(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	TRACE_DEBUG_LOG("Wiping off PktEngine: %p\n", to_pkteng(L, 1));
	TRACE_LUA_FUNC_END();
	UNUSED(L);
	return 0;
}
/*---------------------------------------------------------------------*/
/**
 * Will only work for Lua-5.2
 */
static int
pkteng_tostring(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	char buff[LUA_MAXINPUT];
	PktEngine *pe;
	fprintf(stderr, "Calling %s\n", __FUNCTION__);
	pe = to_pkteng(L, 1);
	sprintf(buff, "PktEngine:\n\tName: %s\n\tType: %s\n\tBatch: %d\n",
		pe->eng_name, pe->type, pe->batch);
	lua_pushfstring(L, "%s", buff);

	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static const luaL_reg pkteng_meta[] = {
        {"__gc",       pkteng_gc},
        {"__tostring", pkteng_tostring},
        {0, 0}
};
/*---------------------------------------------------------------------*/
int
pkteng_register(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	/* create methods table, add it to the globals */
	luaL_openlib(L, "PktEngine", pkteng_methods, 0);
	/* create metatable for PktEngine, & add it to the Lua registry */
	luaL_newmetatable(L, "PktEngine");
	/* fill metatable */
	luaL_openlib(L, 0, pkteng_meta, 0);
	lua_pushliteral(L, "__index");
	/* dup methods table*/
	lua_pushvalue(L, -3);
	/* metatable.__index = methods */
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	/* dup methods table*/
	lua_pushvalue(L, -3);
	/* hide metatable: metatable.__metatable = methods */
	lua_rawset(L, -3);
	/* drop metatable */
	lua_pop(L, 1);
	TRACE_LUA_FUNC_END();
	return 1; /* return methods on the stack */
}
/*---------------------------------------------------------------------*/
static int
luaopen_pkteng(lua_State *L)
{
	/* this loads the pkteng lua interface */
	TRACE_LUA_FUNC_START();

	pkteng_register(L);

	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
int
register_lua_procs(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	/* loads all lua interfaces */
	luaopen_platform(L);
	luaopen_pkteng(L);

	TRACE_LUA_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
