#ifndef __PKT_ENGINE_H__
#define __PKT_ENGINE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for queue management */
#include "queue.h"
/* for io_module_funcs */
#include "io_module.h"
/* for pthreads */
#include <pthread.h>
/* for file I/O */
#include <stdio.h>
/* for Element def'n */
#include "element.h"
/*---------------------------------------------------------------------*/
/**
 *  io_type: Right now, we only support IO_NETMAP.
 */
typedef enum io_type {
	IO_NETMAP, IO_DPDK, IO_PFRING, IO_PSIO, IO_LINUX, IO_FILE
} io_type;
/* the default is set to IO_NETMAP */
#define IO_DEFAULT		IO_NETMAP
/*---------------------------------------------------------------------*/
/**
 *
 * PACKET ENGINE INTERFACE
 *
 * This is a template for the packet engine. More comments will be added
 * in the future... as the interface is matured further.
 *
 */
/*---------------------------------------------------------------------*/
typedef struct engine {
	uint8_t run; 			/* the engine mode running/stopped */
	io_type iot;			/* type: currently only supports netmap */
	uint8_t *name;			/* the engine name will be used as an identifier */
	int8_t cpu;			/* the engine thread will be affinitized to this cpu */
	uint64_t byte_count;		/* total number of bytes seen by this engine */
	uint64_t pkt_count;		/* total number of packets seen by this engine */
	uint64_t pkt_dropped;		/* total number of packets dropped by this engine */
	int32_t dev_fd;			/* file desc of the net I/O */
	int32_t listen_fd;		/* listening socket fd */
	uint16_t listen_port;		/* listening port */

	struct io_module_funcs iom;	/* io_funcs ptrs */
	void *private_context;		/* I/O-related context */
	pthread_t t;

	Element *elem;			/* list of attached elements */
	uint8_t mark_for_copy;		/* marking for copy */
	uint64_t seed;			/* seed for hashing in pipelining mode */

	/*
	 * the linked list ptr that will chain together
	 * all the engines (for network_interface.c) 
	 */
	TAILQ_ENTRY(engine) if_entry; 

	/*
	 * the linked list ptr that will chain together 
	 * all the engines (for pkt_engine.c) 
	 */
	TAILQ_ENTRY(engine) entry; 
} engine __attribute__((aligned(__WORDSIZE)));

typedef TAILQ_HEAD(elist, engine) elist;
/*---------------------------------------------------------------------*/
/**
 * Creates a new pkt_engine for packet sniffing
 *
 */
void
pktengine_new(const unsigned char *name, const unsigned char *type, 
		      const int8_t cpu);

/**
 * Deletes the pkt_engine
 *
 */
void
pktengine_delete(const unsigned char *name);

/**
 * Register an iface to the pkt engine
 *
 */
void
pktengine_link_iface(const unsigned char *name, 
		     const unsigned char *iface,
		     const int16_t batch_size,
		     const int8_t queue);

/**
 * Unregister the iface from the pkt engine
 *
 */
void
pktengine_unlink_iface(const unsigned char *name,
		       const unsigned char *iface);

/**
 * Start the engine
 *
 */
void
pktengine_start(const unsigned char *name);

/**
 * Stop the engine
 */
void
pktengine_stop(const unsigned char *name);

/**
 * Print current traffic statistics
 */
void
pktengine_dump_stats(const unsigned char *name);

/**
 * Print all engines' traffic stats to the given file object
 */
void
pktengines_list_stats(FILE *f);
				  
/**
 * Initializes the engine module
 */
void
pktengine_init();
				  
/**
 * Checks whether a given engine is online
 */
uint8_t
is_pktengine_online(const unsigned char *eng_name);
				  
/**
 * Creates a channel through which userland applications
 * communicate with the system
 */
int32_t
pktengine_open_channel(const unsigned char *eng_name, 
		       const unsigned char *from_channel_name,
		       const unsigned char *to_channel_name,
		       const unsigned char *action_name);

/**
 *  Drop the pkts coming to this engine
 */
int32_t
pktengine_drop_pkts(const unsigned char *eng_name);

/**
 *  Redirect the pkts coming to this engine to an outgoing interface
 */
int32_t
pktengine_redirect_pkts(const unsigned char *eng_name,
			const unsigned char *ifname);

/**
 * Search for a registered engine in the engine list
 */
engine *
engine_find(const unsigned char *name);
/*---------------------------------------------------------------------*/
#endif /* !__PKT_ENGINE_H__ */
