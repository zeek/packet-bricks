#ifndef __PACF_LOG_H__
#define __PACF_LOG_H__
/*---------------------------------------------------------------------*/
/* for std I/O */
#include <stdio.h>
/* for std lib funcs */
#include <stdlib.h>
/*---------------------------------------------------------------------*/
/**
 * DEBUG-enabled loggers.
 * Use:
 * TRACE_FUNC_* : for function entry and exit points
 * TRACE_DEBUG_*: for printing log and warning statements
 */
#ifdef DEBUG
#define TRACE_FUNC_START(m...) {					\
		fprintf(stdout, ">>> [%10s():%4d] " f, __FUNCTION__,	\
			__LINE__, ##m);					\
	}
#define TRACE_FUNC_END(m...) {						\
		fprintf(stdout, "<<< [%10s():%4d] " f, __FUNCTION__,	\
			__LINE__, ##m);					\
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

#define TRACE_LUA_FUNC_START(m...)	TRACE_FUNC_START(m...)
#define TRACE_LUA_FUNC_END(m...)	TRACE_FUNC_END(m...)


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
		fprintf(stderr, "[%10s():%4d]>> ERROR!!: " f, __FUNCTION__,	\
		       __LINE__, ##m);					\
		exit(EXIT_FAILURE);					\
	}

#define TRACE_FLUSH() fflush(stdout)
/*---------------------------------------------------------------------*/
#endif /* !__PACF_LOG_H__ */
