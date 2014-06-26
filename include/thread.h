#ifndef __THREAD_H__
#define __THREAD_H__
/*---------------------------------------------------------------------*/
/* for gettid/set_affinity */
#include <sched.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <unistd.h>
/*---------------------------------------------------------------------*/
int
set_affinity(int cpu);
/*---------------------------------------------------------------------*/
#endif /* !__THREAD_H__ */
