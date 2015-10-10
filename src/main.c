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
/* for stdio and stdlib */
#include "bricks_log.h"
/* for filter interface */
#include "bricks_interface.h"
/* for pc_info definition */
#include "main.h"
/* for errno */
#include <errno.h>
/* for string funcs */
#include <string.h>
/* for getopt() */
#include <unistd.h>
/* for init'ing the engine list */
#include "pkt_engine.h"
/* for init'ing the network interface list */
#include "network_interface.h"
/* for bricks interface funcs (start_listening_reqs()) */
#include "bricks_interface.h"
/* for lua variables access */
#include "lua_interpreter.h"
/* for signal handling */
#include <signal.h>
/* for config names */
#include "config.h"
/* for file open */
#include <sys/types.h>
/* for file open */
#include <sys/stat.h>
/* for file open */
#include <fcntl.h>
/*---------------------------------------------------------------------*/
/* program variable for lua */
extern progvars_t pv;
/* global variable to indicate start/stop for lua interpreter */
volatile uint32_t stop_processing = 0;

/* global variable definitions */
BricksInfo pc_info = {
	.batch_size 		= DEFAULT_BATCH_SIZE,
	.lua_startup_file 	= NULL,
	.daemonize		= 0,
	.rshell			= 0,
	.rshell_args		= NULL
};
/*---------------------------------------------------------------------*/
/**
 * Program termination point
 * This function will free up all the previously 
 * allocated resources.
 */
inline void
clean_exit(int exit_val)
{
	TRACE_FUNC_START();
	/* free up the start_lua_file_name (if reqd.)*/
	if (pc_info.lua_startup_file != NULL)
		free((unsigned char *)pc_info.lua_startup_file);

	if (pc_info.rshell_args != NULL)
		free(pc_info.rshell_args);
	if (!pc_info.daemonize) fprintf(stdout, "Goodbye!\n");
	TRACE_FUNC_END();
	exit(exit_val);

}
/*---------------------------------------------------------------------*/
/**
 * Prints the help menu 
 */
