/* for Element def'n */
#include "element.h"
/* for logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
/**
 * See element.h for comments. Initializes the element given
 * the target.
 */
inline Element *
createElement(Target t)
{
	TRACE_ELEMENT_FUNC_START();
	Element *elem = calloc(1, sizeof(Element));
	if (elem == NULL) {
		TRACE_ERR("Can't create element: %s\n",
			  (t == SHARE) ? 
			  "LoadBalancer" : "Duplicator");
		TRACE_ELEMENT_FUNC_END();
		return NULL;
	}
	switch (t) {
	case SHARE:
		elem->elib = &lbfuncs;
		break;
	case COPY:
		elem->elib = &dupfuncs;
		break;
	case LINKER_MERGE:
		elem->elib = &mergefuncs;
		break;
	default:
		TRACE_ERR("Invalid linker type requested!\n");
		TRACE_ELEMENT_FUNC_END();
	}
	TRACE_ELEMENT_FUNC_END();
	return elem;
}
/*---------------------------------------------------------------------*/
