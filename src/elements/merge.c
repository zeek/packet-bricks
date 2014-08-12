/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/*---------------------------------------------------------------------*/
int32_t
merge_init(Element *elem, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(li);
	return 1;
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
void
merge_link(struct Element *from, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(from);
	UNUSED(li);
}
/*---------------------------------------------------------------------*/
void
merge_link_eng(struct Element *elem, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(li);
}
/*---------------------------------------------------------------------*/
element_funcs mergefuncs = {
	.init			= 	merge_init,
	.link			=	merge_link,
	.process		= 	merge_process,
	.deinit			= 	merge_deinit
};
/*---------------------------------------------------------------------*/
