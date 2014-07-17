#ifndef __UTIL_H__
#define __UTIL_H__
/*---------------------------------------------------------------------*/
/* for pthreads */
#include <pthread.h>
/* for gettid/set_affinity */
#include <sched.h>
#ifdef linux
#include <linux/sched.h>
#include <linux/unistd.h>
#endif /* !linux */
#include <unistd.h>
/*---------------------------------------------------------------------*/
/**
 * Affinitizes running thread to the given CPU id.
 */
int
set_affinity(int cpu, pthread_t *t);
/*---------------------------------------------------------------------*/
#endif /* !__UTIL_H__ */
