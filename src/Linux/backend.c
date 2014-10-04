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
/* for epoll */
#include <sys/epoll.h>
/* for socket */
#include <sys/socket.h>
/* for sockaddr_in */
#include <netinet/in.h>
/* for requests/responses */
#include "bricks_interface.h"
/* for errno */
#include <errno.h>
/* for close (2) */
#include <unistd.h>
/* for remote shell defines */
#include "lua_interpreter.h"
/* for inet_addr */
#include <arpa/inet.h>
/*---------------------------------------------------------------------*/
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
	struct epoll_event ev, events[EPOLL_MAX_EVENTS];
	int epoll_fd, nfds, n;	

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

	/* set up the epolling structure */
	epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
	if (epoll_fd == -1) {
		TRACE_ERR("BRICKS failed to create an epoll fd!\n");
		TRACE_BACKEND_FUNC_END();
		return;
	}


	/* register listening socket */
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = listen_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
		TRACE_LOG("BRICKS failed to exe epoll_ctl for fd: %d\n",
			  epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	/*
	 * Main loop that processes incoming remote shell commands
	 * if the remote request arrives at listen_fd, it accepts connections
	 * and creates a new 'shell'
	 * if the remote request comes from a client request, it calls
	 * the lua string command function to execute the command
	 */

	do {
		/* wait for new epoll requests */
		nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, -1);
		if (nfds == -1) {
			if (errno == EINTR) continue;
			TRACE_ERR("BRICKS poll error: %s\n", strerror(errno));
			TRACE_BACKEND_FUNC_END();
		}
		/* got some request... now process each one of them */
		for (n = 0; n < nfds; n++) {
			/* the request is for a new shell */
			if (events[n].data.fd == listen_fd) {
				client_sock = accept(listen_fd, NULL, NULL);
				if (client_sock < 0) {
					TRACE_ERR("accept failed: %s\n", strerror(errno));
					TRACE_BACKEND_FUNC_END();
				}
				lua_kickoff(LUA_EXE_STR, &client_sock);
				/* add client_sock and listen_fd for epolling again */
				ev.data.fd = client_sock;
				ev.events = EPOLLIN;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
					TRACE_ERR("Can't register client sock for epolling!\n");
					TRACE_BACKEND_FUNC_END();
				}
				ev.data.fd = listen_fd;
				ev.events = EPOLLIN | EPOLLOUT;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
					TRACE_ERR("Can't register client sock for epolling!\n");
					TRACE_BACKEND_FUNC_END();
				}
			} else { /* events[n].data.fd == regular sock */
				/* 
				 * if the client socket has incoming read, this means we are getting
				 * some lua commands.... execute them 
				 */
				if (events[n].events == EPOLLIN || events[n].events == EPOLLPRI) {
					lua_kickoff(LUA_EXE_STR, &events[n].data.fd);
					ev.data.fd = events[n].data.fd;
					ev.events = EPOLLIN;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
						TRACE_ERR("Can't register client sock for epolling!\n");
						TRACE_BACKEND_FUNC_END();
					}
				} else { /* the only other epoll request is for error output.. close the socket then */
					ev.data.fd = events[n].data.fd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1) {
						TRACE_ERR("Can't register client sock for epolling!\n");
						TRACE_BACKEND_FUNC_END();
					}	
					close(ev.data.fd);
				}
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
	
	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * XXX - This function is being revised
 * Services incoming request from userland applications and takes
 * necessary actions. The actions can be: (i) passing packets to userland
 * apps etc.
 */
