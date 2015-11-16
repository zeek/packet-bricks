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
/* for prints etc */
#include "bricks_log.h"
/* for string functions */
#include <string.h>
/* for io modules */
#include "io_module.h"
/* for accessing pc_info variable */
#include "main.h"
/* for function prototypes */
#include "backend.h"
/* for kqueue */
#include <sys/event.h>
#include <sys/types.h>
#include <sys/time.h>
/* for socket */
#include <sys/socket.h>
/* for sockaddr_in */
#include <netinet/in.h>
/* for inet_addr */
#include <arpa/inet.h>
/* for requests/responses */
#include "bricks_interface.h"
/* for remote shell defines */
#include "lua_interpreter.h"
/* for errno */
#include <errno.h>
/* for polling */
#include <sys/poll.h>
/* for open/close */
#include <unistd.h>
/* for assert */
#include <assert.h>
/* for likely/unlikely */
#include "pkt_hash.h"
/* for install_filter */
#include "netmap_module.h"
/*---------------------------------------------------------------------*/
/**
 * registers fd in the poll fd set
 */
#define __register_fd(s, list) {					\
		register int idx = -1;					\
		while (likely(idx < POLL_MAX_EVENTS) &&			\
		       list[++idx].fd != -1);				\
		assert(idx < POLL_MAX_EVENTS);				\
		/* register client's socket */				\
		list[idx].fd = s;					\
		list[idx].events = POLLIN | POLLRDNORM;			\
		list[idx].revents = 0;					\
		TRACE_DEBUG_LOG("Accepted connection with sockid: "	\
				"%d at idx: %d\n", s, idx);		\
	}
/*---------------------------------------------------------------------*/
/**
 * unregisters fd in the poll fd set and closes the socket
 */
#define __unregister_fd(idx, list) {					\
		close(list[idx].fd);					\
		TRACE_DEBUG_LOG("Closing connection with sockid: "	\
				"%d at idx: %d\n", list[idx].fd, idx);	\
		list[idx].fd = -1;					\
	}
/*---------------------------------------------------------------------*/
/**
 * connect shell to the daemon
 * (is only called when bricks is run as daemon).
 */
