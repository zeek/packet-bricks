/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for strcmp */
#include <string.h>
/*---------------------------------------------------------------------*/
int32_t
dup_init(Element *elem, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	elem->private_data = calloc(1, sizeof(linkdata));
	if (elem->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for duplicator\n");
		TRACE_ELEMENT_FUNC_END();
		return -1;
	}
	TRACE_ELEMENT_FUNC_END();
	UNUSED(li);
	return 1;
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
	free(elem->private_data);
	free(elem);
	TRACE_ELEMENT_FUNC_END();
	UNUSED(elem);
}
/*---------------------------------------------------------------------*/
void
dup_link(struct Element *elem, Linker_Intf *linker)
{
	TRACE_ELEMENT_FUNC_START();
	int i, rc;
	engine *eng;
	linkdata *dupdata;
	int div_type = (linker->type == LINKER_LB) ? SHARE : COPY;
	
	dupdata = (linkdata *)elem->private_data;
	eng = engine_find(elem->eng->name);
	/* sanity engine check */
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  elem->eng->name);
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

	if (!strcmp((char *)eng->link_name, (char *)linker->input_link)) {
		strcpy(dupdata->ifname, (char *)eng->link_name);
		dupdata->count = linker->output_count;
		eng->elem = elem;
		eng->mark_for_copy = 1;
		dupdata->external_links = calloc(dupdata->count,
						 sizeof(void *));
		if (dupdata->external_links == NULL) {
			TRACE_LOG("Can't allocate external link contexts "
				  "for duplicator\n");
			TRACE_ELEMENT_FUNC_END();
			return;
		}
	}

	for (i = 0; i < linker->output_count; i++) {
		rc = eng->iom.create_external_link(elem,
						   (char *)linker->input_link,
						   (char *)linker->output_link[i],
						   div_type);
		if (rc == -1) {
			TRACE_LOG("Failed to open channel %s\n",
				  linker->output_link[i]);
			return;
		}
	}

	TRACE_ELEMENT_FUNC_END();
}
/*---------------------------------------------------------------------*/
element_funcs dupfuncs = {
	.init			= 	dup_init,
	.link			=	dup_link,
	.process		= 	dup_process,
	.deinit			= 	dup_deinit
};
/*---------------------------------------------------------------------*/