static void
process_request_backend(engine *eng, int epoll_fd)
{
	TRACE_BACKEND_FUNC_START();
	int client_sock, c, total_read;
	struct sockaddr_in client;
	int read_size;
	char client_message[2000];
	struct epoll_event ev;
	req_block *rb;
	resp_block respb;

	total_read = read_size = 0;
	respb.len = sizeof(resp_block);

	/* accept connection from an incoming client */
	client_sock = accept(eng->listen_fd, (struct sockaddr *)&client, (socklen_t *)&c);
	if (client_sock < 0) {
		TRACE_LOG("accept failed: %s\n", strerror(errno));
		TRACE_BACKEND_FUNC_END();
		return;
	}
     
	/* Receive a message from client */
	while ((read_size = recv(client_sock, 
				 &client_message[total_read], 
				 sizeof(req_block) - total_read, 0)) > 0) {
		total_read += read_size;
		if ((unsigned)total_read >= sizeof(req_block)) break;
	}

	TRACE_DEBUG_LOG("Total bytes read from listening socket: %d\n", 
			total_read);
	
	/* parse new rule */
	rb = (req_block *)client_message;
	TRACE_DEBUG_LOG("Got a new request\n");
	TRACE_DEBUG_LOG("\tLen: %u\n", rb->len);
	TRACE_DEBUG_LOG("\tInterface name: %s\n", rb->ifname);
	TRACE_FLUSH();

#if 0
	respb.flag = (process_filter_request(eng, rb->ifname, &rb->f) == -1) ?
		0 : 1;
#else
	(void)rb;
	respb.flag = 1;
#endif
	
	/* write the response back to the app */
	if (write(client_sock, &respb, sizeof(respb)) == -1) {
		TRACE_LOG("Write failed due to %s\n", 
			  strerror(errno));
	}

	/* add client sock to the polling layer */
	ev.data.fd = client_sock;
	ev.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	/* for the moment, close the client socket immediately */
	close(client_sock);

	/* continue listening */
	ev.data.fd = eng->listen_fd;
	ev.events = EPOLLIN | EPOLLOUT;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	TRACE_BACKEND_FUNC_END();
	return;
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
	struct epoll_event ev, events[EPOLL_MAX_EVENTS];
	int epoll_fd, nfds, n;
	uint i, dev_flag;

	dev_flag = 0;
	/* set up the epolling structure */
	epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
	if (epoll_fd == -1) {
		TRACE_LOG("Engine %s failed to create an epoll fd!\n", 
			  eng->name);
		TRACE_BACKEND_FUNC_END();
		return;
	}

	/* create listening socket */
	create_listening_socket_for_eng(eng);

	/* register listening socket */
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = eng->listen_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->listen_fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	TRACE_LOG("Engine %s is listening on port %d\n", 
		  eng->name, eng->listen_port);

	/* adjust pcapr context in engine */
	if (!strcmp(eng->FIRST_BRICK(esrc)->brick->elib->getId(), "PcapReader")) {
		eng->pcapr_context = eng->FIRST_BRICK(esrc)->brick->private_data;		
	}
	
	/* register iom socket */
	for (i = 0; i < eng->no_of_sources; i++) {
		ev.events = EPOLLIN;
		ev.data.fd = eng->esrc[i]->dev_fd;
		
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->esrc[i]->dev_fd, &ev) == -1) {
			TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
				  eng->name, epoll_fd);
			TRACE_BACKEND_FUNC_END();
			return;
		}
	}


	/* keep on running till engine stops */
	while (eng->run == 1) {
		nfds = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, EPOLL_TIMEOUT);
		if (nfds == -1) {
			TRACE_ERR("epoll error (engine: %s)\n",
				  eng->name);
			TRACE_BACKEND_FUNC_END();
		}
		for (n = 0; n < nfds; n++) {
			/* process dev work (check for all devs) */
			for (i = 0; i < eng->no_of_sources; i++) {
				if (events[n].data.fd == eng->esrc[i]->dev_fd) {
					eng->iom.callback(eng->esrc[i]);
					
					/* continue epolling */
					ev.data.fd = eng->esrc[i]->dev_fd;
					ev.events = EPOLLIN;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ev.data.fd, &ev) == -1) {
						TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
							  eng->name, epoll_fd);
						TRACE_BACKEND_FUNC_END();
						return;
					}
					dev_flag = 1;
				} 
			}

			if (dev_flag == 1) {
				dev_flag = 0;
				continue;
			}
			/* process app reqs */
			if (events[n].data.fd == eng->listen_fd)
				process_request_backend(eng, epoll_fd);
			else {/* all other reqs coming from client socks */
				ev.data.fd = events[n].data.fd;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1) {
					TRACE_ERR("Can't delete client sock for epolling!\n");
					TRACE_BACKEND_FUNC_END();
				}	
				close(ev.data.fd);
			}
		}
		/* pcap handling.. */
		if (eng->pcapr_context != NULL)
			process_pcap_read_request(eng, eng->pcapr_context);
	}

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
