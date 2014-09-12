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
#ifndef __LUA_INTERPRETER_H__
#define __LUA_INTERPRETER_H__
/*---------------------------------------------------------------------*/
#include "lua_interface.h"

/*
  @@ LUA_PROMPT is the default prompt used by stand-alone Lua.
  @@ LUA_PROMPT2 is the default continuation prompt used by stand-alone Lua.
  ** CHANGE them if you want different prompts. (You can also change the
  ** prompts dynamically, assigning to globals _PROMPT/_PROMPT2.)
  */

/**
 * Shell prompt colors. Suit to your liking...
 */
#define KNRM		"\x1B[0m"
#define KRED		"\x1B[31m"
#define KGRN		"\x1B[32m"
#define KYEL		"\x1B[33m"
#define KBLU		"\x1B[34m"
#define KMAG		"\x1B[35m"
#define KCYN		"\x1B[36m"
#define KWHT		"\x1B[37m"

#define LUA_PROMPT 	KGRN""PLATFORM_PROMPT""KNRM"> "
#define LUA_PROMPT2	KGRN""PLATFORM_PROMPT""KNRM"?>> "



/*
  @@ LUA_MAXINPUT is the maximum length for an input line in the
  @* stand-alone interpreter.
  ** CHANGE it if you need longer lines.
  */
#define LUA_MAXINPUT	512
/*---------------------------------------------------------------------*/
/*
  @@ lua_readline defines how to show a prompt and then read a line from
  @* the standard input.
  @@ lua_saveline defines how to "save" a read line in a "history".
  @@ lua_freeline defines how to free a line read by lua_readline.
  ** CHANGE them if you want to improve this functionality (e.g., by using
  ** GNU readline and history facilities).
  */
#if defined(LUA_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define lua_readline(L,b,p)     ((void)L, ((b)=readline(p)) != NULL)
#define lua_saveline(L,idx)				  \
        if (lua_strlen(L,idx) > 0)  /* non-empty line? */		\
		add_history(lua_tostring(L, idx));  /* add it to history */
#define lua_freeline(L,b)       ((void)L, free(b))
#else
#define lua_readline(L,b,p)					       \
        ((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	 fgets(b, LUA_MAXINPUT, stdin) != NULL)  /* get line */
#define lua_saveline(L,idx)     { (void)L; (void)idx; }
#define lua_freeline(L,b)       { (void)L; (void)b; }
#endif
/*---------------------------------------------------------------------*/
/*
  @@ lua_stdin_is_tty detects whether the standard input is a 'tty' (that
  @* is, whether we're running lua interactively).
  ** CHANGE it if you have a better definition for non-POSIX/non-Windows
  ** systems.
  */
#if defined(LUA_USE_ISATTY)
#include <unistd.h>
#define lua_stdin_is_tty()      isatty(0)
#elif defined(LUA_WIN)
#include <io.h>
#include <stdio.h>
#define lua_stdin_is_tty()      _isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()      1  /* assume stdin is a tty */
#endif
/*---------------------------------------------------------------------*/
typedef struct {
	const char *lua_startup;
	const char *pid_file;
	char *dir;
} progvars_t __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
#define	REMOTE_LUA_CMD_DELIM	127
typedef enum LuaMode {
	LUA_EXE_HOME_SHELL = 0, 
	LUA_EXE_REMOTE_SHELL,
	LUA_EXE_SCRIPT,
	LUA_EXE_STR
} LuaMode;
/**
 * Function to call variants of lua. They can be
 * one of the following: home_shell, remote_shell,
 * script or str. In case of str, the 2nd arg needs
 * to be filled
 */
int
lua_kickoff(LuaMode, void *);

/**
 * Function that returns user data blindly. Suprised that this does
 * exist in LUA API... :(
 */
LUALIB_API void *
luaL_optudata(lua_State *L, int ud);
/*---------------------------------------------------------------------*/
#endif /* __LUA_INTERPRETER_H__ */