int
connect_to_bricks_server(char *rshell_args)
{
	TRACE_BACKEND_FUNC_START();
	int sock;
	struct sockaddr_in server;
	char *ip_addr, *port_str;
	uint16_t port;

	if (rshell_args == NULL)
		ip_addr = port_str = NULL;
	else {
		/* first parse the ipaddr and port string */
		ip_addr = strtok(rshell_args, ":");
		port_str = strtok(NULL, ":");
	}
	port = (port_str == NULL) ? BRICKS_LISTEN_PORT : atoi(port_str);

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		TRACE_ERR("Could not create socket: %s\n", 
			  strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return -1;
	}
	TRACE_DEBUG_LOG("Socket created");
     
	server.sin_addr.s_addr = (ip_addr == NULL) ? 
		inet_addr("127.0.0.1") : inet_addr(ip_addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
 
	/* Connect to remote server */
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		TRACE_ERR("Connect failed!: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return -1;
	}
     
	TRACE_DEBUG_LOG("Connected\n");
	TRACE_BACKEND_FUNC_END();

	return sock;
}
/*---------------------------------------------------------------------*/
void
start_listening_reqs()
{
	TRACE_BACKEND_FUNC_START();
	/* socket info about the listen sock */
	struct sockaddr_in serv; 
	int listen_fd, client_sock;
	struct kevent evlist[KQUEUE_MAX_EVENTS];
	struct kevent evSet;
	int kq, events, n;

	/* zero the struct before filling the fields */
	memset(&serv, 0, sizeof(serv));
	/* set the type of connection to TCP/IP */           
	serv.sin_family = AF_INET;
	/* set the address to any interface */                
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 
	/* set the server port number */    
	serv.sin_port = htons(BRICKS_LISTEN_PORT);

	/* create bricks socket for listening remote shell requests */
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		TRACE_ERR("Failed to create listening socket for bricks\n");
		TRACE_BACKEND_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
		TRACE_ERR("Failed to bind listening socket to port %d for bricks\n",
			  BRICKS_LISTEN_PORT);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (for bricks)\n",
			  BRICKS_LISTEN_PORT);
		TRACE_BACKEND_FUNC_END();
	}

	/* set up the kqueue structure */
	kq = kqueue();
	if (kq == -1) {
		TRACE_ERR("kqueue error: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
	}
	EV_SET(&evSet, listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1) {
		TRACE_ERR("kevent error: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
	}

	/*
	 * Main loop that processes incoming remote shell commands
	 * if the remote request arrives at listen_fd, it accepts connections
	 * and creates a new 'shell'
	 * if the remote request comes from a client request, it calls
	 * the lua string command function to execute the command
	 */

	do {
		events = kevent(kq, NULL, 0, evlist, KQUEUE_MAX_EVENTS, NULL);
		if (events == -1 && errno != EINTR) {
			TRACE_ERR("kevent error: %s\n", strerror(errno));
			TRACE_BACKEND_FUNC_END();
		}
		for (n = 0; n < events; n++) {
			/* the request is for a new shell */
			if ((int)evlist[n].ident == listen_fd) {
				client_sock = accept(listen_fd, NULL, NULL);
				if (client_sock < 0) {
					TRACE_ERR("accept failed: %s\n", strerror(errno));
					TRACE_BACKEND_FUNC_END();
				}
				/* add client_sock and listen_fd for kqueueing again */
				EV_SET(&evSet, client_sock, EVFILT_READ,
				       EV_ADD | EV_ENABLE, 0, 0, NULL);
				if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1) {
					TRACE_ERR("kevent error: %s\n", strerror(errno));
					TRACE_BACKEND_FUNC_END();
				}
				lua_kickoff(LUA_EXE_STR, &client_sock);
			} else { /* evlist[n].ident == regular sock */
				/* if the socket sees an error.. close the socket. */
				int s = evlist[n].ident;
				if ((evlist[n].flags & EV_ERROR) ||
				    (evlist[n].flags & EV_EOF)) { 
					EV_SET(&evSet, s, EVFILT_READ,
					       EV_DELETE, 0, 0, NULL);
					if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1) {
						TRACE_ERR("kevent error: %s\n", strerror(errno));
						TRACE_BACKEND_FUNC_END();
					}
					close(s);
				}
				/* 
				 * else if the client socket has incoming read, 
				 * this means we are getting some lua commands.... 
				 * execute them 
				 */
				else
					lua_kickoff(LUA_EXE_STR, &s);
			}
		}
	} while (1);

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/*
 * Code self-explanatory...
 */
