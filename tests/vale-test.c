/* for assert statements */
#include <assert.h>
/* for int*_t datatypes */
#include <sys/types.h>
/* for libc funcs */
#include <stdlib.h>
/* for std I/O */
#include <stdio.h>
#define NETMAP_WITH_LIBS
/* for netmap */
#include <net/netmap_user.h>
/* for mbuf definition */
#include <mbuf.h>
/*-----------------------------------------------------------------------*/
struct my_netmap_port;
void netmap_enqueue(struct mbuf *m);
int netmap_fwd(struct my_netmap_port *port);

#define UNUSED(x)			(void)x
#define FREE_PKT(m)			netmap_enqueue(m)
/*-----------------------------------------------------------------------*/
/*
 * A packet comes from either a netmap slot on the source,
 * or from an mbuf that must be freed.
 * slot != NULL means a netmap slot, otherwise use buf.
 * len == 0 means an empty slot.
 */
struct txq_entry {
        void *ring_or_mbuf;
        uint16_t slot_idx;      /* used if ring */
        uint16_t flags;                 /* 0 if slot, len if mbuf */
#define TXQ_IS_SLOT     0xc555
#define TXQ_IS_MBUF     0xaacd
};

/*
 * the state associated to a netmap port:
 * (goes into the private field of my_ring)
 * XXX have an ifp at the beginning so we can use rcvif to store it.
 */
#define MY_TXQ_LEN      32
typedef struct my_netmap_port {
        char ifname[IFNAMSIZ];          /* contains if_xname */
        struct nm_desc  *d;
        struct my_netmap_port *peer;    /* peer port */
        //struct sess     *sess;          /* my session */
        u_int           cur_txq;        /* next txq slot to use for tx */
        struct txq_entry q[MY_TXQ_LEN]; /* slots are in the peer */
        /* followed by ifname */
} my_netmap_port;
/*-----------------------------------------------------------------------*/
int
set_sessions(fd_set *r, fd_set *w, my_netmap_port *p1, my_netmap_port *p2)
{
	FD_ZERO(r);
	FD_ZERO(w);
	FD_SET(p1->d->fd, r);
	FD_SET(p2->d->fd, r);
	FD_SET(p1->d->fd, w);
	FD_SET(p2->d->fd, w);
	
	return (p1->d->fd > p2->d->fd) ? 
		(p1->d->fd + 1): (p2->d->fd + 1);
}
/*-----------------------------------------------------------------------*/
my_netmap_port *
netmap_add_port(const char *dev)
{
        //static struct sess *s1 = NULL;  // XXX stateful; bad!
        my_netmap_port *port;
        int l;
        //struct sess *s2;

        D("opening netmap device %s", dev);
        l = strlen(dev) + 1;
        if (l >= IFNAMSIZ) {
                D("name %s too long, max %d", dev, IFNAMSIZ - 1);
                sleep(2);
                return NULL;
        }
        port = calloc(1, sizeof(*port));
        port->d = nm_open(dev, NULL, 0, NULL);
        if (port->d == NULL) {
                D("error opening %s", dev);
                free(port);        // XXX compat
                return NULL;
        }
        //s2 = new_session(port->d->fd, netmap_read, port, WANT_READ);
        //port->sess = s2;
        D("create my_netmap_port %p", port);
	strcpy(port->ifname, dev);
        //if (s1 == NULL) {       /* first of a pair */
        //        s1 = s2;
        //} else {                /* second of a pair, cross link */
	//       struct my_netmap_port *peer = s1->arg;
	//      port->peer = peer;
	//      peer->peer = port;
	//      D("%p %s <-> %p %s",
	//	  port, port->d->req.nr_name,
	//	  peer, peer->d->req.nr_name);
	//      s1 = NULL;
        //}
	return port;
}
/*-----------------------------------------------------------------------*/
void    
netmap_enqueue(struct mbuf *m)
{
        struct my_netmap_port *peer = m->__m_peer;
        struct txq_entry *x;


        if (peer == NULL) {
		assert(0);
                D("error missing peer in %p", m);
                FREE_PKT(m);
        }
        ND("start with %d packets", peer->cur_txq);
        if (peer->cur_txq >= MY_TXQ_LEN)
                netmap_fwd(peer);
        x = peer->q + peer->cur_txq;
        x->ring_or_mbuf = m;
        x->flags = TXQ_IS_MBUF;
        peer->cur_txq++;
        //peer->sess->flags |= WANT_RUN;
        ND("end, queued %d on %s", peer->cur_txq, peer->ifname);
}
/*-----------------------------------------------------------------------*/
/*
 * txq[] has a batch of n packets that possibly need to be forwarded.
 */
