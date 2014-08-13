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
/* for element def'n */
#include "element.h"
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
			TRACE_ERR("Can't allocate memory for "
				  "netmap_iface_context (for %s)\n", iface);
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
		TRACE_DEBUG_LOG("zerocopy %s", 
				(nic->global_nmd->mem == nic->base_nmd.mem) ? 
				"enabled\n" : "disabled\n");
		
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
		TRACE_DEBUG_LOG("zerocopy %s", 
				(nic->global_nmd->mem == nmc->local_nmd->mem) ? 
				"enabled\n" : "disabled\n");
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
share_packets(CommNode *cn)
{
	TRACE_NETMAP_FUNC_START();
        u_int dr; 			/* destination ring */
        u_int i = 0;
        const u_int n = cn->cur_txq;	/* how many queued packets */
        struct txq_entry *x = cn->q;
        int retry = TX_RETRIES;		/* max retries */
        struct nm_desc *dst = cn->out_nmd;

	/* if queued pkts are zero.... skip! */
        if (n == 0) {
                TRACE_DEBUG_LOG("Nothing to forward to pipe nmd: %p\n",
				cn->out_nmd);
		TRACE_NETMAP_FUNC_END();
                return 0;
        }

 try_share_again:	
        /* scan all output rings; dr is the destination ring index */
        for (dr = dst->first_tx_ring; i < n && dr <= dst->last_tx_ring; dr++) {
                struct netmap_ring *ring = NETMAP_TXRING(dst->nifp, dr);

                __builtin_prefetch(ring);
		/* oops! try next ring */
                if (nm_ring_empty(ring))
                        continue;

                for  (; i < n && !nm_ring_empty(ring); i++) {
			/* we have empty tx slots, swap the pkts now! */
                        struct netmap_slot *dst, *src;
			struct netmap_ring *sr = x[i].ring;
                        dst = &ring->slot[ring->cur];
			src = &sr->slot[x[i].slot_idx];
			dst->len = src->len;

			/* Swap now! */
			register u_int tmp;
			dst->flags = src->flags = NS_BUF_CHANGED;
			tmp = dst->buf_idx;
			dst->buf_idx = src->buf_idx;
			src->buf_idx = tmp;

			ring->head = ring->cur = nm_ring_next(ring, ring->cur);
                }
        }
        if (i < n) {
                if (retry-- > 0) {
                        ioctl(cn->out_nmd->fd, NIOCTXSYNC);
                        goto try_share_again;
                }
                TRACE_DEBUG_LOG("%d buffers leftover", n - i);
        }
        cn->cur_txq = 0;
	
	TRACE_NETMAP_FUNC_END();
        return 0;
}
/*---------------------------------------------------------------------*/
static int32_t
copy_packets(CommNode *cn)
{
	TRACE_NETMAP_FUNC_START();
        u_int dr; 			/* destination ring */
        int i = 0;
	const int n = cn->cur_txq;	/* how many queued pkts */
        int retry = TX_RETRIES;		/* max retries */
        struct nm_desc *dst = cn->out_nmd;
        struct txq_entry *x = cn->q;
	
	/* if queued pkts are zero.... skip! */
        if (n == 0) {
                TRACE_DEBUG_LOG("Nothing to forward to pipe nmd: %p\n",
				cn->out_nmd);
		TRACE_NETMAP_FUNC_END();
                return 0;
        }
 try_copy_again:
	/* scan all output rings; dr is the destination ring index */
	for (dr = dst->first_tx_ring; i < n && dr <= dst->last_tx_ring; dr++) {
		struct netmap_ring *ring = NETMAP_TXRING(dst->nifp, dr);
		
		__builtin_prefetch(ring);
		/* oops! try next ring */
		if (nm_ring_empty(ring))
			continue;
		
		for  (; i < n && !nm_ring_empty(ring); i++) {
			/* we have empty tx slots, copy the pkts now! */
			char *srcbuf, *dstbuf;
			struct netmap_slot *dst, *src;
			struct netmap_ring *sr = x[i].ring;
			dst = &ring->slot[ring->cur];
			src = &sr->slot[x[i].slot_idx];
			
			dst->len = src->len;
			srcbuf = NETMAP_BUF(sr, src->buf_idx);
			dstbuf = NETMAP_BUF(ring, dst->buf_idx);
			nm_pkt_copy(dstbuf, srcbuf, dst->len); 
			
			ring->head = ring->cur = nm_ring_next(ring, ring->cur);
		}
	}
	if (i < n) {
		if (retry-- > 0) {
			ioctl(cn->out_nmd->fd, NIOCTXSYNC);
			goto try_copy_again;
		}
		TRACE_DEBUG_LOG("%d buffers leftover", n - i);
	}
	cn->cur_txq = 0;
	
	TRACE_NETMAP_FUNC_END();
        return 0;
}
/*------------------------------------------------------------------------*/
void
flush_all_cnodes(Element *elem, engine *eng)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn;
	uint32_t i;
	linkdata *lnd = (linkdata *)elem->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->elem != NULL) {
			flush_all_cnodes(cn->elem, eng);
		} else { /* cn->elem == NULL */
			if (cn->cur_txq > 0) {
				(eng->mark_for_copy == 1) ? 
					copy_packets(cn) :
					share_packets(cn);
			}
		}
	}
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
update_cnode_ptrs(struct netmap_ring *rxring, Element *elem,
		  engine *eng, uint src)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn;
	uint32_t i;
	linkdata *lnd = (linkdata *)elem->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->elem != NULL) {
			update_cnode_ptrs(rxring, cn->elem, eng, src);
		} else {
			uint dst = cn->cur_txq;
			if (cn->mark == 1) {
				cn->q[dst].ring = rxring;
				cn->q[dst].slot_idx = src;
				cn->cur_txq++;
				cn->mark = 0;
			}
		}
	}
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline uint32_t
myrand(uint64_t *seed)
{
	TRACE_NETMAP_FUNC_START();
	*seed = *seed * 1103515245 + 12345;
	return (uint32_t)(*seed >> 32);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
dispatch_pkt(struct netmap_ring *rxring,
	     engine *eng,
	     Element *elem,
	     uint8_t *buf)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn = NULL;
	uint j;
	linkdata *lnd = (linkdata *)elem->private_data;
	
	if (elem->elib == &lbfuncs) {
		TRACE_DEBUG_LOG("(ifname: %s, seed: %llu, lnd_count: %d\n", 
				lnd->ifname, (long long unsigned)eng->seed,
				lnd->count);
		cn = (CommNode *)lnd->external_links[(pkt_hdr_hash(buf) +  
						      myrand(&eng->seed)) % 
						     lnd->count];
		if (cn->elem != NULL) {
			dispatch_pkt(rxring, eng, cn->elem, buf);
		} else if (cn->filt == NULL) {
			cn->mark = 1;
		}
	} else if(elem->elib == &dupfuncs) {
		linkdata *lnd = (linkdata *)elem->private_data;
		/* pick any cn as reference */
		cn = (CommNode *)(*lnd->external_links);
		
		for (j = 0; j < lnd->count; j++) {
			cn = (CommNode *)lnd->external_links[j];
			if (cn->elem != NULL)
				dispatch_pkt(rxring, eng, cn->elem, buf);
			else if (cn->filt == NULL) {
				cn->mark = 1;
			}
		}
	}

	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
int32_t
netmap_callback(void *engptr, Element *elem)
{
	TRACE_NETMAP_FUNC_START();
	int i;
	netmap_module_context *nmc;
	struct nm_desc *local_nmd;
	struct netmap_if *nifp;
	struct netmap_ring *rxring;
	engine *eng;

	eng = (engine *)engptr;
	nmc = (netmap_module_context *)eng->private_context;
	local_nmd = (struct nm_desc *)nmc->local_nmd;

	if (local_nmd == NULL) {
		TRACE_LOG("netmap context was not properly initialized\n");
		TRACE_NETMAP_FUNC_END();
		return -1;
	}

	nifp = local_nmd->nifp;
	
	for (i = local_nmd->first_rx_ring;
	     i <= local_nmd->last_rx_ring;
	     i++) {
		rxring = NETMAP_RXRING(nifp, i);
		__builtin_prefetch(rxring);
		if (nm_ring_empty(rxring))
			continue;
		if (elem == NULL)
			drop_packets(rxring, eng);
		else {
			__builtin_prefetch(&rxring->slot[rxring->cur]);
			while (!nm_ring_empty(rxring)) {
				u_int src, idx;
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
				eng->byte_count += slot->len;
				eng->pkt_count++;
				//eng->seed = 0;
				dispatch_pkt(rxring, eng, elem, buf);
				rxring->head = rxring->cur = nm_ring_next(rxring, src);
				update_cnode_ptrs(rxring, elem, eng, src);
			}
		}
	}

	flush_all_cnodes(elem, eng);
		
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
netmap_delete_all_channels(void *engptr, Element *elem) 
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn = NULL;
	uint32_t i;
	linkdata *lnd = (linkdata *)elem->private_data;
	
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->out_nmd != NULL)
			nm_close(cn->out_nmd);
		if (cn->elem != NULL)
			netmap_delete_all_channels(engptr, cn->elem);
		
#if 0
		if (cn->elem->private_data != NULL)
			free(cn->elem->private_data);
#endif
		free(cn->elem);
		free(cn);
	}
