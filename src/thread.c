/* for func prototypes */
#include "thread.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
int
set_affinity(int cpu)
{
	TRACE_THREAD_FUNC_START();
	pid_t tid;
	cpu_set_t cpus;

	tid = syscall(__NR_gettid);
	/* if cpu affinity was not specified */
	if (cpu < 0) {
		TRACE_THREAD_FUNC_END();
		return 0;
	}
	CPU_ZERO(&cpus);
	CPU_SET((unsigned)cpu, &cpus);
		
	if (sched_setaffinity(tid, sizeof(cpus), &cpus)) {
		TRACE_THREAD_FUNC_END();
		return -1;
	}

	TRACE_FUNC_END();	
	return 0;
}
/*---------------------------------------------------------------------*/