int
netmap_fwd(struct my_netmap_port *port)
{
        u_int dr; /* destination ring */
        u_int i = 0;
        const u_int n = port->cur_txq;  /* how many queued packets */
        struct txq_entry *x = port->q;
        int retry = 5;  /* max retries */
        struct nm_desc *dst = port->d;

        if (n == 0) {
                D("nothing to forward to %s", port->ifname);
                return 0;
        }

 again:
        /* scan all output rings; dr is the destination ring index */
        for (dr = dst->first_tx_ring; i < n && dr <= dst->last_tx_ring; dr++) {
                struct netmap_ring *ring = NETMAP_TXRING(dst->nifp, dr);

                __builtin_prefetch(ring);
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
                        struct netmap_slot *dst, *src;

                        dst = &ring->slot[ring->cur];
                        if (x[i].flags == TXQ_IS_SLOT) {
#if 0 // old code zero copy through interfaces
                                src = x[i].slot;
                                // XXX swap buffers
                                D("pkt %d len %d", i, src->len);
                                dst->len = src->len;
                                u_int tmp;
                                dst->flags = src->flags = NS_BUF_CHANGED;
                                tmp = dst->buf_idx;
                                dst->buf_idx = src->buf_idx;
                                src->buf_idx = tmp;
#else // new code for virtual port, do a copy
                                struct netmap_ring *sr = x[i].ring_or_mbuf;

                                src = &sr->slot[x[i].slot_idx];
                                dst->len = src->len;
                                dst->ptr = (uintptr_t)NETMAP_BUF(sr, src->buf_idx);
                                dst->flags = NS_INDIRECT;
                                if (0) nm_pkt_copy(
						   NETMAP_BUF(sr, src->buf_idx), // XXX wrong ring
						   NETMAP_BUF(ring, dst->buf_idx),
						   dst->len);
#endif
                        } else if (x[i].flags == TXQ_IS_MBUF) {
                                struct mbuf *m = (void *)x[i].ring_or_mbuf;

                                ND("copy from mbuf");
                                dst->len = m->__m_extlen;
                                nm_pkt_copy(m->__m_extbuf,
					    NETMAP_BUF(ring, dst->buf_idx),
					    dst->len);
                                FREE_PKT(m);
                        } else {
                                fprintf(stderr, "bad slot\n");
                        }
                        x[i].flags = 0;
                        ring->head = ring->cur = nm_ring_next(ring, ring->cur);
                }
        }
        if (i < n) {
                if (retry-- > 0) {
                        ioctl(port->d->fd, NIOCTXSYNC);
                        goto again;
                }
                ND("%d buffers leftover", n - i);
                for (;i < n; i++) {
                        if (x[i].flags == TXQ_IS_MBUF) {
                                FREE_PKT(x[i].ring_or_mbuf);
                        }
                }
        }
        port->cur_txq = 0;
        return 0;
}
/*-----------------------------------------------------------------------*/
void
netmap_read(my_netmap_port *p1, my_netmap_port *p2)
{
        struct my_netmap_port *port = p1;
        u_int si;
        //struct mbuf dm, dm0;
        //struct ip_fw_args args;
        struct my_netmap_port *peer = p2;
        struct txq_entry *x = peer->q;
        struct nm_desc *srcp = port->d;
	
        //bzero(&dm0, sizeof(dm0));
        //bzero(&args, sizeof(args));
	
        /* scan all rings */
        for (si = srcp->first_rx_ring; si <= srcp->last_rx_ring; si++) {
		struct netmap_ring *ring = NETMAP_RXRING(srcp->nifp, si);
		
		__builtin_prefetch(ring);
		if (nm_ring_empty(ring))
			continue;
		__builtin_prefetch(&ring->slot[ring->cur]);
		while (!nm_ring_empty(ring)) {
			u_int dst, src, idx/*, len*/;
			struct netmap_slot *slot;
			void *buf;

			dst = peer->cur_txq;
			if (dst >= MY_TXQ_LEN) {
				netmap_fwd(peer);
				continue;
			}
			src = ring->cur;
			slot = &ring->slot[src];
			__builtin_prefetch (slot+1);
			idx = slot->buf_idx;
			buf = (u_char *)NETMAP_BUF(ring, idx);
			if (idx < 2) {
				D("%s bogus RX index at offset %d",
				  srcp->nifp->ni_name, src);
				sleep(2);
			}
			__builtin_prefetch(buf);
			ring->head = ring->cur = nm_ring_next(ring, src);

#if 0
			/* prepare to invoke the firewall */
			dm = dm0;       // XXX clear all including tags
			args.m = &dm;
			len = slot->len;
			dm.m_flags = M_STACK;
			// remember original buf and peer
			dm.__m_extbuf = buf;
			dm.__m_extlen = len;
			dm.__m_peer = peer;
			/* the routine to call in netisr_dispatch */
			dm.__m_callback = netmap_enqueue;

			/* XXX can we use check_frame ? */
			dm.m_pkthdr.rcvif = &port->ifp;
			dm.m_data = buf + 14;   // skip mac
			dm.m_len = dm.m_pkthdr.len = len - 14;
			ND("slot %d len %d", i, dm.m_len);
			// XXX ipfw_chk is slightly faster
			//ret = ipfw_chk(&args);
			ipfw_check_packet(NULL, &args.m, NULL, PFIL_IN, NULL);
			if (args.m != NULL) {   // ok. forward
				/*
				 * XXX TODO remember to clean up any tags that
				 * ipfw may have allocated
				 */
				x[dst].ring_or_mbuf = ring;
				x[dst].slot_idx = src;
				x[dst].flags = TXQ_IS_SLOT;
				peer->cur_txq++;
			}
			ND("exit at slot %d", next_i);
#endif
			x[dst].ring_or_mbuf = ring;
			x[dst].slot_idx = src;
			x[dst].flags = TXQ_IS_SLOT;
			peer->cur_txq++;
			ND("exit at slot %d", next_i);
		}
        }
        if (peer->cur_txq > 0)
                netmap_fwd(peer);
        if (port->cur_txq > 0)          // WANT_RUN
                netmap_fwd(port);
        ND("done");
}
/*-----------------------------------------------------------------------*/
void
run_sessions(fd_set *r, fd_set *w, my_netmap_port *p1, my_netmap_port *p2)
{
	int fd1, fd2;

	fd1 = p1->d->fd;
	fd2 = p2->d->fd;

	if (FD_ISSET(fd1, r) || FD_ISSET(fd1, w))
		netmap_read(p1, p2);
	if (FD_ISSET(fd2, r) || FD_ISSET(fd2, w))
		netmap_read(p2, p1);
}
/*-----------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
	my_netmap_port *p1, *p2;
	
	p1 = netmap_add_port("valeA:f");
	p2 = netmap_add_port("valeB:f");

	while (1) {
		int n;
		fd_set r, w;
		struct timeval delta = {0, 1000000/1000};
		
		n = set_sessions(&r, &w, p1, p2);
		select(n, &r, &w, NULL, &delta);
		run_sessions(&r, &w, p1, p2);
	}

	UNUSED(argc);
	UNUSED(argv);
	return EXIT_SUCCESS;
}
/*-----------------------------------------------------------------------*/
