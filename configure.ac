AC_INIT(config, version-0.1)

AC_PROG_CC
AC_LANG(C)
AC_DISABLE_OPTION_CHECKING
AC_CHECK_HEADER(linux/sched.h,,,) 
AC_CHECK_HEADER(pthread.h,,,) 
AC_CHECK_LIB(m, cos)
# Check lua-5.1 path (lib & inc)
AC_ARG_WITH(stuff, [  --with-lua-lib 	  path to the liblua5.1 install root])
if test "$with_lua_lib" != ""
then
	AC_SUBST(LUALIBPATH, $with_lua_lib)
else
	if echo `uname` | grep -q "FreeBSD"
	then
		AC_SUBST(LUALIBPATH, /usr/local/lib)
	fi
	if echo `uname` | grep -q "Linux"
	then
		AC_SUBST(LUALIBPATH, /usr/lib)
	fi
fi
AC_ARG_WITH(stuff, [  --with-lua-include	  path to the lua5.1 header install root])
if test "$with_lua_include" != ""
then
	AC_SUBST(LUAINCPATH, $with_lua_include)
else
	if echo `uname` | grep -q "FreeBSD"
	then
		AC_SUBST(LUAINCPATH, /usr/local/include/lua51)
	fi
	if echo `uname` | grep -q "Linux"
	then
		AC_SUBST(LUAINCPATH, /usr/include/lua5.1)
	fi
fi

a=1
# Check whether lua-5.1 exists
AC_CHECK_HEADER($LUAINCPATH/lua.h,[AC_DEFINE(HAVE_LUA_H, 1)],[a=0],
[[
#if HAVE_LUA_H
#include <lua5.1/lua.h>
#endif
]])
if test $a == 0
then
	AC_MSG_ERROR([lua.h is missing. Please install lua-5.1 development library.])
fi

# Check if the user wants to use libjemalloc
AC_ARG_WITH(stuff, [  --with-jemalloc-include	  path to the jemalloc header install root])
if test "$with_jemalloc_include" != ""
then
	AC_SUBST(JEINCPATH, $with_jemalloc_include)
fi

# Check if the path to to libjemalloc is valid
a=1
# Check whether lua-5.1 exists
AC_CHECK_HEADER($JEINCPATH/jemalloc.h,[AC_DEFINE(HAVE_JEMALLOC_H, 1)],[a=0],
[[
#if HAVE_JEMALLOC_H
#include <jemalloc/jemalloc.h>
#endif
]])
if test $a == 0
then
	AC_MSG_WARN([jemalloc.h not found.])
fi

AC_OUTPUT(Makefile)
