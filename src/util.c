/* for type def'n */
#include <stdint.h>
/* for func prototypes */
#include "util.h"
/* for pacf logging */
#include "pacf_log.h"
/* iphdr */
#include <linux/ip.h>
/* ipv6hdr */
#include <linux/ipv6.h>
/* tcphdr */
#include <linux/tcp.h>
/* eth hdr */
#include <linux/if_ether.h>
/* for [n/h]to[h/n][ls] */
//#include <netinet/in.h>
/*---------------------------------------------------------------------*/
/**
 * Affinitizes current thread to the specified cpu
 */
int
set_affinity(int cpu)
{
	TRACE_UTIL_FUNC_START();
	pid_t tid;
	cpu_set_t cpus;

	tid = syscall(__NR_gettid);
	/* if cpu affinity was not specified */
	if (cpu < 0) {
		TRACE_UTIL_FUNC_END();
		return 0;
	}
	CPU_ZERO(&cpus);
	CPU_SET((unsigned)cpu, &cpus);
		
	if (sched_setaffinity(tid, sizeof(cpus), &cpus)) {
		TRACE_UTIL_FUNC_END();
		return -1;
	}

	TRACE_UTIL_FUNC_END();	
	return 0;
}
/*---------------------------------------------------------------------*/
/*
 * Computes hash function from the packet header
 */
uint32_t 
master_custom_hash_function(const unsigned char *buffer, 
			    const uint16_t buffer_len)
{
	TRACE_UTIL_FUNC_START();
        uint32_t l3_offset = sizeof(struct ethhdr);
        uint16_t eth_type;
	
        eth_type = (buffer[12] << 8) + buffer[13];
	
        while (eth_type == 0x8100 /* VLAN */) {
		l3_offset += 4;
		eth_type = (buffer[l3_offset - 2] << 8) + buffer[l3_offset - 1];
	}
	
        switch (eth_type) {
	case 0x0800:
		{
			/* IPv4 */
			struct iphdr *iph;
			if (unlikely(buffer_len < l3_offset + sizeof(struct iphdr))) {
				TRACE_UTIL_FUNC_END();
				return 0;
			}
			iph = (struct iphdr *) &buffer[l3_offset];
			TRACE_UTIL_FUNC_END();
			return NTOHL(iph->saddr) + NTOHL(iph->daddr); /* this can be optimized by avoiding calls
								       * to ntohl(), but it can lead to balancing 
								       * issues 
								       */
		}
		break;
	case 0x86DD:
		{
			/* IPv6 */
			struct ipv6hdr *ipv6h;
			u_int32_t *s, *d;
			
			if (unlikely(buffer_len < l3_offset + sizeof(struct ipv6hdr))) {
				TRACE_UTIL_FUNC_END();
				return 0;
			}
			
			ipv6h = (struct ipv6hdr *) &buffer[l3_offset];
			s = (u_int32_t *) &ipv6h->saddr, d = (u_int32_t *) &ipv6h->daddr;
			TRACE_UTIL_FUNC_END();
			return(s[0] + s[1] + s[2] + s[3] + d[0] + d[1] + d[2] + d[3]);
		}
		break;
	default:
		break; /* Unknown protocol */
	}
	TRACE_UTIL_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
