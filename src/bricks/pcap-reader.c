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
/* for regex to filter pcap files */
#include <regex.h>
/* for engine definition */
#include "pkt_engine.h"
/* for netmap inject packet func */
#include "netmap_module.h"
/* for getting ethernet frame length */
#include <net/ethernet.h>
/* for hash function */
#include "pkt_hash.h"
/*---------------------------------------------------------------------*/
/**
 * PcapReaderContext -
 * Private context of the PcapReader element. Currently holds only
 * pcap handler. Fields may be added in the future versions 
 */
typedef struct PcapReaderContext {
	/* array of open pcap handlers */
	/* see pcapr_init() for more details */
	pcap_t **handler;
	uint8_t count_pcaps;
	uint8_t current_idx;
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
	char comp_filename[NAME_MAX];
	PcapReaderContext *prc;
	DIR *d;
	struct dirent *dir;
	regex_t regex;
	int rc;

	/* First, compile the regexp for later pcap filtering... */
	rc = regcomp(&regex, "pcap$", 0);
	if (rc) {
		TRACE_LOG("Could not compile regex for .pcap file search\n");
		goto error_exit;
	}

	/* declare the PcapReader context */
	prc = (PcapReaderContext *)calloc(1, sizeof(PcapReaderContext));
	if (prc == NULL) {
		TRACE_LOG("Can't allocate memory for pcap reader context!\n");
		goto error_exit;
	}

	/* 
	 * the first "input link" is the dir that 
	 * has all the pcap files
	 */
	d = opendir(li->input_link[0]);
	if (d == NULL) {
		TRACE_LOG("Can't open directory for pcap reading!\n");
		goto error_exit;
	}

	/* 
	 * Scan for all pcap files in the directory
	 * and then open their pcap handlers
	 */
	while ((dir = readdir(d)) != NULL) {
		/* see if it is a regular file(/dir) entry */
		if (dir->d_type == DT_REG &&
		    /* and see if it is a pcap file */
		    !regexec(&regex, dir->d_name, 0, NULL, 0)) {

			/* increment the registered file handle count */
			prc->count_pcaps++;
			/* private data will hold ptrs for pcap handlers */
			prc->handler = realloc(prc->handler,
					       prc->count_pcaps * 
					       sizeof(pcap_t *));
			if (prc->handler == NULL) {
				TRACE_LOG("Can't create private context "
					  "for PcapReader\n");
				closedir(d);
				goto error_exit;
			}
			/* full path to the pcap file */
			sprintf(comp_filename, "%s%s", 
				li->input_link[0], dir->d_name);

			prc->handler[prc->count_pcaps - 1] = 
				pcap_open_offline((char *)comp_filename, 
						  errbuf);
			if (prc->handler[prc->count_pcaps - 1] == NULL) {
				TRACE_LOG("Can't open pcap file: %s - %s",
					  dir->d_name, errbuf);
				free(prc->handler);
				free(prc);
				prc = NULL;
				closedir(d);
				goto error_exit;
			}

			TRACE_LOG("Pcap file: %s opened\n", comp_filename);
		}
	}

	/* finally set brick's private_data */
	brick->private_data = prc;

	/* close the directory now */
	closedir(d);

	/* we need to copy packets upstream */
	li->type = COPY;

	TRACE_BRICK_FUNC_END();
	return 1;

 error_exit:
	TRACE_BRICK_FUNC_END();
	return -1;
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
	linkdata *lnd;
	uint key;

	INIT_BITMAP(b);
	lnd = &(brick->lnd);
	
	/* if there is only 1 channel, no need to compute hash */
	if (lnd->count == 1) {
		SET_BIT(b, 0);
		return b;
	}
	key = pkt_hdr_hash(buf, 4, lnd->level) % lnd->count;
	SET_BIT(b, key);
	
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
	PcapReaderContext *prc = 
		(PcapReaderContext *)brick->private_data;
	free(prc->handler);

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
/**
 * Open up pcap file for reading... if pcap
 * handling in enabled..
 */
void
pcapr_link_next_pcap(engine *eng)
{
	TRACE_BRICK_FUNC_START();
	PcapReaderContext *prc = (PcapReaderContext *)eng->pcapr_context;
	if (prc == NULL ||
	    prc->count_pcaps == prc->current_idx + 1) {
		if (prc != NULL) {
			eng->pcapr_context = NULL;
			pcap_close(prc->handler[prc->current_idx - 1]);
		}
		TRACE_LOG("End of all pcap files. Quitting now.\n");
		exit(EXIT_SUCCESS);
		TRACE_BRICK_FUNC_END();
		return;
	}

	int idx = prc->current_idx++;
	TRACE_LOG("index selected is: %d\n", idx);
	/*
	 * if idx is greater than 1, 
	 * then close the previous pcap handle
	 */
	if (idx != 0) pcap_close(prc->handler[idx - 1]);
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
process_pcap_read_request(engine *eng, void *prcptr)
{
	TRACE_BRICK_FUNC_START();
	PcapReaderContext *prc;
	pcap_t *handler;
	struct pcap_pkthdr *header;
	const u_char *packet;

	prc = (PcapReaderContext *)prcptr;
	if (prc == NULL || prc->handler == NULL || 
	    prc->handler[prc->current_idx] == NULL) {
		TRACE_BRICK_FUNC_END();
		return;
	}
		
	handler = prc->handler[prc->current_idx];

	if (pcap_next_ex(handler, &header, &packet) >= 0) {
		if (header->len <= ETH_FRAME_LEN && 
		    header->len == header->caplen) {
			TRACE_DEBUG_LOG("Packet length: %d\n", header->len);
			netmap_pcap_push_pkt(eng, 
					     packet, 
					     header->len);
		}
	} else
		pcapr_link_next_pcap(eng);
	
	TRACE_BRICK_FUNC_END();
	UNUSED(eng);
}
/*---------------------------------------------------------------------*/
/**
 * This brick needs a customized link function since it needs to push
 * pkts from files
 */
void
pcapr_link(struct Brick *from, PktEngine_Intf *pe, Linker_Intf *linker)
{
	TRACE_BRICK_FUNC_START();
	int i, j, rc, flag;
	engine *eng;
	linkdata *lbd;
	int div_type = (linker->type == LINKER_DUP) ? COPY : SHARE;
	
	lbd = (linkdata *)(&from->lnd);
	eng = engine_find(from->eng->name);
	flag = 0;
	
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
		strcpy(lbd->ifname, (char *)linker->input_link[1]);
		lbd->count = linker->output_count;
		/* link the source(s) with the packet engine */
		pktengine_link_iface((uint8_t *)eng->name, 
				     (uint8_t *)linker->input_link[1], 
				     pe->batch, pe->qid);
		TRACE_LOG("Linking %s with link %s with batch size: %d and qid: %d\n",
			  eng->name, linker->input_link[1], pe->batch, pe->qid);
		
		eng->FIRST_BRICK(esrc)->brick = from;
		eng->mark_for_copy = (linker->type == COPY) ? 1 : 0;
		lbd->external_links = calloc(lbd->count,
						sizeof(void *));
		if (lbd->external_links == NULL) {
			TRACE_LOG("Can't allocate external link contexts "
				  "for load balancer\n");
			TRACE_BRICK_FUNC_END();
			return;
		} else {
			TRACE_LOG("Link %s has %d out links\n", linker->input_link[1],
				  lbd->count);
		}
	} else {
		flag = 1;
	}

	/* XXX - This line may go away in future revisions */
	if (flag == 1) j = 0;
	else j =  (!strcmp(from->elib->getId(), "PcapReader")) ? 1 : 0;

	for (; j < linker->input_count; j++) {
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
	return;
}
/*---------------------------------------------------------------------*/
brick_funcs pcaprfuncs = {
	.init			= 	pcapr_init,
	.link			=	pcapr_link,
	.process		= 	pcapr_process,
	.deinit			= 	pcapr_deinit,
	.getId			=	pcapr_getid
};
/*---------------------------------------------------------------------*/
