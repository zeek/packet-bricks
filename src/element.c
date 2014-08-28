/* for Element def'n */
#include "element.h"
/* for logging */
#include "bricks_log.h"
/*---------------------------------------------------------------------*/
element_funcs elibs[MAX_ELEMENTS];
/*---------------------------------------------------------------------*/
/**
 * Initialize all element function libraries
 */
inline void
initElements()
{
	TRACE_ELEMENT_FUNC_START();
	/* register your elements here */
	elibs[3] = lbfuncs;
	elibs[4] = dupfuncs;
	elibs[5] = mergefuncs;
	/* delimiter */
	elibs[6] = (element_funcs){NULL, NULL, NULL, NULL, NULL};
	TRACE_ELEMENT_FUNC_END();
}
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
			  (t == (Target)LINKER_LB) ? 
			  "LoadBalancer" : "Duplicator");
		TRACE_ELEMENT_FUNC_END();
		return NULL;
	}
	elem->elib = &elibs[t];

	TRACE_ELEMENT_FUNC_END();
	return elem;
}
/*---------------------------------------------------------------------*/

