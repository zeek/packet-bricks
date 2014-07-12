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
/* for lua variables access */
#include "lua_interpreter.h"
/* for signal handling */
#include <signal.h>
/* for limits */
#include <bits/stdio_lim.h>
/* for config names */
#include "config.h"
/*---------------------------------------------------------------------*/
/* program variable for lua */
extern progvars_t pv;
/* global variable to indicate start/stop for lua interpreter */
volatile uint32_t stop_processing = 0;
/* function to call lua interpreter */
extern int lua_kickoff(void);
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

	fprintf(stdout, "Goodbye!\n");
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
 * XXX - A rudimentary method to display file contents. This may be revised...
 */
static inline void
status_print(int sig)
{
	TRACE_FUNC_START();
	FILE *f;
	char status_fname[FILENAME_MAX];

	sprintf(status_fname, "/tmp/%s.status", PLATFORM_PROMPT);
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
	int c;

	/* accept command-line arguments */
	while ( (c = getopt(argc, argv, "b:hf:")) != -1) {
		switch(c) {
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
		case 'b':
			pc_info.batch_size = atoi(optarg);
			TRACE_DEBUG_LOG("Taking batch_size as %d\n", 
					pc_info.batch_size);
			break;
		case 'h':
			print_help(*argv);
			break;
		default:
			print_help(*argv);
			break;
		}
	}

	/* init_modules */
	init_modules();

	/* initialize file printing mini-module */
	print_status_file();

	/* warp to lua interface */
	lua_kickoff();

	TRACE_FUNC_END();

	clean_exit(EXIT_SUCCESS);

	/* control will never come here */
	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------*/
