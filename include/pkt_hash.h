#ifndef __PKT_HASH__
#define __PKT_HASH__
/*---------------------------------------------------------------------*/
/**
 * Packet header hashing function utility - This file contains functions
 * that parse the packet headers and computes hash functions based on
 * the header fields. Please see pkt_hash.c for more details...
 */
/*---------------------------------------------------------------------*/
/* for type def'n */
#include <stdint.h>
/*---------------------------------------------------------------------*/
/* Enable this macro if you want a trivial version of the hash */
/* It only parses till the IP header */
//define TRIVIAL_HASH_FUNCTION
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#define HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | \
		  (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | \
		  (((unsigned short)(n) & 0xFF00) >> 8))

#define HTONL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
        ((((unsigned long)(n) & 0xFF00)) << 8) | \
        ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
		  ((((unsigned long)(n) & 0xFF000000)) >> 24))

#define NTOHL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
        ((((unsigned long)(n) & 0xFF00)) << 8) | \
        ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
		  ((((unsigned long)(n) & 0xFF000000)) >> 24))
/*---------------------------------------------------------------------*/
typedef struct vlanhdr {
	uint16_t pri_cfi_vlan;
	uint16_t proto;
} vlanhdr;
/*---------------------------------------------------------------------*/
/**
 * Analyzes the packet header of computes a corresponding 
 * hash function.
 */
uint32_t
pkt_hdr_hash(const unsigned char *buffer,
	     uint8_t hash_split,
	     uint8_t seed);
/*---------------------------------------------------------------------*/
#endif /* __PKT_HASH__ */
