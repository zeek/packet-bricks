/**
 *
 * Heavily derived from netmap's pkt-gen.c file. This submodule
 * is still under heavy construction.
 * 
 */
/* for poll() syscall */
#include <sys/poll.h>
/* for io_module struct defn */
#include "io_module.h"
/* for pacf logging */
#include "pacf_log.h"
/* for netmap structs */
#include "netmap_module.h"
/* for engine definition */
#include "pkt_engine.h"
/* for netmap initialization */
#include <sys/param.h>
/*---------------------------------------------------------------------*/
int
netmap_init(void **ctxt_ptr)
{
	TRACE_NETMAP_FUNC_START();
	netmap_module_context *nmc;

	/* create netmap context */
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
	nmc->global_fd = nmc->local_fd = -1;
	{
		/* XXX - this field may be useless... we'll see */
		nmc->nmr_config = (unsigned char *)"";
		/* use some extra rings */
		nmc->base_nmd.req.nr_arg3 = DEFAULT_EXTRA_RINGS;
		nmc->nmd_flags |= NM_OPEN_ARG3;

	}
	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
int
netmap_link_iface(void *ctxt, const unsigned char *iface,
		  const uint16_t batchsize)
{
	TRACE_NETMAP_FUNC_START();

	char nifname[MAX_IFNAMELEN];
	netmap_module_context *nmc = (netmap_module_context *)ctxt;

	/* setting nm-ifname*/
	sprintf(nifname, "netmap:%s", iface);
	nmc->global_nmd = nm_open((char *)nifname, NULL, 
				  nmc->nmd_flags, 
				  &nmc->base_nmd);

	if (nmc->global_nmd == NULL) {
		TRACE_LOG("Unable to open %s: %s\n", 
			  iface, strerror(errno));
		TRACE_NETMAP_FUNC_END();
		return -1;
	}
	nmc->global_fd = nmc->global_nmd->fd;
	TRACE_DEBUG_LOG("mapped %dKB at %p\n", 
			nmc->global_nmd->req.nr_memsize>>10, 
			nmc->global_nmd->mem);

	/* setting batch size */
	nmc->batch_size = batchsize;
	
	{
		/* XXX - support multiple threads */
		//nmc->global_nmd->req.nr_flags = NR_REG_ONE_NIC;
		//nmc->global_nmd->req.nr_ringid = 1;
	}

	/* open handle */
	nmc->local_nmd = nm_open((char *)nifname, NULL, nmc->nmd_flags |
				 NM_OPEN_IFNAME | NM_OPEN_NO_MMAP, 
				 nmc->global_nmd);
	if (nmc->local_nmd == NULL) {
		TRACE_LOG("Unable to open %s: %s",
			  iface, strerror(errno));
		TRACE_NETMAP_FUNC_END();
		return -1;
	} else {
		nmc->local_fd = nmc->local_nmd->fd;
	}
	
#if 0
	/* XXX Uncomment the following out in next version */
	devqueues = nmc->nmd->req.nr_rx_rings;
	/* CHECK IF # OF THREADS ARE LESS THAN # OF AVAILABLE QUEUES */
	/* THIS MAY BE SHIFTED TO PKT_ENGINE.C FILE */
#else
	nmc->global_nmd->req.nr_ringid |= NETMAP_NO_TX_POLL;
#endif
	
	/* Wait for mandatory (& cautionary) PHY reset */
	TRACE_LOG("Wait for %d secs for phy reset\n",
		  NETMAP_LINK_WAIT_TIME);
	
	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
void
netmap_unlink_iface(const unsigned char *iface, void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	engine *eng = (engine *)engptr;
	netmap_module_context *nmc = (netmap_module_context *)eng->private_context;
	/* if local netmap desc is not closed, close it */
	if (nmc->local_nmd != NULL)
		nm_close(nmc->local_nmd);
	nmc->local_nmd = NULL;

	UNUSED(iface);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
handle_packets(struct netmap_ring *ring, engine *eng)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t cur, rx, n;
	netmap_module_context *ctxt = (netmap_module_context *) eng->private_context;
	char *p;
	struct netmap_slot *slot;

	cur = ring->cur;
	n = nm_ring_space(ring);
	n = MIN(ctxt->batch_size, n);

	for (rx = 0; rx < n; rx++) {
		slot = &ring->slot[cur];
		p = NETMAP_BUF(ring, slot->buf_idx);
		
		/* update the statistics */
		eng->byte_count += slot->len;
		eng->pkt_count++;
		TRACE_DEBUG_LOG("Got one!\n");
		cur = nm_ring_next(ring, cur);
	}
	ring->head = ring->cur = cur;

	UNUSED(p);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
netmap_callback(void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	engine *eng = (engine *)engptr;
	netmap_module_context *nmc = (netmap_module_context *)eng->private_context;
	struct pollfd pfd = {.fd = nmc->local_fd, .events = POLLIN};
	struct netmap_if *nifp;
	struct netmap_ring *rxring;
	int32_t i;

	if (nmc->local_nmd == NULL) {
		TRACE_LOG("netmap context was not properly initialized\n");
		return;
	}

	nifp = nmc->local_nmd->nifp;
	while (eng->run) {
		i = poll(&pfd, 1, 1000);
		if (i > 0 && !(pfd.revents & POLLERR))
			break;
		TRACE_DEBUG_LOG("waiting for initial packets, "
				"poll returns %d %d\n",
				i, pfd.revents);
	}

	while (eng->run) {
		if (poll(&pfd, 1, 1 * 1000) < 0) {
			TRACE_LOG("Something weird happened\n");
			break;
		}
		if (pfd.revents & POLLERR) {
			TRACE_LOG("poll error!: %s\n", strerror(errno));
			break;
		}
		for (i = nmc->local_nmd->first_rx_ring; 
		     i <= nmc->local_nmd->last_rx_ring; 
		     i++) {
			rxring = NETMAP_RXRING(nifp, i);
			if (nm_ring_empty(rxring))
				continue;
			
			handle_packets(rxring, eng);
		}
	}

	TRACE_LOG("Engine %s is stopping...\n", eng->name);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
int32_t
netmap_shutdown(void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	engine *eng = (engine *)engptr;
	if (eng->run == 1)
		eng->run = 0;
	else
		return -1;
	
	return 0;
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
