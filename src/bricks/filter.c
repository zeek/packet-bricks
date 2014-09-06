/* See LICENSE in the main directory for copyright. */
/* THIS BRICK IS CURRENTLY UNDER CONSTRUCTION. PLZ CHECK BACK LATER */
/* for Brick struct */
#include "brick.h"
/* for bricks logging */
#include "bricks_log.h"
#if 0
/* for engine declaration */
#include "pkt_engine.h"
/* for string functions */
#include <string.h>
#endif
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
	li->type = SHARE;
	TRACE_BRICK_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
/**
 * XXX - Under construction. This function only forwards packets
 * based on packet header data for the time being...
 */
static BITMAP
filter_dummy(Brick *brick, unsigned char *buf)
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
	.link			=	brick_link,
	.process		= 	filter_dummy,
	.deinit			= 	filter_deinit,
	.getId			=	filter_getid
};
/*---------------------------------------------------------------------*/
