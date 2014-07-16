#ifndef __UTIL_H__
#define __UTIL_H__
/*---------------------------------------------------------------------*/
/* for gettid/set_affinity */
#include <sched.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <unistd.h>
/*---------------------------------------------------------------------*/
/**
 * Affinitizes running thread to the given CPU id.
 */
int
set_affinity(int cpu);
/*---------------------------------------------------------------------*/
#endif /* !__UTIL_H__ */
