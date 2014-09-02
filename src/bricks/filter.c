/* for Brick struct */
#include "brick.h"
/* for bricks logging */
#include "bricks_log.h"
/* for engine declaration */
#include "pkt_engine.h"
/* for string functions */
#include <string.h>
#include "pkt_hash.h"
/*---------------------------------------------------------------------*/
int32_t
filter_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	brick->private_data = calloc(1, sizeof(linkdata));
	if (brick->private_data == NULL) {
		TRACE_LOG("Can't create private context "
			  "for filter\n");
		TRACE_BRICK_FUNC_END();
		return -1;
	}
	TRACE_BRICK_FUNC_END();
	UNUSED(li);
	return 1;
}
/*---------------------------------------------------------------------*/
static BITMAP
isTCP(Brick *brick, unsigned char *buf)
{
	TRACE_BRICK_FUNC_START();
	linkdata *lnd = brick->private_data;
	BITMAP b;

	INIT_BITMAP(b);
	uint key = pkt_hdr_hash(buf, 4, lnd->level) 
		% lnd->count;
	SET_BIT(b, key);
	
	TRACE_BRICK_FUNC_END();

	return b;
}
/*---------------------------------------------------------------------*/
void
filter_deinit(Brick *brick)
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
filter_link(struct Brick *from, Linker_Intf *linker)
{
	TRACE_BRICK_FUNC_START();
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
		eng->FIRST_BRICK(esrc)->brick = from;
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
			rc = eng->iom.create_external_link((char *)linker->input_link[j],
							   (char *)linker->output_link[i],
							   div_type, eng->FIRST_BRICK(esrc));
			if (rc == -1) {
				TRACE_LOG("Failed to open channel %s\n",
					  linker->output_link[i]);
				return;
			}
		}
	}      
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
char *
filter_getid()
{
	TRACE_BRICK_FUNC_START();
	static char *name = "Filter";
	return name;
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
brick_funcs filterfuncs = {
	.init			= 	filter_init,
	.link			=	filter_link,
	.process		= 	isTCP,
	.deinit			= 	filter_deinit,
	.getId			=	filter_getid
};
/*---------------------------------------------------------------------*/
