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
/* for func prototypes */
#include "pkt_hash.h"
/* for bricks logging */
#include "bricks_log.h"
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
/* for memset */
#include <string.h>
/*---------------------------------------------------------------------*/
/* no. of bits in a byte */
#define NBBY			8
#define TOEPLITZ_KEYLEN_MAX     40
#define TOEPLITZ_KEYSEED_CNT    2
#define TOEPLITZ_KEYSEED0       0x6d
#define TOEPLITZ_KEYSEED1       0x5a
#define TOEPLITZ_INIT_KEYLEN    (TOEPLITZ_KEYSEED_CNT + sizeof(uint32_t))

static uint32_t toeplitz_keyseeds[TOEPLITZ_KEYSEED_CNT] =
	{ TOEPLITZ_KEYSEED0, TOEPLITZ_KEYSEED1 };

uint32_t        toeplitz_cache[TOEPLITZ_KEYSEED_CNT][256];
/*---------------------------------------------------------------------*/
static inline uint32_t __unused
toeplitz_rawhash_addrport(in_addr_t _faddr, in_addr_t _laddr,
			  in_port_t _fport, in_port_t _lport)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t _res;
	
	_res =  toeplitz_cache[0][_faddr & 0xff];
	_res ^= toeplitz_cache[0][(_faddr >> 16) & 0xff];
	_res ^= toeplitz_cache[0][_laddr & 0xff];
	_res ^= toeplitz_cache[0][(_laddr >> 16) & 0xff];
	_res ^= toeplitz_cache[0][_fport & 0xff];
	_res ^= toeplitz_cache[0][_lport & 0xff];
	
	_res ^= toeplitz_cache[1][(_faddr >> 8) & 0xff];
	_res ^= toeplitz_cache[1][(_faddr >> 24) & 0xff];
	_res ^= toeplitz_cache[1][(_laddr >> 8) & 0xff];
	_res ^= toeplitz_cache[1][(_laddr >> 24) & 0xff];
	_res ^= toeplitz_cache[1][(_fport >> 8) & 0xff];
	_res ^= toeplitz_cache[1][(_lport >> 8) & 0xff];

	TRACE_PKTHASH_FUNC_END();
	return _res;
}
/*---------------------------------------------------------------------*/
static __inline uint32_t __unused
toeplitz_rawhash_addr(in_addr_t _faddr, in_addr_t _laddr)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t _res;
	
	_res =  toeplitz_cache[0][_faddr & 0xff];
	_res ^= toeplitz_cache[0][(_faddr >> 16) & 0xff];
	_res ^= toeplitz_cache[0][_laddr & 0xff];
	_res ^= toeplitz_cache[0][(_laddr >> 16) & 0xff];
	
	_res ^= toeplitz_cache[1][(_faddr >> 8) & 0xff];
	_res ^= toeplitz_cache[1][(_faddr >> 24) & 0xff];
	_res ^= toeplitz_cache[1][(_laddr >> 8) & 0xff];
	_res ^= toeplitz_cache[1][(_laddr >> 24) & 0xff];

	TRACE_PKTHASH_FUNC_END();
	return _res;
}
/*---------------------------------------------------------------------*/
#if 0
static __inline int
toeplitz_hash(uint32_t _rawhash)
{
	TRACE_PKTHASH_FUNC_START();
	TRACE_PKTHASH_FUNC_END();
	return (_rawhash & 0xffff);
}
#endif
/*---------------------------------------------------------------------*/
static void
toeplitz_cache_create(uint32_t cache[][256], int cache_len,
		      const uint8_t key_str[], int key_strlen __unused)
{
	TRACE_PKTHASH_FUNC_START();
	int i;
	
	for (i = 0; i < cache_len; ++i) {
		uint32_t key[NBBY];
		int j, b, shift, val;
		
		memset(key, 0, sizeof(key));
		
		/*
		 * Calculate 32bit keys for one byte; one key for each bit.
		 */
		for (b = 0; b < NBBY; ++b) {
			for (j = 0; j < 32; ++j) {
				uint8_t k;
				int bit;
				
				bit = (i * NBBY) + b + j;
				
				k = key_str[bit / NBBY];
				shift = NBBY - (bit % NBBY) - 1;
				if (k & (1 << shift))
					key[b] |= 1 << (31 - j);
			}
		}
		
		/*
		 * Cache the results of all possible bit combination of
		 * one byte.
		 */
		for (val = 0; val < 256; ++val) {
			uint32_t res = 0;
			
			for (b = 0; b < NBBY; ++b) {
				shift = NBBY - b - 1;
				if (val & (1 << shift))
					res ^= key[b];
			}
			cache[i][val] = res;
		}
	}
	TRACE_PKTHASH_FUNC_END();
}
/*---------------------------------------------------------------------*/
#ifdef RSS_DEBUG  
static void
toeplitz_verify(void)
{
	TRACE_PKTHASH_FUNC_START();
	in_addr_t faddr, laddr;
	in_port_t fport, lport;
	
	/*
	 * The first IPv4 example in the verification suite
	 */
	
	/* 66.9.149.187:2794 */
	faddr = 0xbb950942;
	fport = 0xea0a;
	
	/* 161.142.100.80:1766 */
	laddr = 0x50648ea1;
	lport = 0xe606;
	
	kprintf("toeplitz: verify addr/port 0x%08x, addr 0x%08x\n",
		toeplitz_rawhash_addrport(faddr, laddr, fport, lport),
		toeplitz_rawhash_addr(faddr, laddr));
	TRACE_PKTHASH_FUNC_END();
}
#endif  /* RSS_DEBUG */
/*---------------------------------------------------------------------*/
void
toeplitz_get_key(uint8_t *key, int keylen)
{
	TRACE_PKTHASH_FUNC_START();
	int i;
	
	if (keylen > TOEPLITZ_KEYLEN_MAX) {
		TRACE_ERR("invalid key length %d", keylen);
		TRACE_PKTHASH_FUNC_END();
		exit(EXIT_FAILURE);
	}
	
	/* Replicate key seeds to form key */
	for (i = 0; i < keylen; ++i)
		key[i] = toeplitz_keyseeds[i % TOEPLITZ_KEYSEED_CNT];
	TRACE_PKTHASH_FUNC_END();
}
/*---------------------------------------------------------------------*/
static void
toeplitz_init(void *dummy __unused)
{
	TRACE_PKTHASH_FUNC_START();
	uint8_t key[TOEPLITZ_INIT_KEYLEN];
	int i;

	for (i = 0; i < TOEPLITZ_KEYSEED_CNT; ++i)
		toeplitz_keyseeds[i] &= 0xff;
	
	toeplitz_get_key(key, TOEPLITZ_INIT_KEYLEN);
	
#ifdef RSS_DEBUG
	kprintf("toeplitz: keystr ");
	for (i = 0; i < TOEPLITZ_INIT_KEYLEN; ++i)
		kprintf("%02x ", key[i]);
	kprintf("\n");
#endif
	toeplitz_cache_create(toeplitz_cache, TOEPLITZ_KEYSEED_CNT,
			      key, TOEPLITZ_INIT_KEYLEN);
	
#ifdef RSS_DEBUG
	toeplitz_verify();
#endif
	TRACE_PKTHASH_FUNC_END();
}
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
static uint32_t __unused
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
		toeplitz_init(NULL);
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
decode_ip_n_hash(struct ip *iph, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t rc = 0;
	
	if (hash_split == 2) {
#if 1
		rc = sym_hash_fn(ntohl(iph->ip_src.s_addr),
			ntohl(iph->ip_dst.s_addr),
			ntohs(0xFFFD) + seed,
			ntohs(0xFFFE) + seed);
#else
		rc = toeplitz_rawhash_addr(ntohl(iph->ip_src.s_addr),
					   ntohl(iph->ip_dst.s_addr));
#endif
	} else {
		struct tcphdr *tcph = NULL;
		struct udphdr *udph = NULL;
		
		switch (iph->ip_p) {
		case IPPROTO_TCP:
			tcph = (struct tcphdr *)((uint8_t *)iph + (iph->ip_hl<<2));
#if 1
			rc = sym_hash_fn(ntohl(iph->ip_src.s_addr), 
					 ntohl(iph->ip_dst.s_addr), 
					 ntohs(tcph->th_sport) + seed, 
					 ntohs(tcph->th_dport) + seed);
#else
			rc = toeplitz_rawhash_addrport(ntohl(iph->ip_src.s_addr), 
						       ntohl(iph->ip_dst.s_addr), 
						       ntohs(tcph->th_sport) + seed, 
						       ntohs(tcph->th_dport) + seed);
#endif
			break;
		case IPPROTO_UDP:
			udph = (struct udphdr *)((uint8_t *)iph + (iph->ip_hl<<2));
#if 1
			rc = sym_hash_fn(ntohl(iph->ip_src.s_addr),
					 ntohl(iph->ip_dst.s_addr),
					 ntohs(udph->uh_sport) + seed,
					 ntohs(udph->uh_dport) + seed);
#else
			rc = toeplitz_rawhash_addrport(ntohl(iph->ip_src.s_addr),
						       ntohl(iph->ip_dst.s_addr),
						       ntohs(udph->uh_sport) + seed,
						       ntohs(udph->uh_dport) + seed);	
#endif
			break;
		case IPPROTO_IPIP:
			/* tunneling */
			rc = decode_ip_n_hash((struct ip *)((uint8_t *)iph + (iph->ip_hl<<2)),
					      hash_split, seed);
			break;
		default:
			/* 
			 * the hash strength (although weaker but) should still hold 
			 * even with 2 fields 
			 */
#if 1
			rc = sym_hash_fn(ntohl(iph->ip_src.s_addr),
					 ntohl(iph->ip_dst.s_addr),
					 ntohs(0xFFFD) + seed,
					 ntohs(0xFFFE) + seed);
#else
			rc = toeplitz_rawhash_addr(ntohl(iph->ip_src.s_addr),
						   ntohl(iph->ip_dst.s_addr));
#endif
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
decode_ipv6_n_hash(struct ip6_hdr *ipv6h, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t saddr, daddr;
	uint32_t rc = 0;
	
	/* Get only the first 4 octets */
	saddr = ipv6h->ip6_src.s6_addr[0] |
		(ipv6h->ip6_src.s6_addr[1] << 8) |
		(ipv6h->ip6_src.s6_addr[2] << 16) |
		(ipv6h->ip6_src.s6_addr[3] << 24);
	daddr = ipv6h->ip6_dst.s6_addr[0] |
		(ipv6h->ip6_dst.s6_addr[1] << 8) |
		(ipv6h->ip6_dst.s6_addr[2] << 16) |
		(ipv6h->ip6_dst.s6_addr[3] << 24);
	
	if (hash_split == 2) {
#if 1
		rc = sym_hash_fn(ntohl(saddr),
				 ntohl(daddr),
				 ntohs(0xFFFD) + seed,
				 ntohs(0xFFFE) + seed);
#else
		rc = toeplitz_rawhash_addr(ntohl(saddr),
					   ntohl(daddr));
#endif
	} else {
		struct tcphdr *tcph = NULL;
		struct udphdr *udph = NULL;
		
		switch(ntohs(ipv6h->ip6_ctlun.ip6_un1.ip6_un1_nxt)) {
		case IPPROTO_TCP:
			tcph = (struct tcphdr *)(ipv6h + 1);
#if 1
			rc = sym_hash_fn(ntohl(saddr), 
					 ntohl(daddr), 
					 ntohs(tcph->th_sport) + seed, 
					 ntohs(tcph->th_dport) + seed);
#else
			rc = toeplitz_rawhash_addrport(ntohl(saddr), 
						       ntohl(daddr), 
						       ntohs(tcph->th_sport) + seed, 
						       ntohs(tcph->th_dport) + seed);
#endif
			break;
		case IPPROTO_UDP:
			udph = (struct udphdr *)(ipv6h + 1);
#if 1
			rc = sym_hash_fn(ntohl(saddr),
					 ntohl(daddr),
					 ntohs(udph->uh_sport) + seed,
					 ntohs(udph->uh_dport) + seed);
#else
			rc = toeplitz_rawhash_addrport(ntohl(saddr),
						       ntohl(daddr),
						       ntohs(udph->uh_sport) + seed,
						       ntohs(udph->uh_dport) + seed);
#endif
			break;
		case IPPROTO_IPIP:
			/* tunneling */
			rc = decode_ip_n_hash((struct ip *)(ipv6h + 1),
					      hash_split, seed);
			break;
		case IPPROTO_IPV6:
			/* tunneling */
			rc = decode_ipv6_n_hash((struct ip6_hdr *)(ipv6h + 1),
						hash_split, seed);
			break;
		case IPPROTO_ICMP:
		case IPPROTO_GRE:
		case IPPROTO_ESP:
		case IPPROTO_PIM:
		case IPPROTO_IGMP:
		default:
			/* 
			 * the hash strength (although weaker but) should still hold 
			 * even with 2 fields 
			 */
#if 1
			rc = sym_hash_fn(ntohl(saddr),
					 ntohl(daddr),
					 ntohs(0xFFFD) + seed,
					 ntohs(0xFFFE) + seed);
#else
			rc = toeplitz_rawhash_addr(ntohl(saddr),
						   ntohl(daddr));			
#endif
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
decode_others_n_hash(struct ether_header *ethh, uint8_t seed __unused)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t saddr, daddr, rc;
	
	saddr = ethh->ether_shost[5] |
		(ethh->ether_shost[4] << 8) |
		(ethh->ether_shost[3] << 16) |
		(ethh->ether_shost[2] << 24);
	daddr = ethh->ether_dhost[5] |
		(ethh->ether_dhost[4] << 8) |
		(ethh->ether_dhost[3] << 16) |
		(ethh->ether_dhost[2] << 24);

#if 1
	rc = sym_hash_fn(ntohl(saddr),
			 ntohl(daddr),
			 ntohs(0xFFFD) + seed,
			 ntohs(0xFFFE) + seed);
#else
	rc = toeplitz_rawhash_addr(ntohl(saddr),
				   ntohl(daddr));
#endif

	TRACE_PKTHASH_FUNC_END();
	return rc;
}
/*---------------------------------------------------------------------*/
/**
 * Parser + hash function for VLAN packet
 */
static inline uint32_t
decode_vlan_n_hash(struct ether_header *ethh, uint8_t hash_split, uint8_t seed)
{
	TRACE_PKTHASH_FUNC_START();
	uint32_t rc = 0;
	struct vlanhdr *vhdr = (struct vlanhdr *)(ethh + 1);
	
	switch (ntohs(vhdr->proto)) {
	case ETHERTYPE_IP:
		rc = decode_ip_n_hash((struct ip *)(vhdr + 1),
				      hash_split, seed);
		break;
	case ETHERTYPE_IPV6:
		rc = decode_ipv6_n_hash((struct ip6_hdr *)(vhdr + 1),
					hash_split, seed);
		break;
	case ETHERTYPE_PPPOEDISC:
	case ETHERTYPE_PPPOE:
	case ETHERTYPE_IPX:
	case ETHERTYPE_LOOPBACK:
	case ETHERTYPE_MPLS_MCAST:
	case ETHERTYPE_MPLS:
	case ETHERTYPE_ARP:
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
	struct ether_header *ethh = (struct ether_header *)buffer;
	
	switch (ntohs(ethh->ether_type)) {
	case ETHERTYPE_IP:
		rc = decode_ip_n_hash((struct ip *)(ethh + 1),
				      hash_split, seed);
		break;
	case ETHERTYPE_IPV6:
		rc = decode_ipv6_n_hash((struct ip6_hdr *)(ethh + 1),
					hash_split, seed);
		break;
	case ETHERTYPE_VLAN:
		rc = decode_vlan_n_hash(ethh, hash_split, seed);
		break;
	case ETHERTYPE_PPPOEDISC:
	case ETHERTYPE_PPPOE:
	case ETHERTYPE_IPX:
	case ETHERTYPE_LOOPBACK:
	case ETHERTYPE_MPLS_MCAST:
	case ETHERTYPE_MPLS:
	case ETHERTYPE_ARP:
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
