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
#include "util.h"
/* for bricks logging */
#include "bricks_log.h"
/* pthread w/ affinity */
#include <pthread_np.h>
/* cpu_set */
#include <sys/cpuset.h>
/*---------------------------------------------------------------------*/
/**
 * Affinitizes current thread to the specified cpu
 */
int
set_affinity(int cpu, pthread_t *t)
{
	TRACE_UTIL_FUNC_START();
	cpuset_t cpus;

	
	/* if cpu affinity was not specified */
	if (cpu < 0) {
		TRACE_UTIL_FUNC_END();
		return 0;
	}
	CPU_ZERO(&cpus);
	CPU_SET((unsigned)cpu, &cpus);
		
	if (pthread_setaffinity_np(*t, sizeof(cpus), &cpus)) {
		TRACE_LOG("Unable to affinitize thread %u to core %d\n",
			  *((unsigned int *)t), cpu);
		TRACE_UTIL_FUNC_END();
		return -1;
	}

	TRACE_UTIL_FUNC_END();	
	return 0;
}
/*---------------------------------------------------------------------*/
