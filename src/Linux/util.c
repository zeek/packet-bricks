/* for func prototypes */
#include "util.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
/**
 * Affinitizes current thread to the specified cpu
 */
int
set_affinity(int cpu, pthread_t *t)
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
		TRACE_LOG("Unable to affinitize thread %u to core %d\n",
			  tid, cpu);
		TRACE_UTIL_FUNC_END();
		return -1;
	}

	TRACE_UTIL_FUNC_END();
	UNUSED(t);
	return 0;
}
/*---------------------------------------------------------------------*/
