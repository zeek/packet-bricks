/* for filter def'n */
#include "filter.h"
/* for logging */
#include "pacf_log.h"
/* for engine ptr */
#include "pkt_engine.h"
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
int32_t
process_filter_request(void *engptr, unsigned char *ifname, Filter *f)
{
	engine *eng = (engine *)engptr;
	TRACE_FILTER_FUNC_START();
	TRACE_FILTER_FUNC_END();
	return eng->iom.add_filter(eng->r, f, ifname);
}
/*---------------------------------------------------------------------*/
