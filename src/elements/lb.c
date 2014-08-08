/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
void
lb_init(Element *elem, Element *from)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(from);
}
/*---------------------------------------------------------------------*/
void
lb_process(Element *elem, unsigned char *pktbuf)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(pktbuf);
}
/*---------------------------------------------------------------------*/
void
lb_deinit(Element *elem)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
}
/*---------------------------------------------------------------------*/
element_funcs lbfuncs = {
	.init		= 	lb_init,
	.process	= 	lb_process,
	.deinit		= 	lb_deinit
};
/*---------------------------------------------------------------------*/
