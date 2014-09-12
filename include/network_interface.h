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
#ifndef __NETWORK_INTERFACE_H__
#define __NETWORK_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* to access engine struct */
#include "pkt_engine.h"
/*---------------------------------------------------------------------*/
/**
 * NETIFACE
 *
 * This structure is used to hold basic properties of the network
 * interface that gets registered by the pktengine. It is also used
 * to put various configuration checks e.g. to deny an engine from
 * registering 1 interface more than once...
 */
/*---------------------------------------------------------------------*/
typedef struct netiface {
	unsigned char *ifname;		/* the name of the interface */
	uint8_t multiqueue_enabled;	/* check if it will receive packets via hardware queues */
	io_type iot;			/* I/O type */
	void *context;			/* some I/O libs (e.g. netmap) need private contexts */
	elist registered_engines;	/* list of engines that have registered this iface */

	TAILQ_ENTRY(netiface) entry;
} netiface __attribute__((aligned(__WORDSIZE)));

typedef TAILQ_HEAD(nlist, netiface) nlist;
enum {NO_QUEUES = 0, HW_QUEUES};
/*---------------------------------------------------------------------*/
/**
 * initializes the niface_list...
 */
void
interface_init();

/**
 * find the right interface
 */
netiface *
interface_find(const char *iface);
				    
/**
 * Create new interface entry
 */
netiface *
create_interface_entry(const unsigned char *iface,
		       unsigned char queues_enabled,
		       io_type iot,
		       void *ctxt,
		       engine *eng);

/**
 *
 * Register engine with an already tagged interface
 *
 */
void *
retrieve_and_register_interface_entry(const unsigned char *iface, 
				      unsigned char queues_enabled, 
				      io_type io,
				      engine *eng);

/**
 * Unregister engine from the interface. If the interface object
 * has only one registered engine, then free the interface as well.
 */
void
unregister_interface_entry(const unsigned char *iface,
			   engine *eng);

/**
 * Unregister engine from all the interfaces.
 */
void
unregister_all_interfaces(engine *eng);
/*---------------------------------------------------------------------*/
#endif /* !__NETWORK_INTERFACE_H__ */
