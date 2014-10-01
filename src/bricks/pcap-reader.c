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
/* for pcap reading */
#include <pcap.h>
/* for posix file operations */
#include <dirent.h>
/* for string functions */
#include <string.h>
/*---------------------------------------------------------------------*/
typedef struct PcapReaderContext {
	pcap_t **handler;
} PcapReaderContext __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * XXX - Under construction. This function will read list of pcap files
 * to be read...
 */
int32_t
pcapr_init(Brick *brick, Linker_Intf *li)
{
	TRACE_BRICK_FUNC_START();
	char errbuf[PCAP_ERRBUF_SIZE];
	PcapReaderContext *prc;
	DIR *d;
	struct dirent *dir;
	int count_pcaps = 0;

	/* 
	 * the first "input link" is the dir that 
	 * has all the pcap files
	 */
	d = opendir(li->input_link[0]);
	if (d == NULL) {
		TRACE_LOG("Can't open directory for pcap reading!\n");
		TRACE_BRICK_FUNC_END();
		return -1;
	}
	/* 
	 * Scan for all pcap files in the directory
	 * and then open their pcap handlers
	 */
	while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_REG) {
			printf("%s\n", dir->d_name);
			count_pcaps++;
			brick->private_data = realloc(brick->private_data, 
						      count_pcaps * 
						      sizeof(PcapReaderContext));
			if (brick->private_data == NULL) {
				TRACE_LOG("Can't create private context "
					  "for PcapReader\n");
				TRACE_BRICK_FUNC_END();
				return -1;
			}
			/* XXX- temp solution for the time being */
			/* This will eventually change... */
			/*----.....----*/
			prc = (PcapReaderContext *)brick->private_data;
			prc->handler[count_pcaps - 1] = 
				pcap_open_offline((char *)dir->d_name, 
						  errbuf);
			if (prc->handler[count_pcaps - 1] == NULL) {
				TRACE_LOG("Can't open pcap file: %s - %s",
					  dir->d_name, errbuf);
				TRACE_BRICK_FUNC_END();
				return -1;
			}
			/*----.....----*/
		}
	}
	closedir(d);
	li->type = COPY;
	TRACE_BRICK_FUNC_END();
	return 1;
}
/*---------------------------------------------------------------------*/
/**
 * XXX
 * Straight in... and straight out!
 * pcap handler redirects all pkts read from the file to
 * the *sole* (connected) netmap file descriptor
 */
static BITMAP
pcapr_process(Brick *brick, unsigned char *buf)
{
	TRACE_BRICK_FUNC_START();
	PcapReaderContext *prc = (PcapReaderContext *)brick->private_data;
	BITMAP b;

	INIT_BITMAP(b);
	SET_BIT(b, 0);
	
	TRACE_BRICK_FUNC_END();
	UNUSED(prc);
	UNUSED(buf);
	return b;
}
/*---------------------------------------------------------------------*/
void
pcapr_deinit(Brick *brick)
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
pcapr_getid()
{
	TRACE_BRICK_FUNC_START();
	static char *name = "PcapReader";
	return name;
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
brick_funcs pcaprfuncs = {
	.init			= 	pcapr_init,
	.link			=	brick_link,
	.process		= 	pcapr_process,
	.deinit			= 	pcapr_deinit,
	.getId			=	pcapr_getid
};
/*---------------------------------------------------------------------*/
