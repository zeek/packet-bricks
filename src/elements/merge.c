/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
void
merge_init(Element *elem, Element *from)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(from);
}
/*---------------------------------------------------------------------*/
void
merge_process(Element *elem, unsigned char *pktbuf)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(pktbuf);
}
/*---------------------------------------------------------------------*/
void
merge_deinit(Element *elem)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
}
/*---------------------------------------------------------------------*/
element_funcs mergefuncs = {
	.init		= 	merge_init,
	.process	= 	merge_process,
	.deinit		= 	merge_deinit
};
/*---------------------------------------------------------------------*/
