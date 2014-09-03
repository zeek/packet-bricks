/* See LICENSE in the main directory for copyright. */
#ifndef __BRICKS_INTERFACE_H__
#define __BRICKS_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* for uint?_* types */
#include <sys/types.h>
#include <stdint.h>
/* for IFNAMSIZ */
#include <net/if.h>
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
	LIMIT,
	PKT_NOTIFY,
	BYTE_NOTIFY
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
#define BRICKS_CONNECTION_FILTER		 1
#define BRICKS_TCPSPORT_FILTER		 2
#define BRICKS_TCPDPORT_FILTER		 3
#define BRICKS_UDPSPORT_FILTER		 4
#define BRICKS_UDPDPORT_FILTER		 5
#define BRICKS_TCPPROT_FILTER		 6
#define BRICKS_UDPPROT_FILTER		 7
#define BRICKS_SRCIP4ADDR_FILTER		 8
#define BRICKS_DSTIP4ADDR_FILTER		 9
#define BRICKS_SRCIP6ADDR_FILTER		10
#define BRICKS_DSTIP6ADDR_FILTER		11
#define BRICKS_IPPROT_FILTER		12
#define BRICKS_ARPPROT_FILTER		13
#define BRICKS_ICMPPROT_FILTER		14
#define BRICKS_SRCETH_FILTER		15
#define BRICKS_DSTETH_FILTER		16
#define BRICKS_TOTAL_FILTERS		BRICKS_DSTETH_FILTER
/*...*/
#define BRICKS_UNUSED1_FILTER		17
#define BRICKS_UNUSED2_FILTER		18
#define BRICKS_UNUSED3_FILTER		19
#define BRICKS_UNUSED4_FILTER		20
#define BRICKS_UNUSED5_FILTER	        21
#define BRICKS_UNUSED6_FILTER		22
#define BRICKS_UNUSED7_FILTER		23
#define BRICKS_UNUSED8_FILTER		24
#define BRICKS_UNUSED9_FILTER		25
#define BRICKS_UNUSED10_FILTER		26
#define BRICKS_UNUSED11_FILTER		27
#define BRICKS_UNUSED12_FILTER		28
#define BRICKS_UNUSED13_FILTER		29
#define BRICKS_UNUSED14_FILTER		30
#define BRICKS_UNUSED15_FILTER		31
#define BRICKS_UNUSED16_FILTER		32
/*---------------------------------------------------------------------*/
/**
 * Filter: This will become part of the rule 
 * 	   insertion/modification/deletion framework.
 * 	   XXX: More comments later...
 */
typedef struct Filter {
	uint32_t filter_type_flag: BRICKS_TOTAL_FILTERS;
	union {
		EthAddress *ethaddr;
		IP4Address *ip4addr;
		IP6Address *ip6addr;
		Port *p;
		Protocol *prot;
		Connection *conn;
	};
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
