/* for Element struct */
#include "element.h"
/* for pacf logging */
#include "pacf_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for strcmp */
#include <string.h>
/* for hash function */
#include "pkt_hash.h"
/*---------------------------------------------------------------------*/
int32_t
lb_init(Element *elem, Linker_Intf *li)
{
	TRACE_ELEMENT_FUNC_START();
	elem->private_data = calloc(1, sizeof(linkdata));
	if (elem->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for load balancer\n");
		TRACE_ELEMENT_FUNC_END();
		return -1;
	}
	TRACE_ELEMENT_FUNC_END();
	UNUSED(li);

	return 1;
}
/*---------------------------------------------------------------------*/
static inline uint32_t
myrand(uint64_t *seed)
{
	TRACE_NETMAP_FUNC_START();
	*seed = *seed * 1103515245 + 12345;
	return (uint32_t)(*seed >> 32);
	TRACE_NETMAP_FUNC_END();
}
/*---------------------------------------------------------------------*/
int32_t
lb_process(Element *elem, unsigned char *buf)
{
	TRACE_ELEMENT_FUNC_START();
	linkdata *lnd = elem->private_data;

	return (pkt_hdr_hash(buf) + myrand(&elem->eng->seed)) % 
		lnd->count;

	TRACE_ELEMENT_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
lb_deinit(Element *elem)
{
	TRACE_ELEMENT_FUNC_START();
	free(elem->private_data);
	free(elem);
	TRACE_ELEMENT_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
lb_link(struct Element *elem, Linker_Intf *linker)
{
	TRACE_ELEMENT_FUNC_START();
	int i, j, rc;
	engine *eng;
	linkdata *lbd;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	lbd = (linkdata *)elem->private_data;
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

	if (eng->elem == NULL) {
		strcpy(lbd->ifname, (char *)linker->input_link[0]);
		lbd->count = linker->output_count;
		eng->elem = elem;
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
			rc = eng->iom.create_external_link(elem,
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
element_funcs lbfuncs = {
	.init			= 	lb_init,
	.link			=	lb_link,
	.process		= 	lb_process,
	.deinit			= 	lb_deinit
};
/*---------------------------------------------------------------------*/
