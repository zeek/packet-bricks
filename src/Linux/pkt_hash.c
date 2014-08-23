/* for func prototypes */
#include "pkt_hash.h"
/* for pacf logging */
#include "pacf_log.h"
/* iphdr */
#include <linux/ip.h>
/* ipv6hdr */
#include <linux/ipv6.h>
/* tcphdr */
#include <linux/tcp.h>
/* udphdr */
#include <linux/udp.h>
/* eth hdr */
#include <linux/if_ether.h>
/* for [n/h]to[h/n][ls] */
#include <linux/in.h>
/*---------------------------------------------------------------------*/
/**
 * The cache table is used to pick a nice seed for the hash value. It is
 * built only once when sym_hash_fn is called for the very first time
 */
static void
build_sym_key_cache(uint32_t *cache, int cache_len)
{
	TRACE_PKTHASH_FUNC_START();
	static const uint8_t key[] = {
		0x50, 0x6d, 0x50, 0x6d,
                0x50, 0x6d, 0x50, 0x6d,
                0x50, 0x6d, 0x50, 0x6d,
                0x50, 0x6d, 0x50, 0x6d,
                0xcb, 0x2b, 0x5a, 0x5a,
		0xb4, 0x30, 0x7b, 0xae,
                0xa3, 0x2d, 0xcb, 0x77,
                0x0c, 0xf2, 0x30, 0x80,
                0x3b, 0xb7, 0x42, 0x6a,
                0xfa, 0x01, 0xac, 0xbe};
	
        uint32_t result = (((uint32_t)key[0]) << 24) |
                (((uint32_t)key[1]) << 16) |
                (((uint32_t)key[2]) << 8)  |
                ((uint32_t)key[3]);
	
        uint32_t idx = 32;
        int i;
	
        for (i = 0; i < cache_len; i++, idx++) {
                uint8_t shift = (idx % (sizeof(uint8_t) * 8));
                uint32_t bit;
		
                cache[i] = result;
                bit = ((key[idx/(sizeof(uint8_t) * 8)] << shift) 
		       & 0x80) ? 1 : 0;
                result = ((result << 1) | bit);
        }
	TRACE_PKTHASH_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Computes symmetric hash based on the 4-tuple header data
 */
static uint32_t
sym_hash_fn(uint32_t sip, uint32_t dip, uint16_t sp, uint32_t dp)
{
#define MSB32				0x80000000
#define MSB16				0x8000
#define KEY_CACHE_LEN			96

	TRACE_PKTHASH_FUNC_START();
	uint32_t rc = 0;
	int i;
	static int first_time = 1;
	static uint32_t key_cache[KEY_CACHE_LEN] = {0};
	
	if (first_time) {
		build_sym_key_cache(key_cache, KEY_CACHE_LEN);
		first_time = 0;
	}
	
	for (i = 0; i < 32; i++) {
                if (sip & MSB32)
                        rc ^= key_cache[i];
                sip <<= 1;
        }
        for (i = 0; i < 32; i++) {
                if (dip & MSB32)
			rc ^= key_cache[32+i];
                dip <<= 1;
        }
        for (i = 0; i < 16; i++) {
		if (sp & MSB16)
                        rc ^= key_cache[64+i];
                sp <<= 1;
        }
        for (i = 0; i < 16; i++) {
                if (dp & MSB16)
                        rc ^= key_cache[80+i];
                dp <<= 1;
        }

	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 * Parser + hash function for the IPv4 packet
 */
static uint32_t
decode_ip_n_hash(struct iphdr *iph, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t rc = 0;
	
	if (hash_split == 2) {
		rc = sym_hash_fn(NTOHL(iph->saddr),
			NTOHL(iph->daddr),
			NTOHS(0xFFFD) + seed,
			NTOHS(0xFFFE) + seed);
	} else {
		struct tcphdr *tcph = NULL;
		struct udphdr *udph = NULL;
		
		switch (iph->protocol) {
		case IPPROTO_TCP:
			tcph = (struct tcphdr *)((uint8_t *)iph + (iph->ihl<<2));
			rc = sym_hash_fn(NTOHL(iph->saddr), 
					 NTOHL(iph->daddr), 
					 NTOHS(tcph->source) + seed, 
					 NTOHS(tcph->dest) + seed);
			break;
		case IPPROTO_UDP:
			udph = (struct udphdr *)((uint8_t *)iph + (iph->ihl<<2));
			rc = sym_hash_fn(NTOHL(iph->saddr),
					 NTOHL(iph->daddr),
					 NTOHS(udph->source) + seed,
					 NTOHS(udph->dest) + seed);
			break;
		case IPPROTO_IPIP:
			/* tunneling */
			rc = decode_ip_n_hash((struct iphdr *)((uint8_t *)iph + (iph->ihl<<2)),
					      hash_split, seed);
			break;
		default:
			/* 
			 * the hash strength (although weaker but) should still hold 
			 * even with 2 fields 
			 */
			rc = sym_hash_fn(NTOHL(iph->saddr),
					 NTOHL(iph->daddr),
					 NTOHS(0xFFFD) + seed,
					 NTOHS(0xFFFE) + seed);
			break;
		}
	}
	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 * Parser + hash function for the IPv6 packet
 */
static uint32_t
decode_ipv6_n_hash(struct ipv6hdr *ipv6h, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t saddr, daddr;
	uint32_t rc = 0;

	/* Get only the first 4 octets */
	saddr = ipv6h->saddr.in6_u.u6_addr8[0] |
		(ipv6h->saddr.in6_u.u6_addr8[1] << 8) |
		(ipv6h->saddr.in6_u.u6_addr8[2] << 16) |
		(ipv6h->saddr.in6_u.u6_addr8[3] << 24);
	daddr = ipv6h->daddr.in6_u.u6_addr8[0] |
		(ipv6h->daddr.in6_u.u6_addr8[1] << 8) |
		(ipv6h->daddr.in6_u.u6_addr8[2] << 16) |
		(ipv6h->daddr.in6_u.u6_addr8[3] << 24);

	if (hash_split == 2) {
		rc = sym_hash_fn(NTOHL(saddr),
				 NTOHL(daddr),
				 NTOHS(0xFFFD) + seed,
				 NTOHS(0xFFFE) + seed);
	} else {
		struct tcphdr *tcph = NULL;
		struct udphdr *udph = NULL;
		
		switch(NTOHS(ipv6h->nexthdr)) {
		case IPPROTO_TCP:
			tcph = (struct tcphdr *)(ipv6h + 1);
			rc = sym_hash_fn(NTOHL(saddr), 
					 NTOHL(daddr), 
					 NTOHS(tcph->source) + seed, 
					 NTOHS(tcph->dest) + seed);	       
			break;
		case IPPROTO_UDP:
			udph = (struct udphdr *)(ipv6h + 1);
			rc = sym_hash_fn(NTOHL(saddr),
					 NTOHL(daddr),
					 NTOHS(udph->source) + seed,
					 NTOHS(udph->dest) + seed);		
			break;
		case IPPROTO_IPIP:
			/* tunneling */
			rc = decode_ip_n_hash((struct iphdr *)(ipv6h + 1),
					      hash_split, seed);
			break;
		case IPPROTO_IPV6:
			/* tunneling */
			rc = decode_ipv6_n_hash((struct ipv6hdr *)(ipv6h + 1),
						hash_split, seed);
			break;
		case IPPROTO_ICMP:
		case IPPROTO_GRE:
		case IPPROTO_ESP:
		case IPPROTO_PIM:
		case IPPROTO_IGMP:
		default:
			/* 
			 * the hash strength (although weaker but)
			 * should still hold  even with 2 fields 
			 */
			rc = sym_hash_fn(NTOHL(saddr),
					 NTOHL(daddr),
					 NTOHS(0xFFFD) + seed,
					 NTOHS(0xFFFE) + seed);
		}
	}

	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 *  A temp solution while hash for other protocols are filled...
 * (See decode_vlan_n_hash & pkt_hdr_hash functions).
 */
static uint32_t
decode_others_n_hash(struct ethhdr *ethh, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t saddr, daddr, rc;
	
	saddr = ethh->h_source[5] |
		(ethh->h_source[4] << 8) |
		(ethh->h_source[3] << 16) |
		(ethh->h_source[2] << 24);
	daddr = ethh->h_dest[5] |
		(ethh->h_dest[4] << 8) |
		(ethh->h_dest[3] << 16) |
		(ethh->h_dest[2] << 24);
	
	rc = sym_hash_fn(NTOHL(saddr),
			 NTOHL(daddr),
			 NTOHS(0xFFFD) + seed,
			 NTOHS(0xFFFE) + seed);

	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 * Parser + hash function for VLAN packet
 */
static inline uint32_t
decode_vlan_n_hash(struct ethhdr *ethh, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t rc = 0;
	struct vlanhdr *vhdr = (struct vlanhdr *)(ethh + 1);
	
	switch (NTOHS(vhdr->proto)) {
	case ETH_P_IP:
		rc = decode_ip_n_hash((struct iphdr *)(vhdr + 1),
				      hash_split, seed);
		break;
	case ETH_P_IPV6:
		rc = decode_ipv6_n_hash((struct ipv6hdr *)(vhdr + 1),
					hash_split, seed);
		break;
	case ETH_P_PPP_DISC:
	case ETH_P_PPP_SES:
	case ETH_P_IPX:
	case ETH_P_LOOP:
	case ETH_P_MPLS_MC:
	case ETH_P_MPLS_UC:
	case ETH_P_ARP:
	case ETH_P_RARP:
	default:
		/* others */
		rc = decode_others_n_hash(ethh, seed);
		break;
	}
	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 * General parser + hash function...
 */
uint32_t
pkt_hdr_hash(const unsigned char *buffer, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	int rc = 0;
	struct ethhdr *ethh = (struct ethhdr *)buffer;
	
	switch (NTOHS(ethh->h_proto)) {
	case ETH_P_IP:
		rc = decode_ip_n_hash((struct iphdr *)(ethh + 1),
				      hash_split, seed);
		break;
	case ETH_P_IPV6:
		rc = decode_ipv6_n_hash((struct ipv6hdr *)(ethh + 1), 
					hash_split, seed);
		break;
	case ETH_P_8021Q:
		rc = decode_vlan_n_hash(ethh, hash_split, seed);
		break;
	case ETH_P_PPP_DISC:
	case ETH_P_PPP_SES:
	case ETH_P_IPX:
	case ETH_P_LOOP:
	case ETH_P_MPLS_MC:
	case ETH_P_MPLS_UC:
	case ETH_P_ARP:
	case ETH_P_RARP:
	default:
		/* others */
		rc = decode_others_n_hash(ethh, seed);
		break;
	}
	TRACE_PKTHASH_FUNC_END();

	return rc;
}
/*---------------------------------------------------------------------*/
#if 0
uint8_t isTCP(unsigned char *buf)
{
	TRACE_PKTHASH_FUNC_START();
	uint8_t rc = 0;
	struct ethhdr *ethh = (struct ethhdr *)buf;

	switch (NTOHS(ethh->h_proto)) {
	case ETH_P_IP:
		rc = decode_ip_n_hash((struct iphdr *)(ethh + 1));
		break;
	case ETH_P_IPV6:
		rc = decode_ipv6_n_hash((struct ipv6hdr *)(ethh + 1));
		break;
	case ETH_P_8021Q:
		rc = decode_vlan_n_hash(ethh);
		break;
	default:
		break;
	}	
	TRACE_PKTHASH_FUNC_END();
	return rc;
}
#endif
/*---------------------------------------------------------------------*/
