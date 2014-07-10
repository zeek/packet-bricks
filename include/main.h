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
 *		 - the LUA script that will be interpreted by pacf...
 */
typedef struct PacInfo {
	uint16_t batch_size; /* read these many packets per read */
	const unsigned char *lua_startup_file; /* path to lua startup file 
						  given at cmd line */	
} PacfInfo __attribute__((aligned(__WORDSIZE)));

extern PacfInfo pc_info;
/*---------------------------------------------------------------------*/
#define DEFAULT_BATCH_SIZE		512
#define FILE_PRINT_TIMER		5 /* in secs */
/*---------------------------------------------------------------------*/
#endif /* !__MAIN_H__ */
