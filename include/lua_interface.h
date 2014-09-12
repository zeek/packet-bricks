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
