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
/* for logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
/**
 *
 * ELEMENT DEFINITION
 *
 * Template for Elements defined as below. The fields are defined
 * as below:
 *
 *		private_data: Used to hold element instance-specific
 *			      private data context
 *
 *		eng: the engine that the current element belongs
 *		      (connects) to.
 *
 *		next_elem: the next element that connects to this
 *			  element (This may not be needed) XXX
 *
 *		elib: libary of element functions
 *		      - init(): initializes the element. This function
 *				can also be used to private contexts.
 *				(private_data)
 *
 *		      - link(): link connecting elements to their parents.
 *
 *		      - process(): run the Element's action function that
 *				  processes incoming packet.
 *
 *		      - deinit(): frees up resources previously allocated
 *				 by the element.
 */
/*---------------------------------------------------------------------*/
struct Element;
typedef struct element_funcs {		/* element funcs ptrs */
	int32_t (*init)(struct Element *elem, Linker_Intf *li);
	void (*link)(struct Element *elem, Linker_Intf *li);
	int32_t (*process)(struct Element *elem, unsigned char *pktbuf);
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
/**
 *
 * LINK DATA ELEMENT'S PRIVATE CONTEXT DEFINITION
 *
 * This struct is used to declare private contexts for
 * special linker elements (e.g. LoadBalancers and Duplicators) that
 * lays down the topology of PACF system...
 * 
 */
/*---------------------------------------------------------------------*/
typedef struct linkdata {
	uint8_t count;		/* # of external links */
	uint8_t init_cur_idx;	/* current index (used during initialization */	
	void **external_links;	/* pointers to external link contexts */
	char ifname[IFNAMSIZ];	/* name of (virtual) source */
	Target tgt;		/* type */	
} linkdata __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * List of external element functions
 */
extern element_funcs lbfuncs;
extern element_funcs dupfuncs;
extern element_funcs mergefuncs;
/*---------------------------------------------------------------------*/
/* creates an Element and initializes the element based on target */
Element *
createElement(Target t);
/*---------------------------------------------------------------------*/
#endif /* !__ELEMENT_H__ */
