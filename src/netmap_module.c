/* for io_module struct defn */
#include "io_module.h"
/* for pacf logging */
#include "pacf_log.h"
/* for netmap structs */
#include "netmap_module.h"
/*---------------------------------------------------------------------*/
int
netmap_init(void **ctxt_ptr)
{
	TRACE_NETMAP_FUNC_START();
	netmap_module_context *nmc;

	*ctxt_ptr = calloc(1, sizeof(netmap_module_context));
	nmc = (netmap_module_context *) ctxt_ptr;
	if (*ctxt_ptr == NULL) {
		TRACE_LOG("Can't allocate memory for netmap context\n");
		TRACE_NETMAP_FUNC_END();
		return -1;
	}
	
	/* resetting base_nmd */
	memset(&nmc->base_nmd, 0, sizeof(struct nm_desc));
	/* resetting fd to -1 */
	nmc->fd = -1;
	/* XXX - this field may be useless... we'll see */
	nmc->nmr_config = (unsigned char *)"";
	/* use some extra rings */
	nmc->base_nmd.req.nr_arg3 = DEFAULT_EXTRA_RINGS;
	nmc->nmd_flags |= NM_OPEN_ARG3;

	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
int
netmap_link_iface(void *ctxt, const unsigned char *iface,
		  const uint16_t batchsize)
{
	TRACE_NETMAP_FUNC_START();

	netmap_module_context *nmc = (netmap_module_context *)ctxt;
	
	nmc->nmd = nm_open((char *)iface, NULL, 
			   nmc->nmd_flags, 
			   &nmc->base_nmd);

	if (nmc->nmd == NULL) {
		TRACE_LOG("Unable to open %s: %s\n", 
			  iface, strerror(errno));
		return -1;
		TRACE_NETMAP_FUNC_END();
	}
	nmc->fd = nmc->nmd->fd;
	TRACE_DEBUG_LOG("mapped %dKB at %p\n", 
			nmc->nmd->req.nr_memsize>>10, 
			nmc->nmd->mem);
	nmc->batch_size = batchsize;

	/* XXX Uncomment the following out in next version */
#if 0
	devqueues = nmc->nmd->req.nr_rx_rings;
	/* CHECK IF # OF THREADS ARE LESS THAN # OF AVAILABLE QUEUES */
	/* THIS MAY BE SHIFTED TO PKT_ENGINE.C FILE */
#else
	nmc->nmd->req.nr_ringid |= NETMAP_NO_TX_POLL;
#endif
	
	/* Wait for mandatory (& cautionary) PHY reset */
	TRACE_LOG("Wait for %d secs for phy reset", NETMAP_LINK_WAIT_TIME);
	
	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
void
netmap_unlink_iface(const unsigned char *iface)
{
	TRACE_NETMAP_FUNC_START();
	UNUSED(iface);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
netmap_callback(void *handle, void *pkts)
{
	TRACE_NETMAP_FUNC_START();
	UNUSED(handle);
	UNUSED(pkts);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
netmap_shutdown(void *handle)
{
	TRACE_NETMAP_FUNC_START();
	UNUSED(handle);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
io_module_funcs netmap_module = {
	.init_context	= netmap_init,
	.link_iface	= netmap_link_iface,
	.unlink_iface	= netmap_unlink_iface,
	.callback	= netmap_callback,
	.shutdown	= netmap_shutdown
};
/*---------------------------------------------------------------------*/
