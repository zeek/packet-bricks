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
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv4Tcp(CommNode *cn, struct ip *iph, struct tcphdr *tcph)
{
	/* FIXME!!!! BIDIRECTIONAL!!! */
	TRACE_FILTER_FUNC_START();
	return ((cn->filt.conn.sip4addr.addr32 == iph->ip_src.s_addr &&
		 cn->filt.conn.dip4addr.addr32 == iph->ip_dst.s_addr &&
		 cn->filt.conn.sport == tcph->th_sport &&
		 cn->filt.conn.dport == tcph->th_dport) ||
		(cn->filt.conn.sip4addr.addr32 == iph->ip_dst.s_addr &&
		 cn->filt.conn.dip4addr.addr32 == iph->ip_src.s_addr &&
		 cn->filt.conn.sport == tcph->th_dport &&
		 cn->filt.conn.dport == tcph->th_sport)
		) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv6Tcp(CommNode *cn, struct ip6_hdr *iph, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	/* to be filled */
	return 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(cn);
	UNUSED(iph);
	UNUSED(tcph);
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv4Udp(CommNode *cn, struct ip *iph, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return ((cn->filt.conn.sip4addr.addr32 == iph->ip_src.s_addr &&
		cn->filt.conn.dip4addr.addr32 == iph->ip_dst.s_addr &&
		cn->filt.conn.sport == udph->uh_sport &&
		 cn->filt.conn.dport == udph->uh_dport) ||
		(cn->filt.conn.sip4addr.addr32 == iph->ip_dst.s_addr &&
		 cn->filt.conn.dip4addr.addr32 == iph->ip_src.s_addr &&
		 cn->filt.conn.sport == udph->uh_dport &&
		 cn->filt.conn.dport == udph->uh_sport)		
		) ? 0 : 1;
	return 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleConnectionFilterIPv6Udp(CommNode *cn, struct ip6_hdr *iph, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	/* to be filled */
	return 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(cn);
	UNUSED(iph);
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterSport(CommNode *cn, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == tcph->th_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterDport(CommNode *cn, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == tcph->th_dport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleTcpFilterPort(CommNode *cn, struct tcphdr *tcph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == tcph->th_dport ||
		cn->filt.p == tcph->th_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterSport(CommNode *cn, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == udph->uh_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterDport(CommNode *cn, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == udph->uh_dport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleUdpFilterPort(CommNode *cn, struct udphdr *udph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.p == udph->uh_dport ||
		cn->filt.p == udph->uh_sport) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddressSrc(CommNode *cn, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.ip4addr.addr32 == iph->ip_src.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddress(CommNode *cn, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.ip4addr.addr32 == iph->ip_src.s_addr ||
		cn->filt.ip4addr.addr32 == iph->ip_dst.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleIPFilterAddressDst(CommNode *cn, struct ip *iph)
{
	TRACE_FILTER_FUNC_START();
	return (cn->filt.ip4addr.addr32 == iph->ip_dst.s_addr) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddressSrc(CommNode *cn, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(cn->filt.ethaddr.addr8, ethh->ether_shost,
			sizeof(ethh->ether_shost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddress(CommNode *cn, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(cn->filt.ethaddr.addr8, ethh->ether_shost,
			sizeof(ethh->ether_shost)) ||
		!memcmp(cn->filt.ethaddr.addr8, ethh->ether_dhost,
			sizeof(ethh->ether_dhost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleEthFilterAddressDst(CommNode *cn, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (!memcmp(cn->filt.ethaddr.addr8, ethh->ether_dhost,
			sizeof(ethh->ether_dhost))) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
}
/*---------------------------------------------------------------------*/
static inline int32_t
HandleARPFilter(CommNode *cn, struct ether_header *ethh)
{
	TRACE_FILTER_FUNC_START();
	return (ethh->ether_type == ETHERTYPE_ARP) ? 0 : 1;
	TRACE_FILTER_FUNC_END();
	UNUSED(cn);
}
/*---------------------------------------------------------------------*/
/* Under construction.. */
int
analyze_packet(unsigned char *buf, CommNode *cn)
{	
	TRACE_FILTER_FUNC_START();
	struct ether_header *ethh = NULL;
	struct ip *iph = NULL;
	struct ip6_hdr *ip6h = NULL;
	struct tcphdr *tcph = NULL;
	struct udphdr *udph = NULL;
	int rc = 1;

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

	switch (cn->filt.filter_type_flag) {
	case BRICKS_CONNECTION_FILTER:
		if (iph != NULL) {
			if (tcph != NULL)
				rc = HandleConnectionFilterIPv4Tcp(cn, iph, tcph);
			else if (udph != NULL)
				rc = HandleConnectionFilterIPv4Udp(cn, iph, udph);
		} else if (ip6h != NULL) {
			if (tcph != NULL)
				rc = HandleConnectionFilterIPv6Tcp(cn, ip6h, tcph);
			else if (udph != NULL)
				rc = HandleConnectionFilterIPv6Udp(cn, ip6h, udph);
		}
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Connection filter\n");
		return rc;
		break;
	case BRICKS_TCPSPORT_FILTER:
		if (tcph != NULL)
			rc = HandleTcpFilterSport(cn, tcph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Tcp src port filter\n");
		return rc;
		break;
	case BRICKS_TCPPORT_FILTER:
		if (tcph != NULL)
			rc = HandleTcpFilterPort(cn, tcph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Tcp port filter\n");
		return rc;
		break;		
	case BRICKS_TCPDPORT_FILTER:
		if (tcph != NULL)
			rc = HandleTcpFilterDport(cn, tcph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Tcp dst port filter\n");
		return rc;
		break;
	case BRICKS_UDPSPORT_FILTER:
		if (udph != NULL)
			rc = HandleUdpFilterSport(cn, udph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Udp src port filter\n");
		return rc;
		break;
	case BRICKS_UDPPORT_FILTER:
		if (udph != NULL)
			rc = HandleUdpFilterPort(cn, udph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Udp port filter\n");
		return rc;
		break;		
	case BRICKS_UDPDPORT_FILTER:
		if (udph != NULL)
			rc = HandleUdpFilterDport(cn, udph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Udp dst port filter\n");
		return rc;
		break;
	case BRICKS_TCPPROT_FILTER:
		if (tcph != NULL)
			rc = 0;
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Tcp protocol filter\n");
		return rc;
		break;
	case BRICKS_UDPPROT_FILTER:
		if (udph != NULL)
			rc = 0;
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Udp protocol filter\n");
		return rc;
		break;
	case BRICKS_SRCIP4ADDR_FILTER:
		if (iph != NULL)
			rc = HandleIPFilterAddressSrc(cn, iph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("IP src address filter\n");
		return rc;
		break;
	case BRICKS_IP4ADDR_FILTER:
		if (iph != NULL)
			rc = HandleIPFilterAddress(cn, iph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("IP address filter\n");
		return rc;
		break;		
	case BRICKS_DSTIP4ADDR_FILTER:
		if (iph != NULL)
			rc = HandleIPFilterAddressDst(cn, iph);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("IP dst address filter\n");
		return rc;		
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
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("IP proto filter\n");
		return rc;
		break;
	case BRICKS_ARPPROT_FILTER:
		rc = HandleARPFilter(cn, ethh);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("ARP proto filter\n");
		return rc;
		break;
	case BRICKS_ICMPPROT_FILTER:
		TRACE_DEBUG_LOG("ICMP filter\n");
		break;
	case BRICKS_SRCETH_FILTER:
		rc = HandleEthFilterAddressSrc(cn, ethh);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Mac src address filter\n");
		return rc;
		break;
	case BRICKS_ETH_FILTER:
		rc = HandleEthFilterAddress(cn, ethh);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Mac address filter\n");
		return rc;
		break;
	case BRICKS_DSTETH_FILTER:
		rc = HandleEthFilterAddressDst(cn, ethh);
		TRACE_FILTER_FUNC_END();
		TRACE_DEBUG_LOG("Mac dest address filter\n");
		return rc;		
		break;
	default:
		break;
	}
	TRACE_FILTER_FUNC_END();
	return rc;

	UNUSED(tcph);
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
