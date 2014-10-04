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
	brick->private_data = NULL;
	li->type = SHARE;
	TRACE_BRICK_FUNC_END();
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
/**
 * This brick needs a customized link function since it needs to push
 * packets from multiple ifaces
 */
void
merge_link(struct Brick *from, PktEngine_Intf *pe, Linker_Intf *linker)
{
	TRACE_BRICK_FUNC_START();
	int i, j, k, rc;
	engine *eng;
	linkdata *lbd;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	lbd = (linkdata *)(&from->lnd);
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

	if (eng->esrc == NULL) {
		strcpy(lbd->ifname, (char *)linker->input_link[0]);
		lbd->count = linker->output_count;
		for (i = 0; i < linker->input_count; i++) {
			/* link the source(s) with the packet engine */
			pktengine_link_iface((uint8_t *)eng->name, 
					     (uint8_t *)linker->input_link[i], 
					     pe->batch, pe->qid);
			TRACE_LOG("Linking %s with link %s with batch size: %d and qid: %d\n",
				  eng->name, linker->input_link[i], pe->batch, pe->qid);
		}
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
					TRACE_BRICK_FUNC_END();
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
