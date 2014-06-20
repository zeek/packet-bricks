#ifndef __PACF_INTERFACE_H__
#define __PACF_INTERFACE_H__
/*---------------------------------------------------------------------*/
/* for uint?_* types */
#include <sys/types.h>
#include <stdint.h>
/*---------------------------------------------------------------------*/
/**
 * Decision & Action targets:
 * 	- FORWARD: 	  for policying bridging/routing
 * 	- UPDATE_FORWARD: for updating a packet field and 
 * 			  then forwarding
 *	- DROP: 	  for dropping packet
 *	- INTERCEPT: 	  for stealing the packet from the 
 * 		     	  interface to a monitoring daemon
 *	- COPY: 	  for passing copy of the packet to the 
 * 			  monitoring daemon
 * 	- PKT_THRESHOLD:  hook-up function for a monitor
 * 			  once a certain packet count threshold
 * 			  is reached
 *	- BYTE_THRESHOLD: hook-up function for a monitor
 * 			  once a certain byte count threshold
 * 			  is reached
 */
typedef enum {
	FORWARD = 1,
	UPDATE_FORWARD,
	DROP,
	INTERCEPT,
	COPY,
	PKT_THRESHOLD,
	BYTE_THRESHOLD
} Target;
/*---------------------------------------------------------------------*/
/**
 * Rule-related data structures start here...
 * TODO: The following structs are still being revised...
 */

/*
 * Arguments passed to the rule
 */
typedef struct {
	void *args;
} RuleArg;
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
 *		XXX: More comments later...
 *
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
#define PACF_CONNECTION_FILTER		 1
#define PACF_TCPSPORT_FILTER		 2
#define PACF_TCPDPORT_FILTER		 3
#define PACF_UDPSPORT_FILTER		 4
#define PACF_UDPDPORT_FILTER		 5
#define PACF_TCPPROT_FILTER		 6
#define PACF_UDPPROT_FILTER		 7
#define PACF_SRCIP4ADDR_FILTER		 8
#define PACF_DSTIP4ADDR_FILTER		 9
#define PACF_SRCIP6ADDR_FILTER		10
#define PACF_DSTIP6ADDR_FILTER		11
#define PACF_IPPROT_FILTER		12
#define PACF_ARPPROT_FILTER		13
#define PACF_ICMPPROT_FILTER		14
#define PACF_SRCETH_FILTER		15
#define PACF_DSTETH_FILTER		16
#define PACF_TOTAL_FILTERS		PACF_DSTETH_FILTER
/*...*/
#define PACF_UNUSED1_FILTER		17
#define PACF_UNUSED2_FILTER		18
#define PACF_UNUSED3_FILTER		19
#define PACF_UNUSED4_FILTER		20
#define PACF_UNUSED5_FILTER	        21
#define PACF_UNUSED6_FILTER		22
#define PACF_UNUSED7_FILTER		23
#define PACF_UNUSED8_FILTER		24
#define PACF_UNUSED9_FILTER		25
#define PACF_UNUSED10_FILTER		26
#define PACF_UNUSED11_FILTER		27
#define PACF_UNUSED12_FILTER		28
#define PACF_UNUSED13_FILTER		29
#define PACF_UNUSED14_FILTER		30
#define PACF_UNUSED15_FILTER		31
#define PACF_UNUSED16_FILTER		32
/*---------------------------------------------------------------------*/
/**
 * Filter: This will become part of the rule 
 * 	   insertion/modification/deletion framework.
 * 	   XXX: More comments later...
 */
typedef struct {
	uint32_t filter_type_flag: PACF_TOTAL_FILTERS;
	union {
		EthAddress *ethaddr;
		IP4Address *ip4addr;
		IP6Address *ip6addr;
		Port *p;
		Protocol *prot;
		Connection *conn;
	};
} Filter __attribute__((aligned(__WORDSIZE)));;
/*---------------------------------------------------------------------*/
#endif /* !__PACF_INTERFACE_H__ */
