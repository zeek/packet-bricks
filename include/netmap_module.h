#ifndef __NETMAP_MODULE_H__
#define __NETMAP_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for engine definition */
#include "pkt_engine.h"
/* for pipelined rule insertions */
#include "rule.h"
/* for netmap specific decls */
#define  NETMAP_WITH_LIBS		1
#include "net/netmap_user.h"
/*---------------------------------------------------------------------*/
/* Macros related to TXQ entry */
#define TXQ_MAX				2048
/*---------------------------------------------------------------------*/
struct txq_entry {
        void *ring;
        uint16_t slot_idx;      /* used if ring */
};

typedef struct CommNode {
	struct nm_desc *out_nmd;		/* Node-local pipe descriptor */
	char nm_ifname[IFNAMSIZ];		/* name of the node */
	struct txq_entry q[TXQ_MAX];		/* transmission queue used to buffer descs */
	int32_t cur_txq;			/* current index of the tx entry */
	struct Rule *r;				/* 
						 * pointer to the child Rule  
						 * (in case the caller enables pipelining)
						 */
} CommNode;

/**
 * Private per-engine netmap module context. Please see the comments 
 * inside the struct for more details
 *
 */
typedef struct netmap_module_context {
	struct nm_desc *local_nmd;		/* thread-local netmap descriptor */
	uint16_t batch_size;			/* burst size */
	int32_t local_fd;			/* thread-local fd*/
	engine *eng;				/* ptr to host engine */
	
} netmap_module_context __attribute__((aligned(__WORDSIZE)));

/**
 * System-wide netmap-specific context
 */
typedef struct netmap_iface_context {
	struct nm_desc *global_nmd;		/* global netmap descriptor */	
	struct nm_desc base_nmd;		/* netmap base struct */
	int32_t global_fd;			/* global fd for ioctl etc. */
	uint64_t nmd_flags;			/* netmap desc flags */
} netmap_iface_context __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
#define TX_RETRIES			5
/* for more buffering */
#define NM_EXTRA_BUFS			8
/* for interface initialization */
#define NETMAP_LINK_WAIT_TIME		2	/* in secs */
/*---------------------------------------------------------------------*/
#endif /* !__NETMAP_MODULE_H__ */
