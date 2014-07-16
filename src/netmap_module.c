/**
 *
 * Heavily derived from netmap's pkt-gen.c & ipfw package. 
 * This submodule is still under heavy construction.
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
/* for netmap initialization */
#include <sys/param.h>
/* for network_interface definition */
#include "network_interface.h"
/* for mbuf definition - for intelligent batching */
#include "mbuf.h"
/* for IFNAMSIZ */
#include <net/if.h>
/* for hash function */
#include "pkt_hash.h"
/*---------------------------------------------------------------------*/
int32_t
netmap_init(void **ctxt_ptr, void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	netmap_module_context *nmc;

	/* create netmap context */
	*ctxt_ptr = calloc(1, sizeof(netmap_module_context));
	nmc = (netmap_module_context *) (*ctxt_ptr);
	if (*ctxt_ptr == NULL) {
		TRACE_LOG("Can't allocate memory for netmap context\n");
		TRACE_NETMAP_FUNC_END();
		return -1;
	}
	
	nmc->eng = (engine *)engptr;
	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
int32_t
netmap_link_iface(void *ctxt, const unsigned char *iface,
		  const uint16_t batchsize, int8_t qid)
{
	TRACE_NETMAP_FUNC_START();

	char nifname[MAX_IFNAMELEN];
	netmap_module_context *nmc = (netmap_module_context *)ctxt;
	netmap_iface_context *nic = NULL;

	/* setting nm-ifname*/
	sprintf(nifname, "netmap:%s", iface);

	/* check if the interface has been registered with some other engine */
	netiface *nif = interface_find((char *)iface);
	if (nif == NULL) {
		nic = calloc(1, sizeof(netmap_iface_context));
		if (nic == NULL) {
			TRACE_ERR("Can't allocate memory for netmap_iface_context (for %s)\n", iface);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
		/* resetting base_nmd */
		memset(&nic->base_nmd, 0, sizeof(struct nm_desc));

		/* resetting fd to -1 */
		nic->global_fd = nmc->local_fd = -1;

		/* use some extra rings */
		nic->base_nmd.req.nr_arg3 = NM_EXTRA_BUFS;
		nic->nmd_flags |= NM_OPEN_ARG3;

		nic->global_nmd = nm_open((char *)nifname, NULL, 
					  nic->nmd_flags, 
					  &nic->base_nmd);

		if (nic->global_nmd == NULL) {
			TRACE_LOG("Unable to open %s: %s\n", 
				  iface, strerror(errno));
			free(nic);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
		nic->global_fd = nic->global_nmd->fd;
		TRACE_DEBUG_LOG("mapped %dKB at %p\n", 
				nic->global_nmd->req.nr_memsize>>10, 
				nic->global_nmd->mem);
	
		if (qid != -1) {
			nic->global_nmd->req.nr_flags = NR_REG_ONE_NIC;
			nic->global_nmd->req.nr_ringid = qid;
		}
	
		/* create interface entry */
		create_interface_entry(iface, (qid == -1) ? NO_QUEUES : HW_QUEUES, 
				       IO_NETMAP, nic, nmc->eng);
	} else { /* otherwise check if that interface can be registered */
		if (qid == -1) {
			TRACE_LOG("Qid not given!!! "
				  "Interface %s is set to read from H/W queues.",
				  iface);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
		nic = retrieve_and_register_interface_entry(iface, HW_QUEUES, 
							    IO_NETMAP, nmc->eng);
		if (nic == NULL) {
			TRACE_LOG("Error in linking ifname: %s to engine %s\n",
				  iface, nmc->eng->name);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
		nic->global_nmd->req.nr_flags = NR_REG_ONE_NIC;
		nic->global_nmd->req.nr_ringid = qid;
	} 

	nic->global_nmd->req.nr_ringid |= NETMAP_NO_TX_POLL;

	/* setting batch size */
	nmc->batch_size = batchsize;
	
	/* open handle */
	nmc->local_nmd = nm_open((char *)nifname, NULL, nic->nmd_flags |
				 NM_OPEN_IFNAME | NM_OPEN_NO_MMAP, 
				 nic->global_nmd);
	if (nmc->local_nmd == NULL) {
		TRACE_LOG("Unable to open %s: %s",
			  iface, strerror(errno));
		TRACE_NETMAP_FUNC_END();
		return -1;
	} else {
		nmc->local_fd = nmc->local_nmd->fd;
	}
	
	/* Wait for mandatory (& cautionary) PHY reset */
	TRACE_LOG("Wait for %d secs for phy reset\n",
		  NETMAP_LINK_WAIT_TIME);
	
	sleep(NETMAP_LINK_WAIT_TIME);
	TRACE_NETMAP_FUNC_END();

	return nmc->local_fd;
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
	
	if (!strcmp("all", (char *)iface))
		unregister_all_interfaces(eng);
	else
		unregister_interface_entry(iface, eng);

	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * XXX - An extremely inefficient way to drop the packets.
 * Will get back to this function again..
 */
static void
drop_packets(struct netmap_ring *ring, engine *eng)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t cur, rx, n;
	netmap_module_context *ctxt = 
		(netmap_module_context *) eng->private_context;
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
static int32_t
sample_packets(engine *eng, CommNode *cn)
{
	TRACE_NETMAP_FUNC_START();
        u_int dr; 			/* destination ring */
        u_int i = 0;
        const u_int n = cn->cur_txq;	/* how many queued packets */
        struct txq_entry *x = cn->q;
        int retry = TX_RETRIES;		/* max retries */
        struct nm_desc *dst = cn->vale_nmd;

	/* if queued pkts are zero.... skip! */
        if (n == 0) {
                TRACE_DEBUG_LOG("Nothing to forward to the interface valeA:s\n");
		TRACE_NETMAP_FUNC_END();
                return 0;
        }
	
 try_again:
        /* scan all output rings; dr is the destination ring index */
        for (dr = dst->first_tx_ring; i < n && dr <= dst->last_tx_ring; dr++) {
                struct netmap_ring *ring = NETMAP_TXRING(dst->nifp, dr);

                __builtin_prefetch(ring);
		/* oops! try next ring */
                if (nm_ring_empty(ring))
                        continue;
                /*
                 * We have different ways to transfer from src->dst
                 *
                 * src  dst     Now             Eventually (not done)
                 *
                 * PHYS PHYS    buf swap
                 * PHYS VIRT    NS_INDIRECT
                 * VIRT PHYS    copy            NS_INDIRECT
                 * VIRT VIRT    NS_INDIRECT
                 * MBUF PHYS    copy            NS_INDIRECT
                 * MBUF VIRT    NS_INDIRECT
                 *
                 * The "eventually" depends on implementing NS_INDIRECT
                 * on physical device drivers.
                 * Note we do not yet differentiate PHYS/VIRT.
                 */
                for  (; i < n && !nm_ring_empty(ring); i++) {
			/* we have empty tx slots, swap the pkts now! */
                        struct netmap_slot *dst, *src;
			struct netmap_ring *sr = x[i].ring;
                        dst = &ring->slot[ring->cur];
			src = &sr->slot[x[i].slot_idx];
			dst->len = src->len;
			dst->ptr = (uintptr_t)NETMAP_BUF(sr, src->buf_idx);
			dst->flags = NS_INDIRECT;
			ring->head = ring->cur = nm_ring_next(ring, ring->cur);
			/* updating engine statistics */
			eng->pkt_intercepted++;
			eng->byte_count += dst->len;
			eng->pkt_count++;
                }
        }
        if (i < n) {
                if (retry-- > 0) {
                        ioctl(cn->vale_nmd->fd, NIOCTXSYNC);
                        goto try_again;
                }
                TRACE_DEBUG_LOG("%d buffers leftover", n - i);
        }
        cn->cur_txq = 0;
	
	TRACE_NETMAP_FUNC_END();
        return 0;
}
/*------------------------------------------------------------------------*/
int32_t
netmap_callback(void *engptr, Rule *r)
{
	TRACE_NETMAP_FUNC_START();
	int i;
	engine *eng = (engine *)engptr;
	netmap_module_context *nmc = (netmap_module_context *)eng->private_context;
	struct netmap_if *nifp;
	struct netmap_ring *rxring;
	CommNode *cn = NULL;
	Target tgt;

	if (nmc->local_nmd == NULL) {
		TRACE_LOG("netmap context was not properly initialized\n");
		TRACE_NETMAP_FUNC_END();
		return -1;
	}

	tgt = (r == NULL || (r->tgt == SAMPLE && r->count == 0)) ? DROP : r->tgt;
	nifp = nmc->local_nmd->nifp;

	switch (tgt) {
	case DROP:
		for (i = nmc->local_nmd->first_rx_ring; 
		     i <= nmc->local_nmd->last_rx_ring; 
		     i++) {
			rxring = NETMAP_RXRING(nifp, i);
			if (nm_ring_empty(rxring))
				continue;
			
			drop_packets(rxring, eng);
		}
		break;
	case SAMPLE:
		for (i = nmc->local_nmd->first_rx_ring;
		     i <= nmc->local_nmd->last_rx_ring;
		     i++) {
			rxring = NETMAP_RXRING(nifp, i);
			
			__builtin_prefetch(rxring);
			if (nm_ring_empty(rxring))
				continue;
			__builtin_prefetch(&rxring->slot[rxring->cur]);
			while (!nm_ring_empty(rxring)) {
				u_int dst, src, idx;
				struct netmap_slot *slot;
				void *buf;

				src = rxring->cur;
				slot = &rxring->slot[src];
				__builtin_prefetch(slot+1);
				idx = slot->buf_idx;
				buf = (u_char *)NETMAP_BUF(rxring, idx);
				if (idx < 2) {
					TRACE_LOG("%s bogus RX index at offset %d",
						  nifp->ni_name, src);
					sleep(NETMAP_LINK_WAIT_TIME);
				}
				__builtin_prefetch(buf);
				
				cn = (CommNode *)r->destInfo[pkt_hdr_hash(buf) % r->count];
				dst = cn->cur_txq;
				if (dst >= TXQ_MAX) {
					sample_packets(eng, cn);
					continue;
				}
				
				rxring->head = rxring->cur = nm_ring_next(rxring, src);
				
				cn->q[dst].ring = rxring;
				cn->q[dst].slot_idx = src;
				cn->cur_txq++;
				
			}
		}
		if (cn->cur_txq > 0)
			sample_packets(eng, cn);
		break;
	case REDIRECT:
		break;
	case MODIFY:
		break;
	case COPY:
		break;
	case LIMIT:
		break;
	case PKT_NOTIFY:
		break;
	case BYTE_NOTIFY:
		break;
	default:
		/* control will never come here */
		TRACE_ERR("Something awful must have happened here\n");
		break;
	}
		
	TRACE_NETMAP_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
int32_t
netmap_shutdown(void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	engine *eng = (engine *)engptr;
	if (eng->run == 1) {
		eng->run = 0;
	} else {
		TRACE_NETMAP_FUNC_END();
		return -1;
	}

	TRACE_NETMAP_FUNC_END();	
	return 0;
}
/*---------------------------------------------------------------------*/
void
netmap_delete_all_channels(void *engptr, Rule *r) 
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn = NULL;
	uint32_t i;
	
	for (i = 0; i < r->count; i++) {
		cn = (CommNode *)r->destInfo[i];
		nm_close(cn->vale_nmd);
		free(cn);
	}

	TRACE_NETMAP_FUNC_END();
	/* not used for the moment */
	UNUSED(engptr);
}
/*---------------------------------------------------------------------*/
int32_t
netmap_create_channel(void *engptr, Rule *r, char *targ, int32_t csock) 
{
	TRACE_NETMAP_FUNC_START();
	char ifname[IFNAMSIZ];
	int32_t fd = -1;
	engine *eng = (engine *)engptr;
	netmap_module_context *nmc = (netmap_module_context *)eng->private_context;
	CommNode *cn;

	/* setting the name */
	snprintf(ifname, IFNAMSIZ, "%s", targ);

	/* create a comm. interface */	
	r->destInfo[r->count-1] = calloc(1, sizeof(CommNode));
	if (r->destInfo[r->count-1] == NULL) {
		TRACE_ERR("Can't allocate mem for destInfo[%d] for engine %s\n",
			  r->count - 1, eng->name);
		TRACE_NETMAP_FUNC_END();
		return -1;
	}
	
	cn = (CommNode *)r->destInfo[r->count - 1];
	uint64_t flags = NM_OPEN_NO_MMAP;
	cn->vale_nmd = nm_open(ifname, NULL, flags, nmc->local_nmd); 
	if (cn->vale_nmd == NULL) {
		TRACE_ERR("Can't open %p(%s)\n", cn->vale_nmd, ifname);
		TRACE_NETMAP_FUNC_END();
	}

	fd = cn->vale_nmd->fd;

	TRACE_LOG("Created %s interface\n", ifname);

	TRACE_NETMAP_FUNC_END();

	UNUSED(targ);
	UNUSED(csock);
	return fd;
}
/*---------------------------------------------------------------------*/
io_module_funcs netmap_module = {
	.init_context	= 	netmap_init,
	.link_iface	= 	netmap_link_iface,
	.unlink_iface	= 	netmap_unlink_iface,
	.callback	= 	netmap_callback,
	.create_channel =	netmap_create_channel,
	.delete_all_channels =	netmap_delete_all_channels,
	.shutdown	= 	netmap_shutdown
};
/*---------------------------------------------------------------------*/
