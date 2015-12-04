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
		 f->conn.dip4addr.addr32 == iph->ip_dst.s_addr &&
		 f->conn.sport == udph->uh_sport &&
		 f->conn.dport == udph->uh_dport) ||
		(f->conn.sip4addr.addr32 == iph->ip_dst.s_addr &&
		 f->conn.dip4addr.addr32 == iph->ip_src.s_addr &&
		 f->conn.sport == udph->uh_dport &&
		 f->conn.dport == udph->uh_sport)		
		) ? 0 : 1;
	return 1;
	TRACE_FILTER_FUNC_END();
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
apply_filter(CommNode *cn, req_block *rb)
{
	TRACE_FILTER_FUNC_START();
	Filter *f = (Filter *)calloc(1, sizeof(Filter));
	if (f == NULL) {
		TRACE_LOG("Could not allocate memory for a new filter!\n");
		return 0;
	}
	memcpy(f, &rb->f, sizeof(Filter));
	f->filt_start_time = time(NULL) + rb->f.filt_start_time;

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
