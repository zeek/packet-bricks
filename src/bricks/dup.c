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
	li->type = COPY;
	TRACE_BRICK_FUNC_END();
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
	.link			=	brick_link,
	.process		= 	dup_process,
	.deinit			= 	dup_deinit,
	.getId			=	dup_getid
};
/*---------------------------------------------------------------------*/
