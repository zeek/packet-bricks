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
 * XXX - To be filled
 */
typedef struct PacInfo {
	uint16_t batch_size; /* read these many packets per read */
	unsigned char *lua_startup_file; /* path to lua startup file 
					    given at cmd line */
	/* XXX - more to come */
	
} PacfInfo __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
#define DEFAULT_BATCH_SIZE		512
/*---------------------------------------------------------------------*/
#endif /* !__MAIN_H__ */