static void
create_listening_socket_for_eng(engine *eng)
{
	TRACE_BACKEND_FUNC_START();

	/* socket info about the listen sock */
	struct sockaddr_in serv; 
 
	/* zero the struct before filling the fields */
	memset(&serv, 0, sizeof(serv));
	/* set the type of connection to TCP/IP */           
	serv.sin_family = AF_INET;
	/* set the address to any interface */                
	serv.sin_addr.s_addr = htonl(INADDR_ANY); 
	/* set the server port number */    
	serv.sin_port = htons(PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd);
 
	eng->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (eng->listen_fd == -1) {
		TRACE_ERR("Failed to create listening socket for engine %s\n",
			  eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(eng->listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
	  TRACE_ERR("Failed to bind listening socket to port %d of engine %s\n",
		    PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd, eng->name);
	  TRACE_BACKEND_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(eng->listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (engine %s)\n",
			  PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd, eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	eng->listen_port = PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd;
	TRACE_LOG("Engine listening on port: %u\n", eng->listen_port);
	
	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Accepts connection for connecting to the backend...
 */
static int
accept_request_backend(engine *eng, int sock)
{
	TRACE_BACKEND_FUNC_START();
	int clientsock;
	struct sockaddr_in client;
	socklen_t c;

	if (sock == eng->listen_fd) {
		/* accept connection from an incoming client */
		clientsock = accept(eng->listen_fd,
				    (struct sockaddr *)&client, &c);
		if (clientsock < 0) {
			TRACE_LOG("accept failed: %s\n", strerror(errno));
			TRACE_BACKEND_FUNC_END();
			return -1;
		}
		TRACE_BACKEND_FUNC_END();
		return clientsock;
	}
	
	TRACE_BACKEND_FUNC_END();
	return -1;
}
/*---------------------------------------------------------------------*/
/**
 * Services incoming request from userland applications and takes
 * necessary actions. The actions can be: (i) passing packets to 
 * userland apps etc.
 */
static int
process_request_backend(int sock, engine *eng)
{
	TRACE_BACKEND_FUNC_START();
	char buf[LINE_MAX];
	ssize_t read_size;
	req_block *rb;
	
	if ((read_size = read(sock, buf, sizeof(buf))) <
	    (ssize_t)(sizeof(req_block))) {
		TRACE_DEBUG_LOG("Request block malformed! (size: %d),"
			  " request block should be of size: %d\n",
			  (int)read_size, (int)sizeof(req_block));
		return -1;
	} else {
		rb = (req_block *)buf;
		TRACE_DEBUG_LOG("Total bytes read from client socket: %d\n", 
			  (int)read_size);
		
		install_filter(rb, eng);
		/* parse new rule */
		TRACE_DEBUG_LOG("Got a new rule\n");
		TRACE_DEBUG_LOG("Target: %d\n", rb->t);
		TRACE_DEBUG_LOG("TargetArgs.pid: %d\n", rb->targs.pid);
		TRACE_DEBUG_LOG("TargetArgs.proc_name: %s\n", rb->targs.proc_name);
		TRACE_FLUSH();
		
		return 0;
	}	
	TRACE_BACKEND_FUNC_END();
	return -1;
}
/*---------------------------------------------------------------------*/
/**
 * Creates listening socket to serve as a conduit between userland
 * applications and the system. Starts listening on all sockets 
 * thereafter.
 */
void
initiate_backend(engine *eng)
{
	TRACE_BACKEND_FUNC_START();

	struct pollfd pollfd[POLL_MAX_EVENTS];
	uint pollfds;
	int i;
	uint dev_flag, j;

	/* initializing main while loop parameters */
	dev_flag = 0;
	pollfds = 0;
	memset(pollfd, -1, sizeof(pollfd));

	/* create listening socket */
	create_listening_socket_for_eng(eng);

	TRACE_DEBUG_LOG("Engine %s is listening on port %d\n", 
			eng->name, eng->listen_port);

	/* adjust pcapr context in engine */
	if (!strcmp(eng->FIRST_BRICK(esrc)->brick->elib->getId(), "PcapReader")) {
		eng->pcapr_context = eng->FIRST_BRICK(esrc)->brick->private_data;		
	}

	/* register iom socket */
	for (j = 0; j < eng->no_of_sources; j++) {
		__register_fd(eng->esrc[j]->dev_fd, pollfd);
		pollfds++;
	}

	/* register eng's listening socket */
	__register_fd(eng->listen_fd, pollfd);
	pollfds++;
	
	/* keep on running till engine stops */
	while (eng->run == 1) {
		/* pcap handling.. */
		if (eng->pcapr_context != NULL)
			process_pcap_read_request(eng, eng->pcapr_context);
		else { /* get input from interface */
			int source_flag;
			i = poll(pollfd, POLL_MAX_EVENTS, POLL_TIMEOUT);
			
			/* if no packet came up, try polling again */
			if (i <= 0) continue;
			
			for (i = 0; i < POLL_MAX_EVENTS; i++) {
				if (pollfd[i].fd == -1)
					continue;
				/* reset the source flag */
				source_flag = 0;
				/* process iom fds */
				for (j = 0; j < eng->no_of_sources; j++) {
					if (eng->esrc[j]->dev_fd == pollfd[i].fd) {
						eng->iom.callback(eng->esrc[j]);
						source_flag = 1;
						break;
					}
				}

				if (source_flag == 1) continue;
				/* process listening requests */
				if (pollfd[i].fd == eng->listen_fd) {
					if ((pollfd[i].revents & POLLIN) &&
					    pollfds < POLL_MAX_EVENTS) {
						int sock = accept_request_backend(eng, pollfd[i].fd);
						if (sock != -1) {
							__register_fd(sock, pollfd);
							pollfds++;
						}
					}
				} else {
					if ((pollfd[i].revents & POLLIN)) {
						if (process_request_backend(pollfd[i].fd, eng) == -1) {
							__unregister_fd(i, pollfd);
							pollfds--;
						}
					} else if ((pollfd[i].revents & (POLLERR | POLLHUP | POLLNVAL))) {
						__unregister_fd(i, pollfd);
						pollfds--;
					}
				}
			}
		}
	}

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
