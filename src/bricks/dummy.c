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
/*---------------------------------------------------------------------*/
int32_t
dummy_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	brick->private_data = NULL;
	li->type = SHARE;
	TRACE_LOG("Adding brick dummy to the engine\n");	
	TRACE_BRICK_FUNC_END();

	return 1;
}
/*---------------------------------------------------------------------*/
BITMAP
dummy_process(Brick *brick, unsigned char *buf)
{
	TRACE_BRICK_FUNC_START();
	BITMAP b;

	/* straigt in... and straight out */
	INIT_BITMAP(b);
	SET_BIT(b, 0);
	TRACE_BRICK_FUNC_END();
	return b;
	UNUSED(brick);
	UNUSED(buf);
}
/*---------------------------------------------------------------------*/
void
dummy_deinit(Brick *brick)
{
	TRACE_BRICK_FUNC_START();
	free(brick);
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
char *
dummy_getid()
{
	TRACE_BRICK_FUNC_START();
	static char *name = "Dummy";
	return name;
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
brick_funcs dummyfuncs = {
	.init			= 	dummy_init,
	.link			=	brick_link,
	.process		= 	dummy_process,
	.deinit			= 	dummy_deinit,
	.getId			=	dummy_getid
};
/*---------------------------------------------------------------------*/
