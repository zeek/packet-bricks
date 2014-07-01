#ifndef __PACF_LOG_H__
#define __PACF_LOG_H__
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

#define TRACE_LUA_FUNC_START()		TRACE_FUNC_START()
#define TRACE_LUA_FUNC_END()		TRACE_FUNC_END()

#define TRACE_PKTENGINE_FUNC_START()	TRACE_FUNC_START()
#define TRACE_PKTENGINE_FUNC_END()	TRACE_FUNC_END()

#define TRACE_NETMAP_FUNC_START()	TRACE_FUNC_START()
#define TRACE_NETMAP_FUNC_END()		TRACE_FUNC_END()

#define TRACE_THREAD_FUNC_START()	TRACE_FUNC_START()
#define TRACE_THREAD_FUNC_END()		TRACE_FUNC_END()

#define TRACE_IFACE_FUNC_START()	TRACE_FUNC_START()
#define TRACE_IFACE_FUNC_END()		TRACE_FUNC_END()

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
#endif /* !__PACF_LOG_H__ */
