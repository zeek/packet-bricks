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
#ifndef __BRICKS_INTERFACE_H__
#define __BRICKS_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* for uint?_* types */
#include <sys/types.h>
#include <stdint.h>
/* for IFNAMSIZ */
#include <net/if.h>
/* for queue management */
#include "mbuf.h"
/*---------------------------------------------------------------------*/
#define __FAVOR_BSD		1
/*---------------------------------------------------------------------*/
/**
 * Decision & Action targets:
 * 	- MODIFY:	  for updating a packet field and 
 * 			  then forwarding
 *	- DROP: 	  for dropping packet
 *	- SHARE: 	  for stealing packet from the 
 * 		     	  interface to a monitoring daemon
 *	- COPY: 	  for passing copy of the packet to the 
 * 			  monitoring daemon
 *	- WRITE:       	  for writing network traffic in pcap
 *			  format
 *	- LIMIT: 	  for rate-limiting packets... 
 * 	- PKT_NOTFIY:	  hook-up function for a monitor
 * 			  once a certain packet count threshold
 * 			  is reached
 *	- BYTE_NOTIFY:	  hook-up function for a monitor
 * 			  once a certain byte count threshold
 * 			  is reached
 */
typedef enum {
	MODIFY=1,
	DROP,
	SHARE,
	COPY,
	WRITE,
	LIMIT,
	PKT_NOTIFY,
	BYTE_NOTIFY,
	WHITELIST,
} Target;
/*---------------------------------------------------------------------*/
typedef struct {
	uint8_t addr8[6];
} EthAddress;

typedef struct {
	union {
		uint8_t addr8[4];
		uint16_t addr16[2];
		uint32_t addr32;
	};
	uint32_t mask;
} IP4Address __attribute__((aligned(__WORDSIZE)));

typedef struct {
	union {
		uint8_t addr8[16];
		uint16_t addr16[8];
		uint32_t addr32[4];
	};
	/* XXX This will be revised */
	/** uint32_t mask; **/
} IP6Address __attribute__((aligned(__WORDSIZE)));;
/*---------------------------------------------------------------------*/
/* XXX: These may be converted into individual structs */
typedef uint16_t Port;
typedef Port TCPPort;
typedef Port UDPPort;
typedef uint16_t Protocol;
typedef Protocol Ethtype;
typedef Protocol IPProtocol;
/*---------------------------------------------------------------------*/
/**
 *  Connection: This will become a part of the filter.
 * 		This contains all 5-tuple fields of the connection.
 *		The Connection filter includes source and destination
 *		IP addresses, source and destination ports and the
 *		protocol number.
 */
typedef struct {
	union {
		IP4Address sip4addr;
		IP6Address sip6addr;
	};
	union {
		IP4Address dip4addr;
		IP6Address dip6addr;
	};
	Port sport;
	Port dport;
	IPProtocol ip_prot;
} Connection __attribute__((aligned(__WORDSIZE)));;
/*---------------------------------------------------------------------*/
/**
 * MACROS for filter_type_flag
 */
#define BRICKS_NO_FILTER		 0
#define BRICKS_CONNECTION_FILTER	 1
#define BRICKS_TCPSPORT_FILTER		 2
#define BRICKS_TCPPORT_FILTER		 3
#define BRICKS_TCPDPORT_FILTER		 4
#define BRICKS_UDPSPORT_FILTER		 5
#define BRICKS_UDPPORT_FILTER		 6
#define BRICKS_UDPDPORT_FILTER		 7
#define BRICKS_TCPPROT_FILTER		 8
#define BRICKS_UDPPROT_FILTER		 9
#define BRICKS_SRCIP4ADDR_FILTER	10
#define BRICKS_IP4ADDR_FILTER	 	11
#define BRICKS_DSTIP4ADDR_FILTER	12
#define BRICKS_SRCIP6ADDR_FILTER	13
#define BRICKS_DSTIP6ADDR_FILTER	14
#define BRICKS_IPPROT_FILTER		15
#define BRICKS_ARPPROT_FILTER		16
#define BRICKS_ICMPPROT_FILTER		17
#define BRICKS_SRCETH_FILTER		18
#define BRICKS_ETH_FILTER		19
#define BRICKS_DSTETH_FILTER		20
#define BRICKS_TOTAL_FILTERS		BRICKS_DSTETH_FILTER
/*...*/
#define BRICKS_UNUSED1_FILTER		21
#define BRICKS_UNUSED2_FILTER		22
#define BRICKS_UNUSED3_FILTER		23
#define BRICKS_UNUSED4_FILTER		24
#define BRICKS_UNUSED5_FILTER	        25
#define BRICKS_UNUSED6_FILTER		26
#define BRICKS_UNUSED7_FILTER		27
#define BRICKS_UNUSED8_FILTER		28
#define BRICKS_UNUSED9_FILTER		29
#define BRICKS_UNUSED10_FILTER		30
#define BRICKS_UNUSED11_FILTER		31
#define BRICKS_UNUSED12_FILTER		32

/*---------------------------------------------------------------------*/
/**
 * Filter: This will become part of the rule 
 * 	   insertion/modification/deletion framework.
 * 	   XXX: More comments later...
 */
typedef struct Filter {
	uint32_t filter_type_flag: BRICKS_TOTAL_FILTERS;
	union {
		EthAddress ethaddr;
		IP4Address ip4addr;
		IP6Address ip6addr;
		Port p;
		Protocol prot;
		Connection conn;
	};
	/* duration of the entire applied filter */
	time_t filt_time_period;
	/* when do you want the filter application to start */
	time_t filt_start_time;
	/* target */
	Target tgt;

	TAILQ_ENTRY(Filter) entry;
} Filter __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * Request block: this struct is passed to the system by the userland
 * application when it makes a certain request.
 */
typedef struct req_block {
	/* length of the request */
	uint32_t len;
	unsigned char ifname[IFNAMSIZ];
	Filter f;
	unsigned char req_payload[0];
} req_block __attribute__((aligned(__WORDSIZE)));

/**
 * Response block: this struct is passed to the userland
 * application when it makes a response back.
 */
typedef struct resp_block {
	/* length of the response */
	uint32_t len;
	/*
	 * flags to indicate whether 
	 * the request was successful 
	 */
	uint8_t flag;

	unsigned char resp_payload[0];
} resp_block __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
#define BRICKS_LISTEN_PORT		1111
/**
 * BRICKS interface that accepts incoming requests from bricks shell and
 * userland apps...
 */
void
start_listening_reqs();
/*---------------------------------------------------------------------*/
#endif /* !__BRICKS_INTERFACE_H__ */
