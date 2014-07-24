/* for stdio and stdlib */
#include "pacf_log.h"
/* for filter interface */
#include "pacf_interface.h"
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
/* for pacf interface funcs (start_listening_reqs()) */
#include "pacf_interface.h"
/* for lua variables access */
#include "lua_interpreter.h"
/* for signal handling */
#include <signal.h>
/* for config names */
#include "config.h"
/* for file open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/*---------------------------------------------------------------------*/
/* program variable for lua */
extern progvars_t pv;
/* global variable to indicate start/stop for lua interpreter */
volatile uint32_t stop_processing = 0;
/* do we have to daemonize? */
static uint8_t daemonize = 0;

/* global variable definitions */
PacfInfo pc_info = {
	.batch_size 		= DEFAULT_BATCH_SIZE,
	.lua_startup_file 	= NULL
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

	if (!daemonize) fprintf(stdout, "Goodbye!\n");
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
	fprintf(stdout, "Usage: %s [-b batch_size] "
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
		if ((fd = open("/dev/null", O_RDWR)) >= 0) {
			dup2(fd, 0);
			//dup2(fd, 1);
			//dup2(fd, 2);
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
/**
 * Main entry point
 */
int
main(int argc, char **argv)
{
	TRACE_FUNC_START();
	int rc;

	/* accept command-line arguments */
	while ( (rc = getopt(argc, argv, "b:hsdf:")) != -1) {
		switch(rc) {
		case 'b':
			pc_info.batch_size = atoi(optarg);
			TRACE_DEBUG_LOG("Taking batch_size as %d\n", 
					pc_info.batch_size);
			break;
		case 'd':
			daemonize = 1;
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
			lua_kickoff(LUA_EXE_REMOTE_SHELL, NULL);
			return EXIT_SUCCESS;
		default:
			print_help(*argv);
			break;
		}
	}

	/* init_modules */
	init_modules();

	if ((daemonize && (rc=do_daemonize()) == 0) ||
	    !daemonize) {
		if (mark_pid_file(pv.pid_file) != 0) {
			TRACE_FUNC_END();
			TRACE_ERR("Can't lock the pid file.\n"
				  "Is a previous pacf daemon already running??\n");
		}			
		lua_kickoff((daemonize) ? LUA_EXE_SCRIPT : LUA_EXE_HOME_SHELL, NULL);
	}

	/* initialize file printing mini-module */
	print_status_file();

	/* 
	 * if the user wants to daemonize and the process 
	 * is the child, then jump to accepting requests...
	 */
	if (daemonize && rc == 0) start_listening_reqs();
	
	TRACE_FUNC_END();
	clean_exit(EXIT_SUCCESS);

	/* control will never come here */
	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------*/
