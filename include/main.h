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
#ifndef __MAIN_H__
#define __MAIN_H__
/*---------------------------------------------------------------------*/
/**
 * This is the main header file for main.c
 * For now, it contains the struct definition
 * that contains global variables of the system
 */
/*---------------------------------------------------------------------*/
/**
 * PacInfo stuct - contains data that is picked up from the command-line
 *		 - and filled in this struct. Batch size gets the default
 *		 - number of packets that needs to be picked up from the
 *		 - interface at one time. lua_startup file contains the
 *		 - the LUA script that will be interpreted by bricks...
 *		 - ...plus a few others
 */
typedef struct PacInfo {
	uint16_t batch_size; /* read these many packets per read */
	const unsigned char *lua_startup_file; /* path to lua startup file 
						  given at cmd line */	
	uint8_t daemonize; /* do we have to daemonize the process? */
	uint8_t rshell; 	/* do we have to make a remote shell? */
	int8_t *rshell_args; /* remote shell args: "ipaddr:port" */
} BricksInfo __attribute__((aligned(__WORDSIZE)));

extern BricksInfo pc_info;
/*---------------------------------------------------------------------*/
//#define PASSIVE_MODE			1
#define DEFAULT_BATCH_SIZE		512
#define PAUSE_PERIOD			1 /* in secs */
#define FILE_PRINT_TIMER		5 /* in secs */
/*---------------------------------------------------------------------*/
#endif /* !__MAIN_H__ */
