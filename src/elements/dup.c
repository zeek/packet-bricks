/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
void
dup_init(Element *elem, Element *from)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(from);
}
/*---------------------------------------------------------------------*/
void
dup_process(Element *elem, unsigned char *pktbuf)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(pktbuf);
}
/*---------------------------------------------------------------------*/
void
dup_deinit(Element *elem)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
}
/*---------------------------------------------------------------------*/
element_funcs dupfuncs = {
	.init		= 	dup_init,
	.process	= 	dup_process,
	.deinit		= 	dup_deinit
};
/*---------------------------------------------------------------------*/
