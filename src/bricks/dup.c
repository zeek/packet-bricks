/* See LICENSE in the main directory for copyright. */
/* for Brick struct */
#include "brick.h"
/* for bricks logging */
#include "bricks_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for strcmp */
#include <string.h>
/*---------------------------------------------------------------------*/
int32_t
dup_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	brick->private_data = calloc(1, sizeof(linkdata));
	if (brick->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for duplicator\n");
		TRACE_BRICK_FUNC_END();
		return -1;
	}
	TRACE_BRICK_FUNC_END();
	UNUSED(li);
	return 1;
}
/*---------------------------------------------------------------------*/
BITMAP
dup_process(Brick *brick, unsigned char *buf)
{
	TRACE_BRICK_FUNC_START();
	linkdata *lnd = brick->private_data;
	BITMAP b;
	int i;

	INIT_BITMAP(b);
	for (i = 0; i < lnd->count; i++) {
		SET_BIT(b, i);
	}
	
	TRACE_BRICK_FUNC_END();
	UNUSED(buf);
	return b;
}
/*---------------------------------------------------------------------*/
void
dup_deinit(Brick *brick)
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
dup_link(struct Brick *brick, Linker_Intf *linker)
{
	TRACE_BRICK_FUNC_START();
	int i, j, rc;
	engine *eng;
	linkdata *dupdata;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	dupdata = (linkdata *)brick->private_data;
	eng = engine_find(brick->eng->name);
	/* sanity engine check */
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  brick->eng->name);
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
		strcpy(dupdata->ifname, (char *)linker->input_link[0]);
		dupdata->count = linker->output_count;
		eng->FIRST_BRICK(esrc)->brick = brick;
		eng->mark_for_copy = 1;
		dupdata->external_links = calloc(dupdata->count,
						 sizeof(void *));
		if (dupdata->external_links == NULL) {
			TRACE_LOG("Can't allocate external link contexts "
				  "for duplicator\n");
			TRACE_BRICK_FUNC_END();
			return;
		}
	}

	for (j = 0; j < linker->input_count; j++) { 
		for (i = 0; i < linker->output_count; i++) {
			rc = eng->iom.create_external_link((char *)linker->input_link[j],
							   (char *)linker->output_link[i],
							   div_type, eng->FIRST_BRICK(esrc));
			if (rc == -1) {
				TRACE_LOG("Failed to open channel %s\n",
					  linker->output_link[i]);
				TRACE_BRICK_FUNC_END();
				return;
			}
		}
	}

	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
char *
dup_getid()
{
	TRACE_BRICK_FUNC_START();
	static char *name = "Duplicator";
	return name;
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
brick_funcs dupfuncs = {
	.init			= 	dup_init,
	.link			=	dup_link,
	.process		= 	dup_process,
	.deinit			= 	dup_deinit,
	.getId			=	dup_getid
};
/*---------------------------------------------------------------------*/
