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
#ifndef __BACKEND_H__
#define __BACKEND_H__
/*---------------------------------------------------------------------*/
/* pktengine def'n */
#include "pkt_engine.h"
/*---------------------------------------------------------------------*/
/**
 *
 * NETWORK BACKEND OF BRICKS
 *
 * This module handles the network communications of the system.
 * meaning... it handles NIC I/O as well as apps I/O
 *
 */
/*---------------------------------------------------------------------*/
/**
 * MACROS
 */
/* Sets the limit on how many processes each engine can handle */
/* __linux__ */
#define EPOLL_MAX_EVENTS		FD_SETSIZE
/* __FreeBSD__ */
#define KQUEUE_MAX_EVENTS		EPOLL_MAX_EVENTS
/* epoll timeout = 500ms */
#define EPOLL_TIMEOUT			500
/* passive sock port */
#define PKTENGINE_LISTEN_PORT		1234
/* listen queue length */
#define LISTEN_BACKLOG			10
/*---------------------------------------------------------------------*/
/**
 * Creates listening socket to serve as a conduit between userland
 * applications and the system. Starts listening on all sockets 
 * thereafter.
 */
void
initiate_backend(engine *eng);

/**
 * Connects to a bricks server for remote shell commands
 * rshell_args contains either NULL (use default) or
 * string "ipaddr:port"
 */
int
connect_to_bricks_server(char *rshell_args);

/**
 * Converts given interface to promiscuous mode
 */
void
promisc(const char *iface);
/*---------------------------------------------------------------------*/
#endif /* __BACKEND_H__ */
