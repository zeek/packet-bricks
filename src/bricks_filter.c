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
/* Under construction.. */
int
analyze_packet(unsigned char *buf, CommNode *cn)
{	
	TRACE_FILTER_FUNC_START();
	struct ether_header *ethh = NULL;
	struct ip *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct udphdr *udph = NULL;

	ethh = (struct ether_header *)buf;
	switch (ntohs(ethh->ether_type)) {
	case ETHERTYPE_IP:
		iph = ((struct ip *)(ethh + 1));
	default:
		TRACE_FILTER_FUNC_END();
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
			TRACE_FILTER_FUNC_END();
			return 1;
		}
	}

	switch (cn->filt.filter_type_flag) {
	case BRICKS_CONNECTION_FILTER:
		break;
	case BRICKS_TCPSPORT_FILTER:
		break;
	case BRICKS_TCPDPORT_FILTER:
		break;
	case BRICKS_UDPSPORT_FILTER:
		break;
	case BRICKS_UDPDPORT_FILTER:
		break;
	case BRICKS_TCPPROT_FILTER:
		break;
	case BRICKS_UDPPROT_FILTER:
		break;
	case BRICKS_SRCIP4ADDR_FILTER:
		break;
	case BRICKS_DSTIP4ADDR_FILTER:
		break;
	case BRICKS_SRCIP6ADDR_FILTER:
		break;
	case BRICKS_DSTIP6ADDR_FILTER:
		break;
	case BRICKS_IPPROT_FILTER:
		break;
	case BRICKS_ARPPROT_FILTER:
		break;
	case BRICKS_ICMPPROT_FILTER:
		break;
	case BRICKS_SRCETH_FILTER:
		break;
	case BRICKS_DSTETH_FILTER:
		break;
	default:
		break;
	}
	TRACE_FILTER_FUNC_END();
	return 1;

	UNUSED(tcph);
	UNUSED(udph);
}
/*---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*/
