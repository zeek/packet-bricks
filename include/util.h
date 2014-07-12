#ifndef __UTIL_H__
#define __UTIL_H__
/*---------------------------------------------------------------------*/
/* for gettid/set_affinity */
#include <sched.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <unistd.h>
/*---------------------------------------------------------------------*/
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
int
set_affinity(int cpu);

uint32_t 
master_custom_hash_function(const unsigned char *buffer, 
			    const uint16_t buffer_len);
/*---------------------------------------------------------------------*/
#endif /* !__UTIL_H__ */
