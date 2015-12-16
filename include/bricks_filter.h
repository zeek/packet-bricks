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
/*---------------------------------------------------------------------*/
#ifndef __BRICKS_FILTER_H__
#define __BRICKS_FILTER_H__
/*---------------------------------------------------------------------*/
/* for polling */
#include <sys/poll.h>
/* for flist */
#include "netmap_module.h"
/*---------------------------------------------------------------------*/
#define INET_MASK			32
#ifdef ENABLE_BROKER
#define BROKER_NODE			"node0"
#define BROKER_TOPIC			"bro/event"
#define BROKER_HOST			"127.0.0.1"
#define BROKER_PORT			9999
#endif
/*---------------------------------------------------------------------*/
struct FilterContext {
	/* name of output node */
	char name[IFNAMSIZ];
	/* 
	 * the linked list ptr that will chain together
	 * all filter bricks (for bricks_filter.c). this
	 * will be used for network communication module
	 */
	TAILQ_ENTRY(FilterContext) entry;

	/* Filter list */
	flist filter_list;
	
} __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * Analyze the packet across the filter. And pass it along
 * the communication node, if the filter allows...
 */
int
#if 1
analyze_packet(unsigned char *buf, CommNode *cn, time_t t);
#else
analyze_packet(unsigned char *buf, FilterContext *cn, time_t t);
#endif

/**
 * Add the filter to the selected CommNode
 */
int
#if 1
apply_filter(CommNode *cn, Filter *f);
#else
apply_filter(FilterContext *cn, Filter *f);
#endif

/**
 * Initialize the filter communication backend
 */
void
initialize_filt_comm(engine *eng);

/**
 * Terminate the filter communication backend
 */
void
terminate_filt_comm(engine *eng);

/**
 * Retrieve the socket descriptor of the broker_message_queue
 * that is present in the engine object
 */
int
fetch_filter_comm_fd(engine *eng);

/**
 * Parse and interpret the incoming filter broker request
 */
void
process_broker_request(engine *eng, struct pollfd *pfd, int i);
/*---------------------------------------------------------------------*/
#endif /* __BRICKS_FILTER_H__ */
