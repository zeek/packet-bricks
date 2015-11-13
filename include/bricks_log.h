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
#ifndef __BRICKS_LOG_H__
#define __BRICKS_LOG_H__
/*---------------------------------------------------------------------*/
/* for std I/O */
#include <stdio.h>
/* for std lib funcs */
#include <stdlib.h>
/*---------------------------------------------------------------------*/
/**
 * Use this macro when the variable is not to be referred to.
 */
#define UNUSED(x)			(void)x
/*---------------------------------------------------------------------*/
/**
 * DEBUG-enabled loggers.
 * Use:
 * TRACE_FUNC_* : for function entry and exit points
 * TRACE_DEBUG_*: for printing log and warning statements
 */
#ifdef DEBUG
#define TRACE_FUNC_START() {					\
		fprintf(stdout, ">>> [%10s():%4d] \n", __FUNCTION__,	\
			__LINE__);					\
	}
#define TRACE_FUNC_END() {						\
		fprintf(stdout, "<<< [%10s():%4d] \n", __FUNCTION__,	\
			__LINE__);					\
	}
#define TRACE_DEBUG_LOG(f, m...) {					\
		fprintf(stdout, "[%10s():%4d] _debuginfo_: " f, __FUNCTION__, \
			__LINE__, ##m);					\
	}
#define TRACE_DEBUG_WARNING(f, m...) {					\
		fprintf(stdout, "[%10s():%4d] _warning_: " f, __FUNCTION__, \
			__LINE__, ##m);					\
	}

#else /* DEBUG */

#define TRACE_FUNC_START(f, m...)	(void)0
#define TRACE_FUNC_END(f, m...)		(void)0
#define TRACE_DEBUG_LOG(f, n...)	(void)0
#endif /* !DEBUG  */

#ifdef DLUA
#define TRACE_LUA_FUNC_START()		TRACE_FUNC_START()
#define TRACE_LUA_FUNC_END()		TRACE_FUNC_END()
#else /* DLUA */
#define TRACE_LUA_FUNC_START()		(void)0
#define TRACE_LUA_FUNC_END()		(void)0
#endif /* !DLUA */

#ifdef DPKTENG
#define TRACE_PKTENGINE_FUNC_START()	TRACE_FUNC_START()
#define TRACE_PKTENGINE_FUNC_END()	TRACE_FUNC_END()
#else /* DPKTENG */
#define TRACE_PKTENGINE_FUNC_START()	(void)0
#define TRACE_PKTENGINE_FUNC_END()	(void)0
#endif /* !DPKTENG */

#ifdef DNMP
#define TRACE_NETMAP_FUNC_START()	TRACE_FUNC_START()
#define TRACE_NETMAP_FUNC_END()		TRACE_FUNC_END()
#else /* DNMP */
#define TRACE_NETMAP_FUNC_START()	(void)0
#define TRACE_NETMAP_FUNC_END()		(void)0
#endif /* !DNMP */

#ifdef DUTIL
#define TRACE_UTIL_FUNC_START()		TRACE_FUNC_START()
#define TRACE_UTIL_FUNC_END()		TRACE_FUNC_END()
#else /* DTHREAD */
#define TRACE_UTIL_FUNC_START()		(void)0
#define TRACE_UTIL_FUNC_END()	        (void)0
#endif /* !DTHREAD */

#ifdef DIFACE
#define TRACE_IFACE_FUNC_START()	TRACE_FUNC_START()
#define TRACE_IFACE_FUNC_END()		TRACE_FUNC_END()
#else /* DIFACE */
#define TRACE_IFACE_FUNC_START()	(void)0
#define TRACE_IFACE_FUNC_END()		(void)0
#endif /* !DIFACE */

#ifdef DBKEND
#define TRACE_BACKEND_FUNC_START()	TRACE_FUNC_START()
#define TRACE_BACKEND_FUNC_END()	TRACE_FUNC_END()
#else /* DBKEND */
#define TRACE_BACKEND_FUNC_START()	(void)0
#define TRACE_BACKEND_FUNC_END()	(void)0
#endif /* !DBKEND */

#ifdef DPKTHASH
#define TRACE_PKTHASH_FUNC_START()	TRACE_FUNC_START()
#define TRACE_PKTHASH_FUNC_END()	TRACE_FUNC_END()
#else /* DPKTHASH */
#define TRACE_PKTHASH_FUNC_START()	(void)0
#define TRACE_PKTHASH_FUNC_END()	(void)0
#endif /* !DPKTHASH */

#ifdef DBRICK
#define TRACE_BRICK_FUNC_START()	TRACE_FUNC_START()
#define TRACE_BRICK_FUNC_END()		TRACE_FUNC_END()
#else /* DBRICK */
#define TRACE_BRICK_FUNC_START()	(void)0
#define TRACE_BRICK_FUNC_END()		(void)0
#endif /* !DBRICK */

#ifdef DFILTER
#define TRACE_FILTER_FUNC_START()	TRACE_FUNC_START()
#define TRACE_FILTER_FUNC_END()		TRACE_FUNC_END()
#else /* DFILTER */
#define TRACE_FILTER_FUNC_START()	(void)0
#define TRACE_FILTER_FUNC_END()		(void)0
#endif /* !DFILTER */

/**
 * Default loggers.
 * Use:
 * TRACE_LOG : for printing log statements
 * TRACE_DEBUG_*: for printing warning statements
 */
#define TRACE_LOG(f, m...) {						\
		fprintf(stdout, "[%10s(): line %4d] " f, __FUNCTION__,	\
			__LINE__, ##m);					\
	}

#define TRACE_ERR(f, m...) {						\
		fprintf(stderr, "[%10s():%4d]>> ERROR!!: " f, __FUNCTION__, \
			__LINE__, ##m);					\
		TRACE_FUNC_END();					\
		exit(EXIT_FAILURE);					\
	}

#define TRACE_FLUSH()			fflush(stdout)
/*---------------------------------------------------------------------*/
#endif /* !__BRICKS_LOG_H__ */
