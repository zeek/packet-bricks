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
#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for target and rule def'ns */
#include "bricks_interface.h"
/* for rule def'n */
#include "brick.h"
/*---------------------------------------------------------------------*/
#define MAX_IFNAMELEN		64
/*---------------------------------------------------------------------*/
/**
 * io_module_funcs - contains template for the various 10Gbps pkt I/O
 * 		   - libraries that can be adopted. Please see netmap_module.c
 *		   - as reference...
 *
 *		   init_context(): Used to initialize the driver library
 *				 : Also use the context to create/initialize
 *				 : a private packet I/O context for the engine
 *
 *		   link_iface(): Used to add interface to the engine. Some
 *				 Some pkt I/O libraries can pass more than
 *				 iface to one engine (e.g. PSIO)
 *
 *		   unlink_ifaces(): Used to remove interface(s) from the engine.
 *				  Call it only when the engine is not running
 *
 *		   callback(): Function that reads the packets and runs 
 *				appropriate handler
 *
 *		   delete_all_channels(): Function that tears down all channels
 *					 associated with a particular rule
 *
 *		   add_filter(): Function that adds filter to the netmap
 *				 framework
 *
 *		   create_external_link(): Function that establishes external
 *				 	   link with userland processes
 *
 *		   shutdown(): Used to destroy the private pkt I/O-specifc 
 *			       context
 */
/*---------------------------------------------------------------------*/
typedef struct io_module_funcs {
	int32_t (*init_context)(void **ctxt, void *engptr);
	int32_t (*link_iface)(void *ctxt,
			      const unsigned char *iface, 
			      const uint16_t batchsize,
			      int8_t qid);
	void	(*unlink_ifaces)(void *engptr);
	int32_t (*callback)(void *engsrcptr);
	void	(*delete_all_channels)(Brick *brick);
	int32_t (*create_external_link)(char *in_name, char *out_name, 
					Target t, void *esrcptr);
	int32_t (*shutdown)(void *engptr);
	
} io_module_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/* only netmap module is enabled at the moment */
extern io_module_funcs netmap_module;
/*---------------------------------------------------------------------*/
#endif /* !__IO_MODULE_H__ */