static inline void
print_help(char *progname)
{
	TRACE_FUNC_START();
	fprintf(stdout, "Usage: %s [-d|-s] [-b batch_size] "
		"[-f start_lua_script_file] [-h]\n", progname);
	
	TRACE_FUNC_END();
	clean_exit(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------*/
/**
 * Initializes all the modules of the system
 */
static inline void
init_modules()
{
	TRACE_FUNC_START();

	TRACE_DEBUG_LOG("Initializing the engines module \n");
	pktengine_init();
	interface_init();
	initBricks();

	TRACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Loads lua start-up file
 */
static void
load_lua_file(const char *lua_file)
{
	TRACE_FUNC_START();
	
	pv.lua_startup = strdup(lua_file);
	if (pv.lua_startup == NULL) {
		TRACE_ERR("Can't strdup for lua_startup_file!\n");
	}

	pc_info.lua_startup_file = (const unsigned char *)pv.lua_startup;
	TRACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Creates a .pid file that not only holds the current process
 * ID of the process but also secures a system wide lock so that
 * a duplicate daemon can not be executed.
 */
static int32_t
mark_pid_file(const char *pid_file)
{
	TRACE_FUNC_START();
	int fd;
	struct flock lock;
	int rc;
	char pid_buffer[12];

	if ((fd = open(pid_file, O_CREAT | O_WRONLY,
		       S_IRUSR | S_IWUSR | S_IRGRP |
		       S_IROTH)) == -1)
		return errno;

	/* pid file locking */
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;

	if (fcntl(fd, F_SETLK, &lock) == -1) {
		if (errno == EACCES || errno == EAGAIN) {
			rc = -1;
		} else {
			rc = -1;
		}
		close(fd);
		return rc;
	}

	snprintf(pid_buffer, sizeof(pid_buffer), "%d\n", (int)getpid());
	rc = ftruncate(fd, 0);
	rc = write(fd, pid_buffer, strlen(pid_buffer));

	TRACE_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
/*
 * Daemonize the process
 */
static int32_t
do_daemonize()
{
	TRACE_FUNC_START();
	pid_t pid;
	int fd;

	pid = fork();
	if (pid < 0) {
		TRACE_FUNC_END();
		TRACE_ERR("Can't daemonize. Could not fork!\n");
	}
	if (pid == 0) {
		if ((fd =
#ifdef DEBUG 
		     open(FILE_DEBUG, O_CREAT | O_TRUNC | O_RDWR)
#else
		     open(FILE_IGNORE, O_RDWR)
#endif
		     ) >= 0) {
			dup2(fd, 0);
			dup2(fd, 1);
			dup2(fd, 2);
			if (fd > 2) close(fd);
		}
	}
	TRACE_FUNC_END();
	return pid;
}
/*---------------------------------------------------------------------*/
/**
 * XXX - A rudimentary method to display file contents. This may be revised...
 */
static inline void
status_print(int sig)
{
	TRACE_FUNC_START();
	FILE *f;
	char status_fname[FILENAME_MAX];

	sprintf(status_fname, "/var/run/%s.status", PLATFORM_PROMPT);
	f = fopen(status_fname, "w+");
	if (f == NULL) {
		TRACE_ERR("Failed to create status file!\n");
		TRACE_FUNC_END();
	}
	
	/* only print status if engines are online */
	if (is_pktengine_online((unsigned char *)"any")) {
		pktengines_list_stats(f);
	} else {
		/* Do nothing! */
	}

	fflush(f);
	fclose(f);
	alarm(FILE_PRINT_TIMER);
	
	TRACE_FUNC_END();
	UNUSED(sig);
}
/*---------------------------------------------------------------------*/
static inline void
print_status_file()
{
	TRACE_FUNC_START();
	signal(SIGALRM, status_print);
	alarm(FILE_PRINT_TIMER);
	TRACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
export_global_socket()
{
	TRACE_FUNC_START();
	FILE *f;
	char socket_fname[FILENAME_MAX];

	sprintf(socket_fname, "/var/run/%s.port", PLATFORM_PROMPT);
	f = fopen(socket_fname, "w+");
	if (f == NULL) {
		TRACE_ERR("Failed to create global socket file!\n");
		TRACE_FUNC_END();
	}
	
	fprintf(f, "%d", BRICKS_LISTEN_PORT);

	fflush(f);
	fclose(f);	
	TRACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Main entry point
 */
int
main(int argc, char **argv)
{
	TRACE_FUNC_START();
	int rc;

	/* accept command-line arguments */
	while ( (rc = getopt(argc, argv, "a:b:hsdf:")) != -1) {
		switch(rc) {
		case 'a':
			pc_info.rshell_args = (int8_t *)strdup(optarg);
			if (NULL == pc_info.rshell_args) {
				TRACE_ERR("Can't strdup remote shell args!!\n");
			}
			break;
		case 'b':
			pc_info.batch_size = atoi(optarg);
			TRACE_DEBUG_LOG("Taking batch_size as %d\n", 
					pc_info.batch_size);
			break;
		case 'd':
			pc_info.daemonize = 1;
			break;
		case 'h':
			print_help(*argv);
			break;
		case 'f':
			pc_info.lua_startup_file = (uint8_t *)strdup(optarg);
			if (NULL == pc_info.lua_startup_file) {
				TRACE_ERR("Can't strdup string for lua_startup_file: %s\n",
					  strerror(errno));
			}
			TRACE_DEBUG_LOG("Taking file %s as startup\n",
					pc_info.lua_startup_file);
			load_lua_file(optarg);
			break;
		case 's':
			pc_info.rshell = 1;
			break;
		default:
			print_help(*argv);
			break;
		}
	}

	/* init_modules */
	init_modules();

	/* check if rshell and daemonize are both enabled */
	if (pc_info.daemonize && pc_info.rshell) {
		fprintf(stdout, "You cannot create a daemon process and "
			"a remote shell at the same time!\n");
		fflush(stdout);
		print_help(*argv);
		return EXIT_FAILURE;
	}
	
	/* warp to remote shell, if asked */
	if (pc_info.rshell) {
		if (pc_info.rshell_args == NULL)
			fprintf(stdout, "Using localhost and port %d by default\n",
				BRICKS_LISTEN_PORT);
		lua_kickoff(LUA_EXE_REMOTE_SHELL, pc_info.rshell_args);
		return EXIT_SUCCESS;
	}

	/* daemonize, if asked */
	if ((pc_info.daemonize && (rc=do_daemonize()) == 0) ||
	    !pc_info.daemonize) {
		if (mark_pid_file(pv.pid_file) != 0) {
			TRACE_FUNC_END();
			TRACE_ERR("Can't lock the pid file.\n"
				  "Is a previous bricks daemon already running??\n");
		}
		export_global_socket();
		lua_kickoff((pc_info.daemonize) ? 
			    LUA_EXE_SCRIPT : LUA_EXE_HOME_SHELL, NULL);
	}

	/* initialize file printing mini-module */
	print_status_file();

	/* 
	 * if the user wants to daemonize and the process 
	 * is the child, then jump to accepting requests...
	 */
	if (pc_info.daemonize && rc == 0) {
#ifdef PASSIVE_MODE
		start_listening_reqs();
#else
		while (1) sleep(PAUSE_PERIOD);
#endif
	}
	
	TRACE_FUNC_END();
	clean_exit(EXIT_SUCCESS);

	/* control will never come here */
	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------*/
