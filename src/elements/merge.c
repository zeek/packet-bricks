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
	elem->private_data = calloc(1, sizeof(linkdata));
	if (elem->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for merge\n");
		TRACE_ELEMENT_FUNC_END();
		return -1;
	}
	TRACE_ELEMENT_FUNC_END();
	UNUSED(li);
	return 1;
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
	/* XXX - to be filled */
	TRACE_ELEMENT_FUNC_END();
	UNUSED(from);
	UNUSED(li);
}
/*---------------------------------------------------------------------*/
element_funcs mergefuncs = {
	.init			= 	merge_init,
	.link			=	merge_link,
	.process		= 	NULL,
	.deinit			= 	merge_deinit
};
/*---------------------------------------------------------------------*/
