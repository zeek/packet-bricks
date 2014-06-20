/* for stdio and stdlib */
#include "pacf_log.h"
/* for filter interface */
#include "pacf_interface.h"
/*---------------------------------------------------------------------*/
volatile uint32_t stop_processing = 0;
extern int lua_kickoff(void);
/*---------------------------------------------------------------------*/
/**
 * Program termination point
 * XXX - This function will free up all the previously 
 * 	allocated resources.
 */
void
clean_exit(int exit_val)
{
	TRACE_FUNC_START();
	fprintf(stdout, "Goodbye!\n");
	TRACE_FUNC_END();
	exit(exit_val);

}
/*---------------------------------------------------------------------*/
/**
 * Main entry point
 */
int
main(int argc, char **argv)
{
	TRACE_FUNC_START();

	/* warp to lua interface */
	lua_kickoff();

	TRACE_FUNC_END();

	clean_exit(EXIT_SUCCESS);
	
	/* control will never come here */
	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------*/