#if 0
	if (elem->private_data)
		free(elem->private_data);
#endif
	free(elem);
	TRACE_NETMAP_FUNC_END();
	/* not used for the moment */
	UNUSED(engptr);
}
/*---------------------------------------------------------------------*/
/**
 * XXX - A rather inefficient version of reversing pipe name strings...
 * Will further improve later...
 */
static void
strcpy_wtih_reverse_pipe(char *to, char *from)
{
	TRACE_NETMAP_FUNC_START();
	register int i = 0;
	do {
		switch (from[i]) {
		case '}':
			to[i] = '{';
			break;
		case '{':
			to[i] = '}';
			break;
		default:
			to[i] = from[i];
			break;
		}
		i++;
	} while (from[i] != '\0');
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
static Element *
enable_pipeline(Element *elem, const char *ifname, Target t)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t i;
	CommNode *cn;
	linkdata *lnd = (linkdata *)elem->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = lnd->external_links[i];
		if (!strcmp(cn->nm_ifname, ifname)) {
			/* found the right entry, now fill Element entry */
			if (cn->elem == NULL) {
				cn->elem = createElement(t);
				if (cn->elem->elib->init(cn->elem, NULL) == -1) {
					TRACE_LOG("Can't allocate mem to add new "
						  "element's private context\n");
					TRACE_NETMAP_FUNC_END();
					free(cn->elem);
					return NULL;
				}
				cn->elem->eng = elem->eng;
				linkdata *lnd = cn->elem->private_data;
				strcpy((char *)lnd->ifname, (char *)ifname);
				lnd->count++;
				lnd->tgt = t;
				lnd->external_links = realloc(lnd->external_links,
							      sizeof(void *) * lnd->count);
				if (lnd->external_links == NULL) {
					TRACE_LOG("Can't re-allocate to add a new element!\n");
					free(cn->elem->private_data);
					free(cn->elem);
					TRACE_NETMAP_FUNC_END();
					return NULL;
				}
				return cn->elem;
			} else  {
				linkdata *lnd = cn->elem->private_data;
				if (lnd->tgt == t) {
					lnd->count++;
					lnd->external_links = realloc(lnd->external_links, 
								      sizeof(void *) * lnd->count);
					if (lnd->external_links == NULL) {
						TRACE_LOG("Can't re-allocate to add a new element!\n");
						if (lnd->count == 1) free(cn->elem);
						TRACE_NETMAP_FUNC_END();
						return NULL;
					}
					return cn->elem;
				} else
					return NULL;
			}
		}
		if (cn->elem != NULL)
			return enable_pipeline(cn->elem, ifname, t);
	}
	TRACE_NETMAP_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
