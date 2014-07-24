/* for prototypes */
#include "lua_interpreter.h"
/* for logging */
#include "pacf_log.h"
/* for global configuration */
#include "config.h"
/* for string functions */
#include <string.h>
/* for uint?_* types */
#include <sys/types.h>
#include <stdint.h>
/* for file stat */
#include <sys/stat.h>
/* for basic configuration */
#include "config.h"
/* for server connection */
#include "backend.h"
/* for write()/close() fns */
#include <unistd.h>
/*---------------------------------------------------------------------*/
extern volatile uint32_t stop_processing;
lua_State *globalL = NULL;
progvars_t pv = {
	.pid_file = "/var/run/"PLATFORM_PROMPT".pid",
	.lua_startup = NULL,
};
/*---------------------------------------------------------------------*/
struct Smain
{
	int argc;
	char **argv;
	int status;
};
/*---------------------------------------------------------------------*/
static int
traceback(lua_State *L)
{
	TRACE_LUA_FUNC_START();

	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		TRACE_LUA_FUNC_END();
		return 1;
	}
	lua_getfield(L, -1, "traceback");
        if (!lua_isfunction(L, -1)) {
                lua_pop(L, 2);
		TRACE_LUA_FUNC_END();
                return 1;
        }
        /* pass error message */
        lua_pushvalue(L, 1);
        /* skip this function & traceback */
        lua_pushinteger(L, 2);
        /* call debug.traceback */
        lua_call(L, 2, 1);

	TRACE_LUA_FUNC_END();	
        return 1;	
}
/*---------------------------------------------------------------------*/
static const char *
get_prompt(lua_State *L, int firstline)
{
	TRACE_LUA_FUNC_START();

        const char *p;
        lua_getfield(L, LUA_GLOBALSINDEX,
                     firstline ? "_PROMPT" : "_PROMPT2");
        p = lua_tostring(L, -1);
        if (p == NULL)
                p = (firstline ? LUA_PROMPT : LUA_PROMPT2);
        /* remove global */
        lua_pop(L, 1);
	
	TRACE_LUA_FUNC_END();
        return p;
}
/*---------------------------------------------------------------------*/
static int
incomplete(lua_State *L, int status)
{
	TRACE_LUA_FUNC_START();

        if (status == LUA_ERRSYNTAX) {
                size_t lmsg;
                const char *msg = lua_tolstring(L, -1, &lmsg);
                const char *tp = msg + lmsg -
                        (sizeof(LUA_QL("<eof>")) - 1);
                if (strstr(msg, LUA_QL("<eof>")) == tp) {
                        lua_pop(L, 1);
			TRACE_LUA_FUNC_END();
                        return 1;
                }
        }

	TRACE_LUA_FUNC_END();
        return 0;  /* else... */
}
/*---------------------------------------------------------------------*/
static int
pushline(lua_State *L, int firstline)
{
	TRACE_LUA_FUNC_START();

        char buffer[LUA_MAXINPUT];
        char *b = buffer;
        size_t l;
        const char *prmt = get_prompt(L, firstline);
        if (lua_readline(L, b, prmt) == 0) {
		TRACE_LUA_FUNC_END();
                return 0;  /* no input */
	}
        l = strlen(b);
        /* line ends with newline? */
        if (l > 0 && b[l-1] == '\n')
                b[l-1] = '\0';  /* remove it */

        /* first line starts with `=' ? */
        if (firstline && b[0] == '=')
                lua_pushfstring(L, "return %s", b+1);  /* change it to `return' */
        else
                lua_pushstring(L, b);
        lua_freeline(L, b);

	TRACE_LUA_FUNC_END();
        return 1;
}
/*---------------------------------------------------------------------*/
static int
loadline(lua_State *L)
{
	TRACE_LUA_FUNC_START();

        int status;
        lua_settop(L, 0);
        if (!pushline(L, 1)) {
		TRACE_LUA_FUNC_END();
                return -1;  /* no input */
	}
        for (;;) {  /* repeat until gets a complete line */
                status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
                if (!incomplete(L, status)) break;  /* cannot try to add lines? */
                if (!pushline(L, 0))  {/* no more input? */
			TRACE_LUA_FUNC_END();
                        return -1;
		}
                lua_pushliteral(L, "\n");  /* add a new line... */
                lua_insert(L, -2);  /* ...between the two lines */
                lua_concat(L, 3);  /* join them */
        }
	TRACE_DEBUG_LOG("%s\n", lua_tostring(L, 1));
	TRACE_LUA_FUNC_END();
        return status;
}
/*---------------------------------------------------------------------*/
static int
docall(lua_State *L, int narg, int clear)
{
	TRACE_LUA_FUNC_START();

        int status;
        int base;

        /* function index */
        base = lua_gettop(L) - narg;
        /* push traceback function */
        lua_pushcfunction(L, traceback);
        /* put it under chunk & args */
        lua_insert(L, base);
        status = lua_pcall(L, narg,
                           (clear ? 0 : LUA_MULTRET),
                           base);
        /* remove traceback function */
        lua_remove(L, base);
        /* force a complete garbage collection in case of errors */
        if (status != 0)
                lua_gc(L, LUA_GCCOLLECT, 0);

	TRACE_LUA_FUNC_END();
        return status;
}
/*---------------------------------------------------------------------*/
static int
report(lua_State *L, int status)
{
	TRACE_LUA_FUNC_START();

        if (status && !lua_isnil(L, -1)) {
                const char *msg = lua_tostring(L, -1);
                if (msg == NULL) msg = "(error object is not a string)";
		TRACE_LOG("%s", msg);
                lua_pop(L, 1);
        }

	TRACE_LUA_FUNC_END();

        return status;
}
/*---------------------------------------------------------------------*/
static void
dotty(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	
        int status;
	
        while (!stop_processing && (status = loadline(L)) != -1) {
		lua_saveline(L, 1);
		lua_remove(L, 1);  /* remove line */
                if (status == 0) status = docall(L, 0, 0);
                report(L, status);
                if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
                        lua_getglobal(L, "print");
                        lua_insert(L, 1);
                        if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
                                TRACE_LOG("%s", lua_pushfstring
					      (L,
					       "error calling " LUA_QL("print") " (%s)",
					       lua_tostring(L, -1)));
                }
        }
        lua_settop(L, 0);  /* clear stack */
        TRACE_FLUSH();
	
	TRACE_LUA_FUNC_END();
}
/*---------------------------------------------------------------------*/
static int
dofile(lua_State *L, const char *name)
{
	TRACE_LUA_FUNC_START();
        int status;

        status = luaL_loadfile(L, name) ||
                docall(L, 0, 1);

	TRACE_LUA_FUNC_END();
	return report(L, status);
}
/*---------------------------------------------------------------------*/
static int
dostring(lua_State *L, const char *s, const char *name)
{
	TRACE_LUA_FUNC_START();
        int status;

        status = luaL_loadbuffer(L, s, strlen(s), name) ||
                docall(L, 0, 1);
	
	TRACE_LUA_FUNC_END();
        return report(L, status);
}
/*---------------------------------------------------------------------*/
static int
handle_luainit(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	
        const char *init = getenv(LUA_INIT);
	
        if (init == NULL) {
		TRACE_LUA_FUNC_END();
		return 0; /* status OK */
	}
        else if (init[0] == '@') {
		TRACE_LUA_FUNC_END();
                return dofile(L, init+1);
	} else
                dostring(L, init, "=" LUA_INIT);
	
	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static void
init_lua(lua_State *L, struct Smain *s)
{
	TRACE_LUA_FUNC_START();

        /* stop collector during initialization */
        lua_gc(L, LUA_GCSTOP, 0);
        /* open libraries */
        luaL_openlibs(L);
        lua_gc(L, LUA_GCRESTART, 0);
        s->status = handle_luainit(L);

	TRACE_LUA_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
load_startup(lua_State *L)
{
	TRACE_LUA_FUNC_START();

        struct stat st;
        const char *fname;
        fname = pv.lua_startup;

        if (fname != NULL) {
                if(stat(fname, &st) != -1) {
                        dofile(L, fname);
                }
        }

	TRACE_LUA_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
print_version(void)
{
	TRACE_FUNC_START();

	TRACE_LOG(PLATFORM_NAME" Version %s\n",
		  VERSION);
	TRACE_FLUSH();
	
	TRACE_LUA_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
do_rshell(lua_State *L)
{
	TRACE_LUA_FUNC_START();
	int csock, status, rc;
	char input_line[LUA_MAXINPUT];
	uint32_t input_len;

	csock = connect_to_pacf_server();
	input_len = 0;

        while (!stop_processing && (status = loadline(L)) != -1) {
		strcpy(input_line, lua_tostring(L, 1));
		input_len = strlen(input_line);

		/* next send the entire command */ 
		/* also sending the last '\0' char that serves */
		/* as a delimiter */
		rc = write(csock, input_line, input_len + 1);
		if (rc == -1) goto write_error;
		
		lua_saveline(L, 1);
		lua_remove(L, 1);  /* remove line */
        }
        lua_settop(L, 0);  /* clear stack */
        TRACE_FLUSH();
	
	TRACE_LUA_FUNC_END();
	close(csock);
	return;

 write_error:
	TRACE_ERR("Failed to send message (%s) of len: %d to server!\n",
		  input_line, input_len);
}
/*---------------------------------------------------------------------*/
static int
pmain(lua_State *L) {
	TRACE_LUA_FUNC_START();

        struct Smain *s;
        char **argv;

        globalL = L;
        s = (struct Smain *)lua_touserdata(L, 1);
        argv = s->argv;

        init_lua(L, s);

        if (s->status != 0) {
		TRACE_LUA_FUNC_END();
		return 0;
	}

        if (!strcmp(argv[0], "home-shell")) {
		register_lua_procs(L);
		TRACE_LOG("Executing %s\n", pv.lua_startup);
		load_startup(L);
		if (lua_stdin_is_tty()) {
			print_version();
			dotty(L);
		}
	} else if (!strcmp(argv[0], "remote-shell")) {
		print_version();
		do_rshell(L);
	} else if (!strcmp(argv[0], "script")) {
		register_lua_procs(L);
		TRACE_LOG("Executing %s\n", pv.lua_startup);
		load_startup(L);
		dofile(L, NULL);
	} else if (!strcmp(argv[0], "string")) {
		register_lua_procs(L);
		load_startup(L);
		dostring(L, argv[1], "init");
	}

	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
int
lua_kickoff(LuaMode lm, char *str)
{
	TRACE_LUA_FUNC_START();
	
	int status;
	struct Smain s;
	char *argv[2];
	lua_State *L;
	
	switch (lm) {
	case LUA_EXE_HOME_SHELL:
		argv[0] = "home-shell";
		break;
	case LUA_EXE_REMOTE_SHELL:
		argv[0] = "remote-shell";
		break;
	case LUA_EXE_SCRIPT:
		argv[0] = "script";
		break;
	case LUA_EXE_STR:
		argv[0] = "string";
		argv[1] = str;
		break;
	default:
		/* control will never come here */
		TRACE_ERR("Wrong choice entered!\n");
	}

	/* create state */
	L = lua_open();
	if (L == NULL)
		TRACE_ERR("cannot create state: not enough memory\n");
	
	s.argc = 2;
	s.argv = argv;
	
	status = lua_cpcall(L, &pmain, &s);
	report(L, status);
	lua_close(L);
	
	TRACE_LUA_FUNC_END();
	return ((status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS);
}
/*---------------------------------------------------------------------*/
