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
/* for DEFAULT_BATCH_SIZE */
#include "main.h"
/*---------------------------------------------------------------------*/
extern volatile uint32_t stop_processing;
#define METATABLE		PLATFORM_NAME" Metatable"
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
		"    PktEngine \n"
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
	/* this prints the system-wide statitics */
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
	/* assigning this to 1 shuts down the lua shell */
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
pktengine_help(lua_State *L)
{
	/* prints pktengine help sub-menu */
	TRACE_LUA_FUNC_START();
	fprintf(stdout, "Packet Engine Commands:\n"
		"    help()\n"
		"    new(<ioengine_name>, <io_type>, <cpu number>)\n"
		"    delete()\n"
		"    link(<linker>, <chunk_size>, <qid>)\n"
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
static PktEngine_Intf *
to_pkteng(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	/* fetches pktengine object param from lua stack */
	PktEngine_Intf *pe = (PktEngine_Intf *)lua_touserdata(L, index);
	if (pe == NULL) luaL_typerror(L, index, "PktEngine");
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static PktEngine_Intf *
push_pkteng(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	/* pushes pktengine object to the lua stack */
	PktEngine_Intf *pe = (PktEngine_Intf *)lua_newuserdata(L, sizeof(PktEngine_Intf));
	luaL_getmetatable(L, "PktEngine");
	lua_setmetatable(L, -2);
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static PktEngine_Intf *
check_pkteng(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	PktEngine_Intf *pe;
	/* see if the param data type is actually PktEngine */
	luaL_checktype(L, index, LUA_TUSERDATA);
	pe = (PktEngine_Intf *)luaL_checkudata(L, index, "PktEngine");
	if (pe == NULL) luaL_typerror(L, index, "PktEngine");
	TRACE_LUA_FUNC_END();
	return pe;
}
/*---------------------------------------------------------------------*/
static int
pkteng_new(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int nargs = lua_gettop(L);
	const char *ename = luaL_optstring(L, 1, 0);
	const char *type = luaL_optstring(L, 2, 0);
	int cpu = -1;
	/* only grab cpu metric if it is mentioned */
	if (nargs == 3)
		cpu = luaL_optint(L, 3, 0);

	/* parse and populate the remaining fields */
	PktEngine_Intf *pe = push_pkteng(L);
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
	int i, rc;
	PktEngine_Intf *pe = check_pkteng(L, 1);
	Linker_Intf *linker;
	const char *channel_type = NULL;

	int nargs = lua_gettop(L);

	/* check if args2 is user-data */
	luaL_checktype(L, 2, LUA_TUSERDATA);
	
	/* Retrieve linker data */
	linker = (Linker_Intf *)luaL_optudata(L, 2);
	if (linker == NULL) luaL_typerror(L, 2, "LoadBalancer");

	if (linker->type == LINKER_LB) {
		linker = (Linker_Intf *)luaL_checkudata(L, 2, "LoadBalancer");
		channel_type = "SHARE";
	} else if (linker->type == LINKER_DUP) {
		linker = (Linker_Intf *)luaL_checkudata(L, 2, "Duplicator");
		channel_type = "COPY";
	} else
		luaL_typerror(L, 2, "LoadBalancer");

	/* set values as default */
	pe->ifname = linker->input_link;
	pe->batch = DEFAULT_BATCH_SIZE;
	pe->qid = -1;

	/* if 3rd arg is passed, fill it with batch size */
	if (nargs >= 3)
		pe->batch = luaL_checkint(L, 3);
	/* if 4th arg is passed, fill it with qid */
	if (nargs >= 4)
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
	
	/* link the source with the packet engine */
	pktengine_link_iface((uint8_t *)pe->eng_name, 
			     (uint8_t *)pe->ifname, 
			     pe->batch, pe->qid);
	
	/* ... and now link all the channels */
	for (i = 0; i < linker->output_count; i++) {
		rc = pktengine_open_channel((uint8_t *)pe->eng_name, 
					    (uint8_t *)pe->ifname,
					    (uint8_t *)linker->output_link[i], 
					    (uint8_t *)channel_type);
		if (rc == -1) {
			TRACE_LOG("Failed to open channel %s\n", 
				  linker->output_link[i]);
		}
	}

	/* if there are pipelines, link them as well */
	while (linker->next_linker != NULL) {
		linker = linker->next_linker;
		for (i = 0; i < linker->output_count; i++) {
			rc = pktengine_open_channel((uint8_t *)pe->eng_name,
						    (uint8_t *)linker->input_link,
						    (uint8_t *)linker->output_link[i],
						    (uint8_t *)((linker->type == LINKER_LB) ? 
						     "SHARE" : "COPY"));
			if (rc == -1) {
				TRACE_LOG("Failed to open channel %s\n",
					  linker->output_link[i]);
			}
		}
	}

	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
pktengine_retrieve(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	const char *ename = luaL_optstring(L, 1, 0);
	engine *e = engine_find((unsigned char *)ename);
	if (e != NULL) {
		PktEngine_Intf *pe = push_pkteng(L);
		pe->eng_name = ename;
		pe->cpu = e->cpu;
		pe->ifname = (char *)e->link_name;
	}
	
	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
static int
pkteng_start(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	PktEngine_Intf *pe = check_pkteng(L, 1);
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
	PktEngine_Intf *pe = check_pkteng(L, 1);
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
	PktEngine_Intf *pe = check_pkteng(L, 1);
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
	PktEngine_Intf *pe = check_pkteng(L, 1);
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
	PktEngine_Intf *pe = check_pkteng(L, 1);
	lua_settop(L, 1);
	pktengine_stop((uint8_t *)pe->eng_name);

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static const luaL_reg pkteng_methods[] = {
        {"new",           pkteng_new},
        {"link",          pkteng_link},
        {"start",         pkteng_start},
	{"show_stats",	  pkteng_show_stats},
        {"delete",	  pkteng_delete},
        {"unlink",        pkteng_unlink},
        {"stop",          pkteng_stop},
	{"help",	  pktengine_help},
	{"retrieve",	  pktengine_retrieve},
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
	PktEngine_Intf *pe;
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
/**
 * Linker interface :P
 */
/*---------------------------------------------------------------------*/
static int
linker_help(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	fprintf(stdout, "LoadBalance/Duplicator Commands:\n"
		"    help()\n"
		"    new([<split-mode>])\n"
		"    connect_input(<interfaces>)\n"
	     	"    connect_output(<interfaces>)\n"
		);
	UNUSED(L);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static Linker_Intf *
to_linker(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker = (Linker_Intf *)lua_touserdata(L, index);
	if (linker == NULL) luaL_typerror(L, index, "LoadBalancer");
	TRACE_LUA_FUNC_END();
	return linker;
}
/*---------------------------------------------------------------------*/
static Linker_Intf *
push_linker(lua_State *L, int type)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker = (Linker_Intf *)lua_newuserdata(L, sizeof(Linker_Intf));
	luaL_getmetatable(L, (type == LINKER_LB) ? "LoadBalancer": "Duplicator");
	lua_setmetatable(L, -2);
	TRACE_LUA_FUNC_END();
	return linker;
}
/*---------------------------------------------------------------------*/
static Linker_Intf *
check_linker(lua_State *L, int index)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker;

	/* Retrieve linker */
	luaL_checktype(L, index, LUA_TUSERDATA);
	linker = (Linker_Intf *)luaL_optudata(L, index);
	
	if (linker == NULL) luaL_typerror(L, index, "LoadBalancer");
	if (linker->type == LINKER_LB)
		linker = (Linker_Intf *)luaL_checkudata(L, index, "LoadBalancer");
	else if (linker->type == LINKER_DUP)
		linker = (Linker_Intf *)luaL_checkudata(L, index, "Duplicator");
	else
		luaL_typerror(L, index, "LoadBalancer");

	TRACE_LUA_FUNC_END();
	return linker;
}
/*---------------------------------------------------------------------*/
static int
linker_new(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int type = LINKER_DUP;
	int hash_split = -1;
	int nargs = lua_gettop(L);
	
	if (nargs == 1) {
		type = LINKER_LB;
		hash_split = luaL_optint(L, 1, 0);
	}
	Linker_Intf *linker = push_linker(L, type);
	linker->type = type;
	linker->hash_split = hash_split;
	linker->output_count = 0;
	linker->next_linker = NULL;
	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static int
linker_input(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker;
	const char *input;

	linker = check_linker(L, 1);
	input = luaL_optstring(L, 2, 0);	

	linker->input_link = input;
	lua_settop(L, 1);

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
linker_output(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker;
	int nargs = lua_gettop(L);
	int i;

	linker = check_linker(L, 1);
	for (i = 2; i <= nargs; i++) {
		linker->output_link[linker->output_count] = 
			luaL_optstring(L, i, 0);
		linker->output_count++;
	}
	lua_settop(L, 1);

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static int
linker_link(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	Linker_Intf *linker, *linker_iter;
	int nargs = lua_gettop(L);
	int i;

	linker = check_linker(L, 1);
	for (i = 2; i <= nargs; i++) {
		linker_iter = check_linker(L, i);
		linker_iter->next_linker = linker->next_linker;
		linker->next_linker = linker_iter;
	}
	lua_settop(L, 1);

	TRACE_LUA_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
static const luaL_reg linker_methods[] = {
        {"new",           	linker_new},
        {"connect_input",	linker_input},
	{"connect_output",	linker_output},
	{"link",		linker_link},
	{"help",		linker_help},
        {0, 0}
};
/*---------------------------------------------------------------------*/
/**
 * Overloaded garbage collector
 */
static int
linker_gc(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	TRACE_DEBUG_LOG("Wiping off Linker: %p\n", to_linker(L, 1));
	TRACE_LUA_FUNC_END();
	UNUSED(L);
	return 0;
}
/*---------------------------------------------------------------------*/
/**
 * Will only work for Lua-5.2
 */
static int
linker_tostring(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	char buff[LUA_MAXINPUT];
	Linker_Intf *linker;
	fprintf(stderr, "Calling %s\n", __FUNCTION__);
	linker = to_linker(L, 1);
	sprintf(buff, (linker->type == 1) ? "LoadBalancer" : "Duplicator"
		":\n\tSource: %s\n",
		linker->input_link);
	lua_pushfstring(L, "%s", buff);

	TRACE_LUA_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
static const luaL_reg linker_meta[] = {
        {"__gc",       linker_gc},
        {"__tostring", linker_tostring},
        {0, 0}
};
/*---------------------------------------------------------------------*/
int
loadbalancer_register(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	/* create methods table, add it to the globals */
	luaL_openlib(L, "LoadBalancer", linker_methods, 0);
	/* create metatable for LoadBalancer, & add it to the Lua registry */
	luaL_newmetatable(L, "LoadBalancer");
	/* fill metatable */
	luaL_openlib(L, 0, linker_meta, 0);
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
int
duplicator_register(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	/* create methods table, add it to the globals */
	luaL_openlib(L, "Duplicator", linker_methods, 0);
	/* create metatable for Duplicator, & add it to the Lua registry */
	luaL_newmetatable(L, "Duplicator");
	/* fill metatable */
	luaL_openlib(L, 0, linker_meta, 0);
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
luaopen_linker(lua_State *L)
{
	/* this loads the pkteng lua interface */
	TRACE_LUA_FUNC_START();

	loadbalancer_register(L);
	duplicator_register(L);

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
	luaopen_linker(L);

	TRACE_LUA_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
