/*
 * Copyright (c) 2014, Asim Jamshed, Robin Sommer, Seth Hall
 * and the International Computer Science Institute. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* for prototypes */
#include "lua_interpreter.h"
/* for logging */
#include "bricks_log.h"
/* for global configuration */
#include "config.h"
/* for string functions */
#include <string.h>
/* for uint?_* types */
#include <sys/types.h>
/* for recv */
#include <sys/socket.h>
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
                } else {
			TRACE_FUNC_END();
			TRACE_ERR("File %s does not exist!\n", fname);
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
do_rshell(lua_State *L, char *rshell_args)
{
	TRACE_LUA_FUNC_START();
	int csock, status, rc;
	char input_line[LUA_MAXINPUT];
	uint32_t input_len;
	char msg_back;

	csock = connect_to_bricks_server(rshell_args);
	input_len = 0;

        while (!stop_processing && (status = loadline(L)) != -1) {
		strcpy(input_line, lua_tostring(L, 1));
		input_len = strlen(input_line);
		input_line[input_len] = REMOTE_LUA_CMD_DELIM;
		/* next send the entire command */ 
		/* also sending the last char as REMOTE_LUA_CMD_DELIM that serves */
		/* as a delimiter */
		rc = write(csock, input_line, input_len + 1);
		if (rc == -1) goto write_error;
		
		/* 
		 * keep on reading till you get 'REMOTE_LUA_CMD_DELIM' delimiter 
		 * or the server closes the socket
		 */
		do {
			rc = read(csock, &msg_back, 1);
			if (rc <= 0 || msg_back == REMOTE_LUA_CMD_DELIM) break;
			rc = write(2, &msg_back, 1);
		} while (1);

		fflush(stdout);
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
void 
process_str_request(lua_State *L, void *argv)
{
	TRACE_LUA_FUNC_START();
	int read_size, total_read, rc;
	char client_msg[LUA_MAXINPUT];
	int client_sock = *((int *)argv);
	total_read = rc = 0;
	
	/* Receive message from client */
	while ((read_size = recv(client_sock, 
				 &client_msg[total_read],
				 LUA_MAXINPUT - total_read, 0)) > 0) {
		total_read += read_size;
		if ((unsigned)(total_read >= LUA_MAXINPUT) || 
		    client_msg[total_read - 1] == REMOTE_LUA_CMD_DELIM) {
			client_msg[total_read - 1] = '\0';
			break;
		}
	}
	/* 
	 * if total_read == 0, 
	 * then this means that the client closed conn. 
	 *
	 * Otherwise process the lua command
	 */
	if (total_read != 0) {
		char delim = REMOTE_LUA_CMD_DELIM;
		register_lua_procs(L);
		load_startup(L);
		setvbuf(stdout, NULL, _IONBF, BUFSIZ);
		/* redirect stdout and stderr to client_sock */
		dup2(client_sock, 1);
		dup2(client_sock, 2);
		/* redirect command output to the client */
		dostring(L, client_msg, "init");
		rc = write(client_sock, &delim, 1);
		/* redirect stdout and stderr back to /dev/null */
		dup2(0, 1);
		dup2(0, 2);
	}
	TRACE_LUA_FUNC_END();
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
		do_rshell(L, argv[1]);
	} else if (!strcmp(argv[0], "script")) {
		register_lua_procs(L);
		TRACE_LOG("Executing %s\n", pv.lua_startup);
		load_startup(L);
		dofile(L, NULL);
	} else if (!strcmp(argv[0], "string")) {
		process_str_request(L, argv[1]);
	}

	TRACE_LUA_FUNC_END();

        return 0;
}
/*---------------------------------------------------------------------*/
/**
 * The lua_kicoff function can run in 4 modes:
 *
 * 1- HOME SHELL mode: This mode is invoked by the non-daemonized version
 * 		       of BRICKS. The system initializes everything before
 *		       control is handed over to the LUA shell. Once the
 *		       terminates, the entire system also shuts down. This
 * 		       mode is only used for testing purposes.
 *
 * 2- REMOTE SHELL mode: This mode works when BRICKS is already running in
 *			daemonized mode. The remote shell is used to connect
 *			to the BRICKS daemon server and send/recv instructions/
 *			output.
 *
 * 3- SCRIPT mode:	This mode is used to only read and load a LUA script
 *			file during BRICKS server initialization
 *
 * 4- STR mode:		This mode is used to execute single line instructions
 *			in the BRICKS server when it is taking instructions from
 *			remote clients (shell)
 */
int
lua_kickoff(LuaMode lm, void *reqptr)
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
		argv[1] = reqptr;
		break;
	case LUA_EXE_SCRIPT:
		argv[0] = "script";
		break;
	case LUA_EXE_STR:
		argv[0] = "string";
		argv[1] = reqptr;
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
/**
 * Function that returns user data blindly. Suprised that this does
 * exist in LUA API... :(
 */
LUALIB_API void *
luaL_optudata(lua_State *L, int ud)
{
	void *p = lua_touserdata(L, ud);
	return p;
}
/*---------------------------------------------------------------------*/
