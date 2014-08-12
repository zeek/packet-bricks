#ifndef __ELEMENT_H__
#define __ELEMENT_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for Linker_Intf def'n */
#include "lua_interface.h"
/* for IFNAMSIZ */
#include <net/if.h>
/* for Target */
#include "pacf_interface.h"
/*---------------------------------------------------------------------*/
struct Element;
typedef struct element_funcs {		/* element funcs ptrs */
	int32_t (*init)(struct Element *elem, Linker_Intf *li);
	void (*link)(struct Element *elem, Linker_Intf *li);
	void (*process)(struct Element *elem, unsigned char *pktbuf);
	void (*deinit)(struct Element *elem);
} element_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
typedef struct Element
{
	void *private_data;
	struct engine *eng;
	struct Element *next_elem;
	
	struct element_funcs *elib;
} Element __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
typedef struct linkdata {
	uint8_t count;		/* # of external links */
	uint8_t init_cur_idx;	/* current index */	
	void **external_links;	/* pointers to external link contexts */
	char ifname[IFNAMSIZ];	/* name of (virtual) source */
	Target tgt;		/* type */	
} linkdata __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
extern element_funcs lbfuncs;
extern element_funcs dupfuncs;
extern element_funcs mergefuncs;
/*---------------------------------------------------------------------*/
#endif /* !__ELEMENT_H__ */
