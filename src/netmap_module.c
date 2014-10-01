/*
 * Copyright (c) 2014, Asim Jamshed, Robin Sommer, Seth Hall
 * and the International Computer Science Institute. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/**
 *
 * This submodule is still under heavy construction.
 * 
 */
/* for io_module struct defn */
#include "io_module.h"
/* for bricks logging */
#include "bricks_log.h"
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
/* for brick def'n */
#include "brick.h"
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
netmap_unlink_ifaces(void *engptr)
{
	TRACE_NETMAP_FUNC_START();
	engine *eng = (engine *)engptr;
	netmap_module_context *nmc;
	uint i;

	for (i = 0; i < eng->no_of_sources; i++) { 
		nmc = (netmap_module_context *)eng->esrc[i]->private_context;
		/* if local netmap desc is not closed, close it */
		if (nmc->local_nmd != NULL)
			nm_close(nmc->local_nmd);
		nmc->local_nmd = NULL;
	}
	
	unregister_all_interfaces(eng);
	
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * XXX - An extremely inefficient way to drop the packets.
 * Will get back to this function again..
 */
static void
drop_packets(struct netmap_ring *ring, engine *eng, engine_src *esrcptr)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t cur, rx, n;
	netmap_module_context *nmc = 
		(netmap_module_context *) esrcptr->private_context;
	char *p;
	struct netmap_slot *slot;

	cur = ring->cur;
	n = nm_ring_space(ring);
	n = MIN(nmc->batch_size, n);

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
write_packets(CommNode *cn)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t rx, n;
	char *p;
	struct pcap_pkthdr phdr;
        struct txq_entry *x = cn->q;

	n = cn->cur_txq;
	gettimeofday(&phdr.ts, NULL);


	for (rx = 0; rx < n; rx++) {
		struct netmap_slot *src;
		struct netmap_ring *sr = x[rx].ring;
		src = &sr->slot[x[rx].slot_idx];
		p = NETMAP_BUF(sr, src->buf_idx);
		
		phdr.caplen = phdr.len = 
			src->len;
		pcap_dump((u_char *)cn->pdumper, &phdr, (u_char *)p);
		TRACE_DEBUG_LOG("Got one!\n");
	}
	
	cn->cur_txq = 0;
	TRACE_NETMAP_FUNC_END();
	return 0;
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

	/* if dst is NULL, then this has to be pcap write request */
	if (dst == NULL) {
		write_packets(cn);
		TRACE_NETMAP_FUNC_END();
		return 0;
	}

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
	
	/* if dst is NULL, then this has to be pcap write request */
	if (dst == NULL) {
		write_packets(cn);
		TRACE_NETMAP_FUNC_END();
		return 0;
	}

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
flush_all_cnodes(Brick *brick, engine *eng)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn;
	uint32_t i;
	linkdata *lnd = (linkdata *)brick->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->brick != NULL) {
			flush_all_cnodes(cn->brick, eng);
		} else { /* cn->brick == NULL */
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
update_cnode_ptrs(struct netmap_ring *rxring, Brick *brick,
		  engine *eng, uint src)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn;
	uint32_t i;
	linkdata *lnd = (linkdata *)brick->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->brick != NULL) {
			update_cnode_ptrs(rxring, cn->brick, eng, src);
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
void
dispatch_pkt(struct netmap_ring *rxring,
	     engine *eng,
	     Brick *brick,
	     uint8_t *buf,
	     unsigned char level)
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn = NULL;
	uint j;
	linkdata *lnd = (linkdata *)brick->private_data;
	BITMAP b;

	TRACE_DEBUG_LOG("(ifname: %s, lnd_count: %d\n", 
			lnd->ifname, lnd->count);
	/* increment the per-brick nested level */
	lnd->level = level + 1;
	b = brick->elib->process(brick, buf);
	for (j = 0; b != 0; j++) {
		if (CHECK_BIT(b, j)) {
			cn = (CommNode *)lnd->external_links[j];
			if (cn->brick != NULL) {
				dispatch_pkt(rxring, eng, 
					     cn->brick, buf, 
					     lnd->level);
			} else if (cn->filt == NULL) {
				cn->mark = 1;
			}
		}
		CLR_BIT(b, j);
	}

	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
int32_t
netmap_callback(void *engsrcptr)
{
	TRACE_NETMAP_FUNC_START();
	int i;
	netmap_module_context *nmc;
	struct nm_desc *local_nmd;
	struct netmap_if *nifp;
	struct netmap_ring *rxring;
	engine *eng;
	engine_src *engsrc;
	Brick *brick;

	engsrc = (engine_src *)engsrcptr;
	brick = engsrc->brick;
	eng = (engine *)brick->eng;
	nmc = (netmap_module_context *)engsrc->private_context;
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
		if (brick == NULL)
			drop_packets(rxring, eng, engsrc);
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
				dispatch_pkt(rxring, eng, brick, buf, 0);
				rxring->head = rxring->cur = nm_ring_next(rxring, src);
				update_cnode_ptrs(rxring, brick, eng, src);
			}
		}
	}

	flush_all_cnodes(brick, eng);
		
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
netmap_delete_all_channels(Brick *brick) 
{
	TRACE_NETMAP_FUNC_START();
	CommNode *cn = NULL;
	uint32_t i;
	linkdata *lnd = (linkdata *)brick->private_data;
	
	for (i = 0; i < lnd->count; i++) {
		cn = (CommNode *)lnd->external_links[i];
		if (cn->out_nmd != NULL)
			nm_close(cn->out_nmd);
		if (cn->pd != NULL || cn->pdumper) {
			pcap_close(cn->pd);
			pcap_dump_close(cn->pdumper);
			cn->pd = NULL;
			cn->pdumper = NULL;				
		}
		if (cn->brick != NULL) {
			netmap_delete_all_channels(cn->brick);
			brick->elib->deinit(cn->brick);
			cn->brick = NULL;
		}
		free(cn);
	}

	brick->elib->deinit(brick);
	TRACE_NETMAP_FUNC_END();
	/* not used for the moment */
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
/**
 * Function very poorly designed. Needs revision! For now.. does the
 * task correctly.
 */
static Brick *
enable_pipeline(Brick *brick, const char *ifname, Target t)
{
	TRACE_NETMAP_FUNC_START();
	uint32_t i;
	CommNode *cn;
	linkdata *lnd = (linkdata *)brick->private_data;
	for (i = 0; i < lnd->count; i++) {
		cn = lnd->external_links[i];
		if (!strcmp(cn->nm_ifname, ifname)) {
			/* found the right entry, now fill Brick entry */
			if (cn->brick == NULL) {
				cn->brick = createBrick(t);
				Linker_Intf li;
				/* XXX - Fix it! */
				li.hash_split = 4;
				if (cn->brick->elib->init(cn->brick, &li) == -1) {
					TRACE_LOG("Can't allocate mem to add new "
						  "brick's private context\n");
					TRACE_NETMAP_FUNC_END();
					free(cn->brick);
					return NULL;
				}
				brick->eng->mark_for_copy = 
					(brick->eng->mark_for_copy == 0 && 
					 li.type == COPY) ? 1 : 0;
				cn->brick->eng = brick->eng;
				linkdata *lnd = cn->brick->private_data;
				strcpy((char *)lnd->ifname, (char *)ifname);
				lnd->count++;
				lnd->tgt = t;
				lnd->external_links = realloc(lnd->external_links,
							      sizeof(void *) * lnd->count);
				if (lnd->external_links == NULL) {
					TRACE_LOG("Can't re-allocate to add a new brick!\n");
					brick->elib->deinit(cn->brick);
					TRACE_NETMAP_FUNC_END();
					return NULL;
				}
				return cn->brick;
			} else  {
				linkdata *lnd = cn->brick->private_data;
				if (lnd->tgt == t) {
					lnd->count++;
					lnd->external_links = realloc(lnd->external_links, 
								      sizeof(void *) * lnd->count);
					if (lnd->external_links == NULL) {
						TRACE_LOG("Can't re-allocate to add a new brick!\n");
						if (lnd->count == 1) free(cn->brick);
						TRACE_NETMAP_FUNC_END();
						return NULL;
					}
					return cn->brick;
				} else
					return NULL;
			}
		}
		if (cn->brick != NULL) {
			Brick *rc = enable_pipeline(cn->brick, ifname, t);
			if (rc != NULL) return rc;
		}
	}
	TRACE_NETMAP_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
int32_t
netmap_create_channel(char *in_name, char *out_name,
		      Target t, void *esrcptr) 
{
	TRACE_NETMAP_FUNC_START();
	char ifname[IFNAMSIZ];
	int32_t fd;
	engine *eng;
	netmap_module_context *nmc;
	CommNode *cn;
	linkdata *lnd;
	engine_src *esrc;
	Brick *brick;
	
	fd = -1;
	esrc = (engine_src *)esrcptr;
	brick = esrc->brick;
	eng = (engine *)brick->eng;

	nmc = (netmap_module_context *)esrc->private_context;

	lnd = (linkdata *)brick->private_data;
	/* first locate the in_nmd */
	if (strcmp((char *)lnd->ifname, in_name) != 0) {
		brick = enable_pipeline(brick, in_name, t);
		if (brick == NULL) {
			TRACE_LOG("Pipelining failed!! Could not find an appropriate "
				  "source (%s) for engine %s!\n", in_name, eng->name);
			TRACE_NETMAP_FUNC_END();
			return -1;
		}
	}
	
	/* reinitialize lnd if brick is reset */
	lnd = (linkdata *)brick->private_data;
	TRACE_LOG("brick: %p, local_desc: %p\n", brick, nmc->local_nmd);

	/* create a comm. interface */	
	lnd->external_links[lnd->init_cur_idx] = calloc(1, sizeof(CommNode));
	if (lnd->external_links[lnd->init_cur_idx] == NULL) {
		TRACE_ERR("Can't allocate mem for destInfo[%d] for engine %s\n",
			  lnd->init_cur_idx, eng->name);
		TRACE_NETMAP_FUNC_END();
		return -1;
	}

	cn = (CommNode *)lnd->external_links[lnd->init_cur_idx];

	if (out_name[0] == '>') {
		cn->pd = pcap_open_dead(DLT_EN10MB, ETH_FRAME_LEN);
		cn->pdumper = pcap_dump_open(cn->pd, &out_name[1]);
		fd = 0;
	} else {		
		/* setting the name */
		snprintf(ifname, IFNAMSIZ, "netmap:%s", out_name);
		cn->out_nmd = nm_open(ifname, NULL, NM_OPEN_NO_MMAP | NM_OPEN_ARG3, 
				      nmc->local_nmd); 
		if (cn->out_nmd == NULL) {
			TRACE_ERR("Can't open %p(%s)\n", cn->out_nmd, ifname);
			TRACE_NETMAP_FUNC_END();
		}
		strcpy_wtih_reverse_pipe(cn->nm_ifname, out_name);
	
		TRACE_LOG("zerocopy for %s --> %s (index: %d) %s", 
			  lnd->ifname, 
			  out_name, 
			  lnd->init_cur_idx,
			  (((struct nm_desc *)nmc->local_nmd)->mem == cn->out_nmd->mem) ? 
			  "enabled\n" : "disabled\n");
		fd = cn->out_nmd->fd;
	}

	lnd->init_cur_idx++;
	TRACE_LOG("Created %s interface\n", ifname);

	TRACE_NETMAP_FUNC_END();
	return fd;
}
/*---------------------------------------------------------------------*/
io_module_funcs netmap_module = {
	.init_context  		= 	netmap_init,
	.link_iface		= 	netmap_link_iface,
	.unlink_ifaces		= 	netmap_unlink_ifaces,
	.callback		= 	netmap_callback,
	.create_external_link 	=	netmap_create_channel,
	.delete_all_channels 	=	netmap_delete_all_channels,
	.shutdown		= 	netmap_shutdown,
};
/*---------------------------------------------------------------------*/