int32_t
netmap_create_channel(Element *elem, char *in_name, 
		      char *out_name, Target t) 
{
	TRACE_NETMAP_FUNC_START();
	char ifname[IFNAMSIZ];
	int32_t fd;
	engine *eng;
	netmap_module_context *nmc;
	CommNode *cn;
	linkdata *lnd;
	
	fd = -1;
	eng = (engine *)elem->eng;
	nmc = (netmap_module_context *)eng->private_context;

	/* first locate the in_nmd */
	if (strcmp((char *)eng->link_name, in_name) != 0) {
		elem = enable_pipeline(elem, in_name, t);
		if (elem == NULL) {
			TRACE_LOG("Pipelining failed!! Could not find an appropriate "
				  "source (%s) for engine %s!\n", in_name, eng->name);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
	}
	
	lnd = (linkdata *)elem->private_data;
	TRACE_LOG("elem: %p, local_desc: %p\n", elem, nmc->local_nmd);
	/* setting the name */
	snprintf(ifname, IFNAMSIZ, "netmap:%s", out_name);

	/* create a comm. interface */	
	lnd->external_links[lnd->init_cur_idx] = calloc(1, sizeof(CommNode));
	if (lnd->external_links[lnd->init_cur_idx] == NULL) {
		TRACE_ERR("Can't allocate mem for destInfo[%d] for engine %s\n",
			  lnd->init_cur_idx, eng->name);
		TRACE_NETMAP_FUNC_END();
		return -1;
	}

	cn = (CommNode *)lnd->external_links[lnd->init_cur_idx];
	cn->out_nmd = nm_open(ifname, NULL, NM_OPEN_NO_MMAP | NM_OPEN_ARG3, 
			      nmc->local_nmd); 
	if (cn->out_nmd == NULL) {
		TRACE_ERR("Can't open %p(%s)\n", cn->out_nmd, ifname);
		TRACE_NETMAP_FUNC_END();
	}
	strcpy_wtih_reverse_pipe(cn->nm_ifname, out_name);
	
	TRACE_LOG("zerocopy for %s --> %s (index: %d) %s", 
		  eng->link_name, 
		  out_name, 
		  lnd->init_cur_idx,
		  (((struct nm_desc *)nmc->local_nmd)->mem == cn->out_nmd->mem) ? 
		  "enabled\n" : "disabled\n");
	fd = cn->out_nmd->fd;

	lnd->init_cur_idx++;
	TRACE_LOG("Created %s interface\n", ifname);

	TRACE_NETMAP_FUNC_END();
	return fd;
}
/*---------------------------------------------------------------------*/
int32_t
netmap_add_filter(Element *elem, Filter *f, unsigned char *ifname)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn;
	uint32_t i;
	linkdata *lnd = (linkdata *)elem->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->elem != NULL)
			netmap_add_filter(cn->elem, f, ifname);
		else if (!strcmp((char *)cn->nm_ifname, (char *)ifname)) {
			if (cn->filt == NULL) {
				cn->filt = calloc(1, sizeof(Filter));
				if (cn->filt == NULL) {
					TRACE_LOG("Can't allocate mem for "
						  "filter for ifname %s\n",
						  ifname);
					TRACE_NETMAP_FUNC_END();
					return -1;
				}
			}
			memcpy(cn->filt, f, sizeof(Filter));
		}
	}
	TRACE_NETMAP_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
io_module_funcs netmap_module = {
	.init_context  		= 	netmap_init,
	.link_iface		= 	netmap_link_iface,
	.unlink_iface		= 	netmap_unlink_iface,
	.callback		= 	netmap_callback,
	.create_external_link 	=	netmap_create_channel,
	.delete_all_channels 	=	netmap_delete_all_channels,
	.shutdown		= 	netmap_shutdown,
	.add_filter		=	netmap_add_filter
};
/*---------------------------------------------------------------------*/
