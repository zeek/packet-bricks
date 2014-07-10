/* for prints etc */
#include "pacf_log.h"
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
#include "pacf_interface.h"
/*---------------------------------------------------------------------*/
/*
 * Code self-explanatory...
 */
static void
create_listening_socket(engine *eng)
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
	serv.sin_port = htons(LISTEN_PORT + eng->local_fd);
 
	eng->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (eng->listen_fd == -1) {
		TRACE_ERR("Failed to create listening socket for engine %s\n",
			  eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(eng->listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
		TRACE_ERR("Failed to bind listening socket to port %d of engine %s\n",
			  LISTEN_PORT + eng->local_fd, eng->name);
		TRACE_BACKEND_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(eng->listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (engine %s)\n",
			  LISTEN_PORT + eng->local_fd, eng->name);
		TRACE_BACKEND_FUNC_END();
	}

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Services incoming request from userland applications and takes
 * necessary actions. The actions can be: (i) passing packets to userland
 * apps etc.
 */
static void
process_request_backend(engine *eng, int epoll_fd)
{
	TRACE_BACKEND_FUNC_START();
	UNUSED(eng);
	UNUSED(epoll_fd);
	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
initiate_backend(engine *eng)
{
	TRACE_BACKEND_FUNC_START();
	struct epoll_event ev, events[EPOLL_MAX_EVENTS];
	int epoll_fd, nfds, n;

	/* set up the epolling structure */
	epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
	if (epoll_fd == -1) {
		TRACE_LOG("Engine %s failed to create an epoll fd!\n", 
			  eng->name);
		TRACE_BACKEND_FUNC_END();
		return;
	}

	/* create listening socket */
	create_listening_socket(eng);

	/* register listening socket */
	ev.events = EPOLLIN;
	ev.data.fd = eng->listen_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->listen_fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
	}
	
	TRACE_LOG("Engine %s is listening on port %d\n", 
		  eng->name, eng->listen_fd);

	/* register iom socket */
	ev.events = EPOLLIN;
	ev.data.fd = eng->local_fd;
	
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, eng->local_fd, &ev) == -1) {
		TRACE_LOG("Engine %s failed to exe epoll_ctl for fd: %d\n",
			  eng->name, epoll_fd);
		TRACE_BACKEND_FUNC_END();
		return;
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
			if (events[n].data.fd == eng->local_fd)
				eng->iom.callback(eng);
			if (events[n].data.fd == eng->listen_fd)
				process_request_backend(eng, epoll_fd);
		}
		
	}

	TRACE_BACKEND_FUNC_END();
}
/*---------------------------------------------------------------------*/
