/*
 * Copyright (c) 2014, Asim Jamshed, Robin Sommer, Seth Hall
 * and the International Computer Science Institute. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* THIS BRICK IS CURRENTLY UNDER CONSTRUCTION. PLZ CHECK BACK LATER */
/* for Brick struct */
#include "brick.h"
/* for bricks logging */
#include "bricks_log.h"
/* for pkt engine */
#include "pkt_engine.h"
/* for strcpy */
#include <string.h>
/* for linked list queue */
#include "queue.h"
/* for filter context */
#include "bricks_filter.h"
/*---------------------------------------------------------------------*/
#if 0
struct FilterContext {
	/* name of output node */
	char name[IFNAMSIZ];
	/* 
	 * the linked list ptr that will chain together
	 * all filter bricks (for bricks_filter.c). this
	 * will be used for network communication module
	 */
	TAILQ_ENTRY(FilterContext) entry;

	/* Filter list */
	flist filter_list;
	
} __attribute__((aligned(__WORDSIZE)));
#endif
/*---------------------------------------------------------------------*/
int32_t
filter_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	FilterContext *fc = calloc(1, sizeof(FilterContext));
	if (fc == NULL) {
		TRACE_LOG("Can't allocate memory for private FilterContext!\n");
		TRACE_BRICK_FUNC_END();
		return -1;
	}
	brick->private_data = fc;
	li->type = SHARE;
#if 1
#else
	strcpy(fc->name, li->output_link[0]);
	TRACE_LOG("Adding brick with output link named: %s for engine %s\n",
		  li->output_link[0], brick->eng->name);
	/* Adding filter brick to the engine's filter list */
	TAILQ_INSERT_TAIL(&brick->eng->filter_list, fc, entry);
#endif
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
	BITMAP b;

	INIT_BITMAP(b);
	SET_BIT(b, 0);
#if 1
	UNUSED(brick);
	UNUSED(buf);
#else
	FilterContext *fc;

	fc = (FilterContext *)brick->private_data;
	if (analyze_packet(buf, fc, time(NULL)))
#endif
	TRACE_BRICK_FUNC_END();
	return b;
}
/*---------------------------------------------------------------------*/
void
filter_deinit(Brick *brick)
{
	TRACE_BRICK_FUNC_START();
	free(brick->private_data);
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
