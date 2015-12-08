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
/* for prints etc */
#include "bricks_log.h"
/* for string functions */
#include <string.h>
/* for io modules */
#include "io_module.h"
/* for function prototypes */
#include "backend.h"
/* for requests/responses */
#include "bricks_interface.h"
/* for install_filter */
#include "netmap_module.h"
/* for filter functions */
#include "bricks_filter.h"
/* for types */
#include <sys/types.h>
/* for [n/h]to[h/n][ls] */
#include <netinet/in.h>
/* iphdr */
#include <netinet/ip.h>
/* ipv6hdr */
#include <netinet/ip6.h>
/* tcphdr */
#include <netinet/tcp.h>
/* udphdr */
#include <netinet/udp.h>
/* eth hdr */
#include <net/ethernet.h>
/* for inet_ntoa() */
#include <sys/socket.h>
#include <arpa/inet.h>
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv4Tcp(Filter *f, struct ip *iph, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return ((f->conn.sip4addr.addr32 == iph->ip_src.s_addr &&
		 f->conn.dip4addr.addr32 == iph->ip_dst.s_addr &&
		 f->conn.sport == tcph->th_sport &&
		 f->conn.dport == tcph->th_dport) ||
		(f->conn.sip4addr.addr32 == iph->ip_dst.s_addr &&
		 f->conn.dip4addr.addr32 == iph->ip_src.s_addr &&
		 f->conn.sport == tcph->th_dport &&
		 f->conn.dport == tcph->th_sport)
		) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv6Tcp(Filter *f, struct ip6_hdr *iph, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	/* to be filled */
	return 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(f);
	UNUSED(iph);
	UNUSED(tcph);
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv4Udp(Filter *f, struct ip *iph, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return ((f->conn.sip4addr.addr32 == iph->ip_src.s_addr &&
		 f->conn.dip4addr.addr32 == iph->ip_dst.s_addr /*&&
		 f->conn.sport == udph->uh_sport &&
		 f->conn.dport == udph->uh_dport*/) ||
		(f->conn.sip4addr.addr32 == iph->ip_dst.s_addr &&
		 f->conn.dip4addr.addr32 == iph->ip_src.s_addr /*&&
		 f->conn.sport == udph->uh_dport &&
		 f->conn.dport == udph->uh_sport*/)		
		) ? 0 : 1;
	return 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv6Udp(Filter *f, struct ip6_hdr *iph, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	/* to be filled */
	return 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(f);
	UNUSED(iph);
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterSport(Filter *f, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == tcph->th_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterDport(Filter *f, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == tcph->th_dport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterPort(Filter *f, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == tcph->th_dport ||
		f->p == tcph->th_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterSport(Filter *f, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == udph->uh_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterDport(Filter *f, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == udph->uh_dport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterPort(Filter *f, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (f->p == udph->uh_dport ||
		f->p == udph->uh_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddressSrc(Filter *f, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (f->ip4addr.addr32 == iph->ip_src.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddress(Filter *f, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (f->ip4addr.addr32 == iph->ip_src.s_addr ||
		f->ip4addr.addr32 == iph->ip_dst.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddressDst(Filter *f, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (f->ip4addr.addr32 == iph->ip_dst.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddressSrc(Filter *f, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(f->ethaddr.addr8, ethh->ether_shost,
			sizeof(ethh->ether_shost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddress(Filter *f, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(f->ethaddr.addr8, ethh->ether_shost,
			sizeof(ethh->ether_shost)) ||
		!memcmp(f->ethaddr.addr8, ethh->ether_dhost,
			sizeof(ethh->ether_dhost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddressDst(Filter *f, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(f->ethaddr.addr8, ethh->ether_dhost,
			sizeof(ethh->ether_dhost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleARPFilter(Filter *f, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (ethh->ether_type == ETHERTYPE_ARP) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(f);
}
/*---------------------------------------------------------------------*/
/* Under construction.. */
int
analyze_packet(unsigned char *buf, CommNode *cn, time_t current_time)
{	
	TRACE_FILTER_FUNC_START();
	struct ether_header *ethh = NULL;
	struct ip *iph = NULL;
	struct ip6_hdr *ip6h = NULL;
	struct tcphdr *tcph = NULL;
	struct udphdr *udph = NULL;
	int rc = 1;
	Filter *f = NULL;
	Filter *f_prev = NULL;

	ethh = (struct ether_header *)buf;
	switch (ntohs(ethh->ether_type)) {
	case ETHERTYPE_IP:
		/* IPv4 case */
		iph = ((struct ip *)(ethh + 1));
		break;
	case ETHERTYPE_IPV6:
		/* IPv6 case */
		ip6h = ((struct ip6_hdr *)(ethh + 1));
		break;
	default:
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Failed to recognize L3 protocol\n");
		return 1;
	}
	
	if (iph != NULL) {
		switch (iph->ip_p) {
		case IPPROTO_TCP:
			tcph = (struct tcphdr *)((uint8_t *)iph + (iph->ip_hl<<2));
			break;
		case IPPROTO_UDP:
			udph = (struct udphdr *)((uint8_t *)iph + (iph->ip_hl<<2));
			break;
		case IPPROTO_IPIP:
			/* tunneling case to be filled */
		default:
			TRACE_FILTER_FUNC_END();
			TRACE_DEBUG_LOG("Failed to recognize L4 protocol\n");
			return 1;
		}
	}
	
	TAILQ_FOREACH_SAFE(f, &cn->filter_list, entry, f_prev) {
		if (unlikely((f->filt_time_period >= 0) &&
			     (current_time - f->filt_start_time >
			      f->filt_time_period))) {
			/* filter has expired, delete entry */
			if (f->filter_type_flag != BRICKS_NO_FILTER) {
				f->filter_type_flag = BRICKS_NO_FILTER;
				TRACE_LOG("Disabling filter: current_time: %d, filt_start_time: %d, filt_time_period: %d\n",
					  (int)current_time, (int)f->filt_start_time, (int)f->filt_time_period);
			}
			TAILQ_REMOVE(&cn->filter_list, f, entry);
			free(f);
			continue;
		}

		switch (f->filter_type_flag) {
		case BRICKS_CONNECTION_FILTER:
			if (iph != NULL) {
				if (tcph != NULL)
					rc = HandleConnectionFilterIPv4Tcp(f, iph, tcph);
				else if (udph != NULL)
					rc = HandleConnectionFilterIPv4Udp(f, iph, udph);
			} else if (ip6h != NULL) {
				if (tcph != NULL)
					rc = HandleConnectionFilterIPv6Tcp(f, ip6h, tcph);
				else if (udph != NULL)
					rc = HandleConnectionFilterIPv6Udp(f, ip6h, udph);
			}
			TRACE_DEBUG_LOG("Connection filter\n");
			break;
		case BRICKS_TCPSPORT_FILTER:
			if (tcph != NULL)
				rc = HandleTcpFilterSport(f, tcph);
			TRACE_DEBUG_LOG("Tcp src port filter\n");
			break;
		case BRICKS_TCPPORT_FILTER:
			if (tcph != NULL)
				rc = HandleTcpFilterPort(f, tcph);
			TRACE_DEBUG_LOG("Tcp port filter\n");
			break;		
		case BRICKS_TCPDPORT_FILTER:
			if (tcph != NULL)
				rc = HandleTcpFilterDport(f, tcph);
			TRACE_DEBUG_LOG("Tcp dst port filter\n");
			break;
		case BRICKS_UDPSPORT_FILTER:
			if (udph != NULL)
				rc = HandleUdpFilterSport(f, udph);
			TRACE_DEBUG_LOG("Udp src port filter\n");
			break;
		case BRICKS_UDPPORT_FILTER:
			if (udph != NULL)
				rc = HandleUdpFilterPort(f, udph);
			TRACE_DEBUG_LOG("Udp port filter\n");
			break;		
		case BRICKS_UDPDPORT_FILTER:
			if (udph != NULL)
				rc = HandleUdpFilterDport(f, udph);
			TRACE_DEBUG_LOG("Udp dst port filter\n");
			break;
		case BRICKS_TCPPROT_FILTER:
			if (tcph != NULL)
				rc = 0;
			TRACE_DEBUG_LOG("Tcp protocol filter\n");
			break;
		case BRICKS_UDPPROT_FILTER:
			if (udph != NULL)
				rc = 0;
			TRACE_DEBUG_LOG("Udp protocol filter\n");
			break;
		case BRICKS_SRCIP4ADDR_FILTER:
			if (iph != NULL)
				rc = HandleIPFilterAddressSrc(f, iph);
			TRACE_DEBUG_LOG("IP src address filter\n");
			break;
		case BRICKS_IP4ADDR_FILTER:
			if (iph != NULL)
				rc = HandleIPFilterAddress(f, iph);
			TRACE_DEBUG_LOG("IP address filter\n");
			break;		
		case BRICKS_DSTIP4ADDR_FILTER:
			if (iph != NULL)
				rc = HandleIPFilterAddressDst(f, iph);
			TRACE_DEBUG_LOG("IP dst address filter\n");
			break;
		case BRICKS_SRCIP6ADDR_FILTER:
			/* to be filled later */
			TRACE_DEBUG_LOG("IP6 src address filter\n");
			break;
		case BRICKS_DSTIP6ADDR_FILTER:
			/* to be filled later */
			TRACE_DEBUG_LOG("IP6 dst address filter\n");
			break;
		case BRICKS_IPPROT_FILTER:
			if (iph != NULL || ip6h != NULL)
				rc = 0;
			TRACE_DEBUG_LOG("IP proto filter\n");
			break;
		case BRICKS_ARPPROT_FILTER:
			rc = HandleARPFilter(f, ethh);
			TRACE_DEBUG_LOG("ARP proto filter\n");
			break;
		case BRICKS_ICMPPROT_FILTER:
			TRACE_DEBUG_LOG("ICMP filter\n");
			break;
		case BRICKS_SRCETH_FILTER:
			rc = HandleEthFilterAddressSrc(f, ethh);
			TRACE_DEBUG_LOG("Mac src address filter\n");
			break;
		case BRICKS_ETH_FILTER:
			rc = HandleEthFilterAddress(f, ethh);
			TRACE_DEBUG_LOG("Mac address filter\n");
			break;
		case BRICKS_DSTETH_FILTER:
			rc = HandleEthFilterAddressDst(f, ethh);
			TRACE_DEBUG_LOG("Mac dest address filter\n");
			break;
		default:
			break;
		}

		if (rc == 0 && f->tgt == WHITELIST)
			return 1;
	}

	TRACE_FILTER_FUNC_END();
	return rc;

	UNUSED(tcph);
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
int
apply_filter(CommNode *cn, Filter *fin)
{
	TRACE_FILTER_FUNC_START();
	Filter *f = (Filter *)calloc(1, sizeof(Filter));
	if (f == NULL) {
		TRACE_LOG("Could not allocate memory for a new filter!\n");
		return 0;
	}
	memcpy(f, fin, sizeof(Filter));
	f->filt_start_time = time(NULL) + fin->filt_start_time;

	TRACE_LOG("Applying filter with time period: %d, and start_time: %d\n",
		  (int)f->filt_time_period, (int)f->filt_start_time);
	
	TAILQ_INSERT_TAIL(&cn->filter_list, f, entry);
	return 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
printFilter(Filter *f)
{
	TRACE_FILTER_FUNC_START();
	struct in_addr addr;
	
	TRACE_LOG("Printing current filter...\n");
	
	switch (f->filter_type_flag) {
	case BRICKS_NO_FILTER:
		TRACE_LOG("No filter!\n");
		break;
	case BRICKS_CONNECTION_FILTER:
		TRACE_LOG("It's a connection filter\n");
		addr.s_addr = f->conn.sip4addr.addr32;
		TRACE_LOG("Sip: %s\n", inet_ntoa(addr));
		addr.s_addr = f->conn.dip4addr.addr32;
		TRACE_LOG("Dip: %s\n", inet_ntoa(addr));
		break;
	default:
		TRACE_LOG("Filter type is unrecognized!\n");
		break;
	}
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
#ifdef ENABLE_BROKER
/* for broker comm. */
#include <broker/broker.h>
/*---------------------------------------------------------------------*/
#define parse_request(x, f)		parse_record(x, f)
void
parse_record(broker_data *v, Filter *f)
{
	TRACE_FILTER_FUNC_START();

	char *str = NULL;
	float duration = 0;
	
	broker_record *inner_r = broker_data_as_record(v);
	broker_record_iterator *inner_it = broker_record_iterator_create(inner_r);
	broker_subnet *bsub = NULL;
	char temp[20];
	struct in_addr temp_addr;
	char *needle;
	
	TRACE_DEBUG_LOG( "Got a record\n");
	while (!broker_record_iterator_at_last(inner_r, inner_it)) {
		broker_data *inner_d = broker_record_iterator_value(inner_it);
		if (inner_d != NULL) {
			broker_data_type bdt = broker_data_which(inner_d);
			switch (bdt) {
			case broker_data_type_bool:
				TRACE_DEBUG_LOG( "Got a bool: %d\n",
						 broker_bool_true(broker_data_as_bool(inner_d)));
				break;
			case broker_data_type_count:
				TRACE_DEBUG_LOG( "Got a count: %lu\n",
						 *broker_data_as_count(inner_d));
				break;
			case broker_data_type_integer:
				TRACE_DEBUG_LOG( "Got an integer: %ld\n",
						 *broker_data_as_integer(inner_d));
				break;
			case broker_data_type_real:
				TRACE_DEBUG_LOG( "Got a real: %f\n",
						 *broker_data_as_real(inner_d));
				break;
			case broker_data_type_string:
				TRACE_DEBUG_LOG( "Got a string: %s\n",
						 broker_string_data(broker_data_as_string(inner_d)));
				break;
			case broker_data_type_address:
				TRACE_DEBUG_LOG( "Got an address: %s\n",
						 broker_string_data(broker_address_to_string(broker_data_as_address(inner_d))));
				break;
			case broker_data_type_subnet:
				bsub = broker_data_as_subnet(inner_d);
				str = (char *)broker_string_data(broker_subnet_to_string(bsub));
				TRACE_DEBUG_LOG("Got a subnet: %s\n",
						str);
				needle = strstr(str, "/");
				if (needle == NULL) {
					strcpy(temp, str);
				} else {
					memcpy(temp, str, needle-str);
					temp[needle-str] = '\0';
				}
				
				inet_aton(temp, &temp_addr);
				if (f->conn.sip4addr.addr32 != 0) {
					memcpy(&f->conn.dip4addr.addr32,
					       &temp_addr,
					       sizeof(uint32_t));
					TRACE_DEBUG_LOG("Setting dest ip address: %u\n",
							f->conn.dip4addr.addr32);
				} else {
					memcpy(&f->conn.sip4addr.addr32,
					       &temp_addr,
					       sizeof(uint32_t));
					TRACE_DEBUG_LOG("Setting src ip address: %u\n",
							f->conn.sip4addr.addr32);
				}
				break;
			case broker_data_type_port:
				TRACE_DEBUG_LOG( "Got a port: %u\n",
						 broker_port_number(broker_data_as_port(inner_d)));
				break;
			case broker_data_type_time:
				TRACE_DEBUG_LOG( "Got time: %f\n",
						 broker_time_point_value(broker_data_as_time(inner_d)));
				break;
			case broker_data_type_duration:
				duration = broker_time_duration_value(broker_data_as_duration(inner_d));
				TRACE_DEBUG_LOG( "Got duration: %f\n",
						 duration);
				f->filt_time_period = (time_t)duration;
				break;
			case broker_data_type_enum_value:
				str = (char *)broker_enum_value_name(broker_data_as_enum(inner_d));
				TRACE_DEBUG_LOG( "Got enum: %s\n",
						 str);
				if (!strcmp(str, "NetControl::DROP"))
					f->tgt = DROP;
				else if (!strcmp(str, "NetControl::WHITELIST"))
					f->tgt = WHITELIST;				
				else if (!strcmp(str, "NetControl::FLOW")) {
					/* process flow record */
					f->filter_type_flag = BRICKS_CONNECTION_FILTER;
				}
				break;
			case broker_data_type_set:
				TRACE_DEBUG_LOG( "Got a set\n");
				break;
			case broker_data_type_table:
				TRACE_DEBUG_LOG( "Got a table\n");
				break;
			case broker_data_type_vector:
				TRACE_DEBUG_LOG( "Got a vector\n");
				break;
			case broker_data_type_record:
				parse_record(inner_d, f);
				break;
			default:
				break;
			}
		}
		broker_data_delete(inner_d);
		broker_record_iterator_next(inner_r, inner_it);
	}
	broker_record_iterator_delete(inner_it);
	broker_record_delete(inner_r);
	TRACE_DEBUG_LOG( "End of record\n");
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
brokerize_request(engine *eng, broker_message_queue *q)
{
	TRACE_FILTER_FUNC_START();
	broker_deque_of_message *msgs = broker_message_queue_need_pop(q);
	int n = broker_deque_of_message_size(msgs);
	int i;
	Filter f;

	memset(&f, 0, sizeof(f));
	
	/* check vector contents */
	for (i = 0; i < n; ++i) {
		broker_vector *m = broker_deque_of_message_at(msgs, i);
		broker_vector_iterator *it = broker_vector_iterator_create(m);
		int count = RULE_ACC;
		while (!broker_vector_iterator_at_last(m, it)) {
			broker_data *v = broker_vector_iterator_value(it);
			switch (count) {
			case RULE_REQUEST:
				parse_request(v, &f);
				break;
			case RULE_ACC:
				/* currently only supports rule addition */
			default:
				if (v != NULL) {
					if (broker_data_which(v) == broker_data_type_record)
						parse_record(v, &f);
					else
						TRACE_DEBUG_LOG("Got a message: %s!\n",
								broker_string_data(broker_data_to_string(v)));
				}
				broker_data_delete(v);
				broker_vector_iterator_next(m, it);
				break;
			}
			count++;
		}
		broker_vector_iterator_delete(it);
		broker_vector_delete(m);
	}
	
	broker_deque_of_message_delete(msgs);	

	printFilter(&f);
	//f.filt_time_period = (time_t)-1;

	/* TODO: XXX - This needs to be set with respect to interface name */
	apply_filter(TAILQ_FIRST(&eng->commnode_list), &f);
	
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
/*
 * Code self-explanatory...
 */
static void __unused
create_listening_socket_for_eng(engine *eng)
{
	TRACE_FILTER_FUNC_START();

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
		TRACE_FILTER_FUNC_END();
	}
	
	/* bind serv information to mysocket */
	if (bind(eng->listen_fd, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1) {
	  TRACE_ERR("Failed to bind listening socket to port %d of engine %s\n",
		    PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd, eng->name);
	  TRACE_FILTER_FUNC_END();
	}
	
	/* start listening, allowing a queue of up to 1 pending connection */
	if (listen(eng->listen_fd, LISTEN_BACKLOG) == -1) {
		TRACE_ERR("Failed to start listen on port %d (engine %s)\n",
			  PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd, eng->name);
		TRACE_FILTER_FUNC_END();
	}
	eng->listen_port = PKTENGINE_LISTEN_PORT + eng->esrc[0]->dev_fd;
	TRACE_LOG("Engine listening on port: %u\n", eng->listen_port);
	
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Accepts connection for connecting to the backend...
 */
static int __unused
accept_request_backend(engine *eng, int sock)
{
	TRACE_FILTER_FUNC_START();
	int clientsock;
	struct sockaddr_in client;
	socklen_t c;

	if (sock == eng->listen_fd) {
		/* accept connection from an incoming client */
		clientsock = accept(eng->listen_fd,
				    (struct sockaddr *)&client, &c);
		if (clientsock < 0) {
			TRACE_LOG("accept failed: %s\n", strerror(errno));
			TRACE_FILTER_FUNC_END();
			return -1;
		}
		TRACE_FILTER_FUNC_END();
		return clientsock;
	}
	
	TRACE_FILTER_FUNC_END();
	return -1;
}
/*---------------------------------------------------------------------*/
/**
 * Services incoming request from userland applications and takes
 * necessary actions. The actions can be: (i) passing packets to 
 * userland apps etc.
 */
static int __unused
process_request_backend(int sock, engine *eng)
{
	TRACE_FILTER_FUNC_START();
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
	TRACE_FILTER_FUNC_END();
	return -1;
}
/*---------------------------------------------------------------------*/
void
initialize_filt_comm(engine *eng)
{
	TRACE_FILTER_FUNC_START();
	broker_init(0);
	broker_endpoint *node0 = broker_endpoint_create("node0");
	broker_string *topic = broker_string_create("bro/event");
	broker_message_queue *pq = broker_message_queue_create(topic, node0);

	/* only works for a single-threaded process */
	if (!broker_endpoint_listen(node0, 9999, "127.0.0.1", 1)) {
		TRACE_ERR("%s\n", broker_endpoint_last_error(node0));
		TRACE_FILTER_FUNC_END();
		return;
	}

	/* finally assigned the pq message ptr */
	eng->pq_ptr = (void *)pq;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
int
fetch_filter_comm_fd(engine *eng)
{
	TRACE_FILTER_FUNC_START();
	return broker_message_queue_fd((broker_message_queue *)eng->pq_ptr);
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
process_broker_request(engine *eng, struct pollfd *pfd, int i)
{
	TRACE_FILTER_FUNC_START();
	broker_message_queue *pq = (broker_message_queue *)eng->pq_ptr;
	if (pfd[i].fd == broker_message_queue_fd(pq) &&
	    (!(pfd[i].revents & (POLLERR |
				 POLLHUP |
				 POLLNVAL)) &&
	     (pfd[i].revents & POLLIN))) {
		brokerize_request(eng, pq);
	}	
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
#endif
