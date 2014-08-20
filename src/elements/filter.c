/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for string functions */
#include <string.h>
/*---------------------------------------------------------------------*/
int32_t
filter_init(Element *elem, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	elem->private_data = calloc(1, sizeof(linkdata));
	if (elem->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for filter\n");
		TRACE_ELEMENT_FUNC_END();
		return -1;
	}
	TRACE_ELEMENT_FUNC_END();
	UNUSED(li);
	return 1;
}
/*---------------------------------------------------------------------*/
static BITMAP
isTCP(Element *elem, unsigned char *buf)
{
	TRACE_ELEMENT_FUNC_START();
	
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
	UNUSED(buf);
	return 1;
}
/*---------------------------------------------------------------------*/
void
filter_deinit(Element *elem)
{
	TRACE_ELEMENT_FUNC_START();
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
}
/*---------------------------------------------------------------------*/
void
filter_link(struct Element *from, Linker_Intf *linker)
{
	TRACE_ELEMENT_FUNC_START();
	int i, j, rc;
	engine *eng;
	linkdata *lbd;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	lbd = (linkdata *)from->private_data;
	eng = engine_find(from->eng->name);
	/* sanity engine check */
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  from->eng->name);
		TRACE_ELEMENT_FUNC_END();
		return;
	}
	/* if engine is already running, then don't connect elements */
	if (eng->run == 1) {
		TRACE_LOG("Can't open channel"
			  "engine %s is already running\n",
			  eng->name);
		TRACE_ELEMENT_FUNC_END();
		return;	      
	}

	if (eng->elem == NULL) {
		strcpy(lbd->ifname, (char *)linker->input_link[0]);
		lbd->count = linker->output_count;
		eng->elem = from;
		lbd->external_links = calloc(lbd->count,
						sizeof(void *));
		if (lbd->external_links == NULL) {
			TRACE_LOG("Can't allocate external link contexts "
				  "for load balancer\n");
			TRACE_ELEMENT_FUNC_END();
			return;
		}
	}

	for (j = 0; j < linker->input_count; j++) {
		for (i = 0; i < linker->output_count; i++) {
			rc = eng->iom.create_external_link(from,
							   (char *)linker->input_link[j],
							   (char *)linker->output_link[i],
							   div_type);
			if (rc == -1) {
				TRACE_LOG("Failed to open channel %s\n",
					  linker->output_link[i]);
				return;
			}
		}
	}      
	TRACE_ELEMENT_FUNC_END();
}
/*---------------------------------------------------------------------*/
element_funcs filterfuncs = {
	.init			= 	filter_init,
	.link			=	filter_link,
	.process		= 	isTCP,
	.deinit			= 	filter_deinit
};
/*---------------------------------------------------------------------*/
