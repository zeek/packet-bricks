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
/* for Brick def'n */
#include "brick.h"
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
/* Declaring struct source for engine_src */
struct engine_src;
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
	int32_t listen_fd;		/* listening socket fd */
	uint16_t listen_port;		/* listening port */

	struct io_module_funcs iom;	/* io_funcs ptrs */
	pthread_t t;
	struct engine_src **esrc;	/* list of sources connected to the engine */
	uint no_of_sources;		/* no. of engine sources */
	uint8_t mark_for_copy;		/* marking for copy */
	int32_t buffer_sz;		/* buffer sizes in between each brick */

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
 *
 * PACKET ENGINE SOURCE
 *
 * Basic context of sources through which the engine talks to the
 * netmap I/O framework...
 */
typedef struct engine_src {
	int32_t dev_fd;			/* file desc of the net I/O */
	Brick *brick;			/* list of attached bricks */
	void *private_context;		/* I/O-related contexts */
} engine_src;
/*---------------------------------------------------------------------*/
/**
 * Creates a new pkt_engine for packet sniffing
 *
 */
void
pktengine_new(const unsigned char *name, 
	      const int32_t buffer_sz,
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
