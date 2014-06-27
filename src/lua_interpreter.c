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
/*---------------------------------------------------------------------*/
extern volatile uint32_t stop_processing;
static const char *progname = "lua";
lua_State *globalL = NULL;
progvars_t pv = {
 pid_file: "/var/run/"PLATFORM_PROMPT".pid",
 lua_startup: NULL,
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
loadline (lua_State *L)
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
        lua_saveline(L, 1);
        lua_remove(L, 1);  /* remove line */

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
        const char *oldprogname = progname;
        progname = NULL;
	
        while (!stop_processing && (status = loadline(L)) != -1) {
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
        progname = oldprogname;
	
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

        register_lua_procs(L);
	
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
                        TRACE_LOG("Executing %s\n", fname);
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
static int
pmain(lua_State *L) {
	TRACE_LUA_FUNC_START();

        struct Smain *s;
        char **argv;
        int script;
        int has_i, has_v, has_e;

        has_i = has_v = has_e = 0;
        script = 0;
        globalL = L;
        s = (struct Smain *)lua_touserdata(L, 1);
        argv = s->argv;

        if (argv[0] && argv[0][0])
                progname = argv[0];
        init_lua(L, s);

        if (s->status != 0) {
		TRACE_LUA_FUNC_END();
		return 0;
	}
        if (has_v) print_version();
        load_startup(L);
        if (has_i)
                dotty(L);
        else if (script == 0 && !has_e && !has_v) {
                if (lua_stdin_is_tty()) {
                        print_version();
                        dotty(L);
                }
                else /* executes stdin as a file */
                        dofile(L, NULL);
        }

	TRACE_LUA_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
int
lua_kickoff(void)
{
	TRACE_LUA_FUNC_START();
	
	int status;
	struct Smain s;
	char *argv[] = {"lua_kickoff"};
	lua_State *L;
	
	/* create state */
	L = lua_open();
	if (L == NULL)
		TRACE_ERR("cannot create state: not enough memory\n");
	
	s.argc = 1;
	s.argv = argv;
	
	TRACE_LOG("%s", "About to call lua_cpcall from lua_kickoff!\n");
	status = lua_cpcall(L, &pmain, &s);
	TRACE_LOG("%s", "Done with lua_cpcall from lua_kickoff!\n");
	report(L, status);
	lua_close(L);
	
	TRACE_LUA_FUNC_END();
	return ((status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS);
}
/*---------------------------------------------------------------------*/
