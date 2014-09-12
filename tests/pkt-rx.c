/* for std I/O */
#include <stdio.h>
#define NETMAP_WITH_LIBS
/* for netmap */
#include "net/netmap_user.h"
/* for int*_t datatypes */
#include <sys/types.h>
/* for libc funcs */
#include <stdlib.h>
#include <poll.h>
/*-----------------------------------------------------------------------*/
#define OUTPUT_RINGS			1
#define UNUSED(x)			(void)x
uint32_t dropped = 0;
uint32_t forwarded = 0;
/*-----------------------------------------------------------------------*/
void
pkt_swap(struct netmap_slot *ts, struct nm_desc *d)
{
        u_int c;
        u_int n = d->last_tx_ring - d->first_tx_ring + 1;
	
        for (c = 0; c < n ; c++) {
		/* compute current ring to use */
		struct netmap_ring *ring;
		struct netmap_slot *rs;
		
		uint32_t ri = d->cur_tx_ring + c;
		
		if (ri > d->last_tx_ring)
			ri = d->first_tx_ring;
		ring = NETMAP_TXRING(d->nifp, ri);
		if (nm_ring_empty(ring))  {
			//RD(5, "no room to transmit on ring %d @ %s (tx_slots %d - tx_slots_pending %d - nm_ring_space%d)!", c, d->req.nr_name, d->req.nr_tx_slots, nm_tx_pending(ring), nm_ring_space(ring));
			++dropped;
			//RD(10, "Dropped packets %llu", dropped);
			continue;
		}
		//u_int c, n = d->last_tx_ring - d->first_tx_ring + 1;
		//for (c = 0; c < n ; c++) 
		//      {
		//      if ( nm_ring_empty(txring) )
		//              {
		//              continue;
		//              }
		//
		////dump_payload(p, rs->len, rxring, rxring->cur);
		//
		////D("output port: %d", output_port);
		//struct netmap_slot *ts = &txring->slot[txring->cur];
		//
		//if (ts->buf_idx < 2 || rs->buf_idx < 2)
		//      {
		//      D("wrong index rx[%d] = %d  -> tx[%d] = %d", rxring->cur, rs->buf_idx, txring->cur, ts->buf_idx);
		//      sleep(2);
		//      }
		///* copy the packet length. */
		//if (rs->len > 2048) 
		//      {
		//      D("wrong len %d rx[%d] -> tx[%d]", rs->len, rxring->cur, txring->cur);
		//      rs->len = 0;
		//      }
		
		++forwarded;
		//RD(10, "Forwarded packets %llu", forwarded);
		
		//D("swap packets");
		rs = &ring->slot[ring->cur];
		ts->len = rs->len;
		uint32_t pkt = ts->buf_idx;
		ts->buf_idx = rs->buf_idx;
		rs->buf_idx = pkt;
		/* report the buffer change. */
		ts->flags |= NS_BUF_CHANGED;
		rs->flags |= NS_BUF_CHANGED;
		ring->head = ring->cur = nm_ring_next(ring, ring->cur);
	}
}
/*-----------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
	struct nm_desc *rxnmd = NULL;
	struct nm_desc *txnmds[OUTPUT_RINGS];
	
	struct netmap_ring *txrings[OUTPUT_RINGS];

	struct netmap_ring *rxring = NULL;
	struct netmap_ring *txring  = NULL;

	u_int received = 0;
	u_int cur, rx, n;
	int ret;
	
	struct nmreq base_req;
	bzero(&base_req, sizeof(base_req));
	// Make the right number of pipes available.
	//base_req.nr_arg1 = OUTPUT_RINGS;
	base_req.nr_tx_slots = 1000;
	base_req.nr_rx_slots = 1000;
	base_req.nr_arg3 = 100000;
	//base_req.req.nr_arg3 = 8;
       
	
	rxnmd = nm_open("netmap:em0", &base_req, NM_OPEN_ARG1|NM_OPEN_RING_CFG, NULL);
	if (rxnmd == NULL)
		{
		printf("cannot open %p", rxnmd);
		return (1);
		}
	else
		{
			fprintf(stderr, "successfully opened %p (tx rings: %d\n", rxnmd, rxnmd->req.nr_tx_slots);
		}

	int i;
	for ( i=0; i<OUTPUT_RINGS; ++i )
		{
		char interface[25];
		sprintf(interface, "netmap:em0{%d", i);
		fprintf(stderr, "opening pipe named %s\n", interface);
		uint64_t flags = NM_OPEN_NO_MMAP;// | NM_OPEN_ARG3;// | NM_OPEN_RING_CFG;

		txnmds[i] = nm_open(interface, NULL, flags, rxnmd);
		if (txnmds[i] == NULL)
			{
			printf("cannot open %p\n", txnmds[i]);
			return (1);
			}
		else
			{
				fprintf(stderr, "successfully opened pipe #%d %p (tx slots: %d)\n", i, txnmds[i], txnmds[i]->req.nr_tx_slots);
			// Is this right?  Do pipes only have one ring?
			txrings[i] = NETMAP_TXRING(txnmds[i]->nifp, 0);
			}
		fprintf(stderr, "zerocopy %s", (rxnmd->mem == txnmds[i]->mem) ? "enabled\n" : "disabled\n");
		}

	sleep(2);

	struct pollfd pollfd[OUTPUT_RINGS+1];
	memset(pollfd, 0, sizeof(pollfd));

	//signal(SIGINT, sigint_h);
	while (1)
		{
			//u_int m = 0;
		uint polli = 0;

		for ( i = 0; i<OUTPUT_RINGS; ++i )
			{
			//if ( nm_ring_space(txrings[i]) < 100 )
			//printf("%d slots available on TX port #%d", nm_ring_space(txrings[i]), i);

			//if( 1 )
			if ( nm_tx_pending(txrings[i]) > 0 )
				{
				//Rprintf(5, "polling non-empty txring %d", i);
				pollfd[polli].fd = txnmds[i]->fd;
				pollfd[polli].events = POLLOUT | POLLWRNORM;
				pollfd[polli].revents = 0;
				++polli;
				}
			//else
			//	{
			//	pollfd[polli].fd = -1;
			//	}
			}

		// if we can't write anywhere, we shouldn't read.
		//if ( polli > 0 )
		if ( 1 )
			{
			pollfd[polli].fd = rxnmd->fd;
			pollfd[polli].events = POLLIN | POLLRDNORM;
			pollfd[polli].revents = 0;
			++polli;
			}

		//Rprintf(5, "polling %d file descriptors", polli+1);
		i = poll(pollfd, polli+1, 2500);
		if ( i <= 0 )
			{
				//printf("poll error/timeout  %s", strerror(errno));
			continue;
			}
		else
			{
			//Rprintf(5, "Poll returned %d", i);
			}

		//if (pollfd[0].revents & POLLERR)
		//	{
		//	struct netmap_ring *rx = NETMAP_RXRING(rxnmd->nifp, rxnmd->cur_rx_ring);
		//	printf("error on fd0, rx [%d,%d,%d)",
		//		rx->head, rx->cur, rx->tail);
		//	}
		//if (pollfd[1].revents & POLLERR) 
		//	{
		//	struct netmap_ring *tx = NETMAP_TXRING(tx1nmd->nifp, tx1nmd->cur_tx_ring);
		//	printf("error on fd1, tx [%d,%d,%d)",
		//		tx->head, tx->cur, tx->tail);
		//	}
		
		for ( i = rxnmd->first_rx_ring; i <= rxnmd->last_rx_ring; i++ ) 
			{
			rxring = NETMAP_RXRING(rxnmd->nifp, i);

			//printf("prepare to scan rings");
			while ( !nm_ring_empty(rxring) )
				{
				struct netmap_slot *rs = &rxring->slot[rxring->cur];

				// CHOOSE THE CORRECT OUTPUT PIPE
				//const char *p = NETMAP_BUF(rxring, rs->buf_idx);
				//uint32_t output_port = master_custom_hash_function(p, rs->len) % OUTPUT_RINGS;

				// Move the packet to the output pipe.
				pkt_swap(rs, txnmds[0]);

				rxring->head = rxring->cur = nm_ring_next(rxring, rxring->cur);
				}
			}
		}

	printf("exiting");

	nm_close(rxnmd);
	for ( i=0; i<OUTPUT_RINGS; ++i )
		{
		nm_close(txnmds[i]);
		}

	UNUSED(argc);
	UNUSED(argv);
	UNUSED(n);
	UNUSED(ret);
	UNUSED(rx);
	UNUSED(cur);
	UNUSED(received);
	UNUSED(txring);
	return EXIT_SUCCESS;
}
/*-----------------------------------------------------------------------*/
