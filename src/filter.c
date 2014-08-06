/* for filter def'n */
#include "filter.h"
/* for logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
uint32_t
pass_pkt_against_filter(Filter *f, unsigned char *buf)
{
	TRACE_FILTER_FUNC_START();
	/*
	 * add your logic here
	 */
	TRACE_FILTER_FUNC_END();
	return 1;
	UNUSED(f);
	UNUSED(buf);
}
/*---------------------------------------------------------------------*/
void
process_filter_request(unsigned char *ifname, Filter *f)
{
	TRACE_FILTER_FUNC_START();
	
	TRACE_FILTER_FUNC_END();
	UNUSED(ifname);
	UNUSED(f);
}
/*---------------------------------------------------------------------*/
