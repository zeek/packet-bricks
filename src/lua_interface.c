/* for header definitions */
#include "lua_interface.h"
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
	/* this prints the system's current status */
	TRACE_LUA_FUNC_START();
	UNUSED(L);
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
		"    open_channel( {pid=<pid>} ) <DISABLED>\n"
		"    add_filter(<params still need to be determined>) <DISABLED>\n"
		"    new( {name=<ioengine_name>, type=<io_type>, [cpu=<cpu number>]} )\n"
		"    delete( {engine=<ioengine_name>} )\n"
		"    link( {engine=<ioengine_name>, ifname=<interface>, [batch=<chunk_size>}] )\n"
		"    unlink( {engine=<ioengine_name>, ifname=<interface>} )\n"
		"    start( {engine=<ioengine_name>} )\n"
		"    stop( {engine=<ioengine_name>} )\n"
		"    show_stats( {engine=<ioengine_name>} )\n"
		);
	UNUSED(L);
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_new_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	const unsigned char *name;
	const unsigned char *type;
	int cpu = -1;

	/* this will load netmap module */
	luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "name");
        name = (unsigned char *)lua_tostring(L, -1);
        lua_getfield(L, 1, "type");
	type = (unsigned char *)lua_tostring(L, -1);
        lua_getfield(L, 1, "cpu");
        if (lua_isnumber(L, -1))
		cpu = lua_tointeger(L, -1);
        lua_remove(L, -1);


        if (name == NULL) {
		TRACE_LOG("pktengine_new: Engine name not specified!\n");
                lua_remove(L, -1);
		TRACE_LUA_FUNC_END();
                return 0;
        }
	if (type == NULL) {
		TRACE_LOG("pktengine_new: pkt library name not specified!\n");
                lua_remove(L, -1);
		TRACE_LUA_FUNC_END();
                return 0;
	}

	TRACE_DEBUG_LOG("Pkt engine name is: %s, type is %s,"
			" and cpu is: %u\n",
			name, type, cpu);
        pktengine_new(name, type, cpu);
        lua_remove(L, -1);
        lua_remove(L, -1);

	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_delete_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	unsigned char *name;

	/* this will unload netmap module */
	luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "engine");
        name = (unsigned char *)lua_tostring(L, -1);

	if (name == NULL) {
		TRACE_LOG("Engine with %s does not exist\n", name);
		TRACE_LUA_FUNC_END();
		return -1;
	}
	
	TRACE_DEBUG_LOG("Pkt with engine name %s is deleted\n",
			name);
	pktengine_delete(name);
	lua_remove(L, -1);
	
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_link_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	unsigned char *name;
	unsigned char *ifname;
	int batch_size;

	/* set initial batch size to -1 */
	batch_size = -1;

	/* this configures iface */
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "engine");
	name = (unsigned char *)lua_tostring(L, -1);
	if (name == NULL) {
		TRACE_LOG("Engine name is absent\n");
		TRACE_LUA_FUNC_END();
		return -1;
	}
    
        lua_getfield(L, 1, "ifname");
        ifname = (unsigned char *)lua_tostring(L, -1);
	if (ifname == NULL) {
		TRACE_LOG("Engine %s needs an interface to link to\n", name);
		lua_remove(L, -1);
		TRACE_LUA_FUNC_END();
		return -1;
	}
        lua_getfield(L, 1, "batch");
        if (lua_isnumber(L, -1))
		batch_size = lua_tointeger(L, -1);
        lua_remove(L, -1);

	TRACE_DEBUG_LOG("Pkt engine name is: %s, interface name is: %s, "
			"and batch_size is: %d\n",
			name, ifname, batch_size);
	pktengine_link_iface(name, ifname, batch_size);
	
	lua_remove(L, -1);
	lua_remove(L, -1);
	
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_unlink_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	/* this will remove iface */
	unsigned char *name;
	unsigned char *ifname;
	
	/* this configures iface */
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "engine");
	name = (unsigned char *)lua_tostring(L, -1);
	if (name == NULL) {
		TRACE_LOG("Engine needs a name\n");
		TRACE_LUA_FUNC_END();
		return -1;
	}
        lua_getfield(L, 1, "ifname");
        ifname = (unsigned char *)lua_tostring(L, -1);
	if (ifname == NULL) {
		TRACE_LOG("Engine %s needs an interface\n", name);
		TRACE_LUA_FUNC_END();
		return -1;
	}
	
	TRACE_DEBUG_LOG("Pkt engine name is: %s " 
			"and interface name is: %s\n",
			name, ifname);	
	pktengine_unlink_iface(name, ifname);	
	lua_remove(L, -1);
	lua_remove(L, -1);
	
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_start_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	unsigned char *name;
	/* this will start netmap engine */
	luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "engine");
        name = (unsigned char *)lua_tostring(L, -1);

	if (name == NULL) {
		TRACE_LOG("Engine needs a name\n");
		TRACE_LUA_FUNC_END();
		return -1;
	}
	pktengine_start(name);
        lua_remove(L, -1);

	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_stop_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	/* this will stop netmap engine */
	unsigned char *name;

	luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "engine");
        name = (unsigned char *)lua_tostring(L, -1);
	if (name == NULL) {
		TRACE_LOG("Engine needs a name\n");
		TRACE_LUA_FUNC_END();
		return -1;
	}
	pktengine_stop(name);       	
	lua_remove(L, -1);

	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int
pktengine_dump_stats_wrap(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	/* this will show per link statistics */
     	unsigned char *name;
	luaL_checktype(L, 1, LUA_TTABLE);
        lua_getfield(L, 1, "engine");
        name = (unsigned char *)lua_tostring(L, -1);

	if (name == NULL) {
		TRACE_LOG("Engine needs a name\n");
		TRACE_LUA_FUNC_END();
		return -1;
	}
	pktengine_dump_stats(name);
	lua_remove(L, -1);

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
        {"new", pktengine_new_wrap},
        {"delete", pktengine_delete_wrap},
        {"link", pktengine_link_wrap},
	{"unlink", pktengine_unlink_wrap},
	{"start", pktengine_start_wrap},
        {"stop", pktengine_stop_wrap},
        {"show_stats", pktengine_dump_stats_wrap},
        {NULL, NULL}
};
/*---------------------------------------------------------------------*/
static int
luaopen_pkteng(lua_State *L)
{
	/* this loads the pkteng lua interface */
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

	/* loads all lua interfaces */
	luaopen_platform(L);
	luaopen_pkteng(L);

	TRACE_LUA_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
