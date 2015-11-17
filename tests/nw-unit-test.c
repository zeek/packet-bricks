/* for I/O */
#include <stdio.h>
/* for std funcs */
#include <stdlib.h>
/* for getopt() */
#include <unistd.h>
/* for str functions */
#include <string.h>
/* basic request structs */
#include "bricks_interface.h"
/* for in_addr */
#include <netdb.h>
/* for inet_aton */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* for errno */
#include <errno.h>
/*---------------------------------------------------------------------*/
#define UNUSED(x)		(void)x
#define PORT			1239
#define ADDR			"127.0.0.1"
/*---------------------------------------------------------------------*/
/**
 * Prints the help menu
 */
static void
print_help(char *progname)
{
	fprintf(stderr,
		"Usage: %s -i $ifname [-p port]\nExample: %s -i eth2}0\n",
		progname, progname);
}
/*---------------------------------------------------------------------*/
static int
connectTcp(struct in_addr addr, uint16_t port)
{
	int sock;
	struct sockaddr_in client;
	
	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) return -1;

	client.sin_addr = addr;
	client.sin_family = AF_INET;
	client.sin_port = htons(port);

	if (connect(sock, (struct sockaddr *)&client, sizeof(client)) < 0)
		return -1;

	else
		return sock;
}
/*---------------------------------------------------------------------*/
static void
sendRequest(const char *ifname, struct in_addr addr, uint16_t port)
{
	int sock;
	req_block rb;
	
	sock = connectTcp(addr, port);
	if (sock == -1) {
		fprintf(stderr, "Can't connect! %s\n",
			strerror(errno));
		return;
	}

	strcpy((char *)rb.ifname, (char *)ifname);
	rb.f.filter_type_flag = BRICKS_UDPPROT_FILTER;
	rb.period = 20;

	fprintf(stderr, "Sending req block of size: %d\n",
		(int)sizeof(rb));
	
	if (write(sock, &rb, sizeof(rb)) < 0) {
		fprintf(stderr, "Can't send data to sock!: %s\n",
			strerror(errno));
		return;
	}

	close(sock);
}
/*---------------------------------------------------------------------*/
/**
 * Main entry point
 */
int
main(int argc, char **argv)
{
	int rc;
	char ifname[IFNAMSIZ];
	uint16_t port;
	char ipstr[20] = ADDR;
	struct in_addr ip;

	port = PORT;
	
	while ( (rc = getopt(argc, argv, "i:a:p:")) != -1) {
		switch(rc) {
		case 'i':
			strcpy(ifname, optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'a':
			strcpy(ipstr, optarg);
			break;
		default:
			print_help(*argv);
			break;
		}
	}
	
	if (!inet_aton(ipstr, &ip)) {
		fprintf(stderr,
			"Can't parse IP address: %s\n",
			ipstr);
		exit(EXIT_FAILURE);
	}

	sendRequest(ifname, ip, port);
		    
	return EXIT_SUCCESS;
}
/*---------------------------------------------------------------------*/
