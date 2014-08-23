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
BITMAP
dup_process(Element *elem, unsigned char *buf)
{
	TRACE_ELEMENT_FUNC_START();
	linkdata *lnd = elem->private_data;
	BITMAP b;
	int i;

	INIT_BITMAP(b);
	for (i = 0; i < lnd->count; i++) {
		SET_BIT(b, i);
	}
	
	TRACE_ELEMENT_FUNC_END();
	UNUSED(buf);
	return b;
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
	int i, j, rc;
	engine *eng;
	linkdata *dupdata;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
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

	if (eng->FIRST_ELEM(esrc)->elem == NULL) {
		strcpy(dupdata->ifname, (char *)linker->input_link[0]);
		dupdata->count = linker->output_count;
		eng->FIRST_ELEM(esrc)->elem = elem;
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

	for (j = 0; j < linker->input_count; j++) { 
		for (i = 0; i < linker->output_count; i++) {
			rc = eng->iom.create_external_link((char *)linker->input_link[j],
							   (char *)linker->output_link[i],
							   div_type, eng->FIRST_ELEM(esrc));
			if (rc == -1) {
				TRACE_LOG("Failed to open channel %s\n",
					  linker->output_link[i]);
				TRACE_ELEMENT_FUNC_END();
				return;
			}
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
