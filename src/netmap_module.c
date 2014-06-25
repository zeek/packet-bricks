/* for io_module struct defn */
#include "io_module.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
void
netmap_link_iface(const unsigned char *iface, const uint16_t batchsize)
{
	TRACE_NETMAP_FUNC_START();
	UNUSED(iface);
	UNUSED(batchsize);
	TRACE_NETMAP_FUNC_END();
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
	.link_iface = netmap_link_iface,
	.unlink_iface = netmap_unlink_iface,
	.callback = netmap_callback,
	.shutdown = netmap_shutdown
};
/*---------------------------------------------------------------------*/
