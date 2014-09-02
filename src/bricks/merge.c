/* for Brick struct */
#include "brick.h"
/* for bricks logging */
#include "bricks_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for string functions */
#include <string.h>
/*---------------------------------------------------------------------*/
int32_t
merge_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	brick->private_data = calloc(1, sizeof(linkdata));
	if (brick->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for merge\n");
		TRACE_BRICK_FUNC_END();
		return -1;
	}
	TRACE_BRICK_FUNC_END();
	UNUSED(li);
	return 1;
}
/*---------------------------------------------------------------------*/
void
merge_deinit(Brick *brick)
{
	TRACE_BRICK_FUNC_START();
	if (brick->private_data != NULL) {
		free(brick->private_data);
		brick->private_data = NULL;
	}
	free(brick);
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
merge_link(struct Brick *from, Linker_Intf *linker)
{
	TRACE_BRICK_FUNC_START();
	int i, j, k, rc;
	engine *eng;
	linkdata *lbd;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	lbd = (linkdata *)from->private_data;
	eng = engine_find(from->eng->name);
	/* sanity engine check */
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  from->eng->name);
		TRACE_BRICK_FUNC_END();
		return;
	}
	/* if engine is already running, then don't connect bricks */
	if (eng->run == 1) {
		TRACE_LOG("Can't open channel"
			  "engine %s is already running\n",
			  eng->name);
		TRACE_BRICK_FUNC_END();
		return;	      
	}

	if (eng->FIRST_BRICK(esrc)->brick == NULL) {
		strcpy(lbd->ifname, (char *)linker->input_link[0]);
		lbd->count = linker->output_count;
		for (k = 0; k < (int)eng->no_of_sources; k++)
			eng->esrc[k]->brick = from;
		lbd->external_links = calloc(lbd->count,
						sizeof(void *));
		if (lbd->external_links == NULL) {
			TRACE_LOG("Can't allocate external link contexts "
				  "for load balancer\n");
			TRACE_BRICK_FUNC_END();
			return;
		}
	}

	for (j = 0; j < linker->input_count; j++) {
		for (i = 0; i < linker->output_count; i++) {
			for (k = 0; k < (int)eng->no_of_sources; k++) {
				rc = eng->iom.create_external_link((char *)linker->input_link[j],
								   (char *)linker->output_link[i],
								   div_type, eng->esrc[k]);
				if (rc == -1) {
					TRACE_LOG("Failed to open channel %s\n",
						  linker->output_link[i]);
					return;
				}
			}
		}
	}      
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
char *
merge_getid()
{
	TRACE_BRICK_FUNC_START();
	static char *name = "Merge";
	return name;
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
brick_funcs mergefuncs = {
	.init			= 	merge_init,
	.link			=	merge_link,
	.process		= 	NULL,
	.deinit			= 	merge_deinit,
	.getId			=	merge_getid
};
/*---------------------------------------------------------------------*/
