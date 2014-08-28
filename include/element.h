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
#include "bricks_interface.h"
/* for logging */
#include "bricks_log.h"
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
 *				  processes incoming packet. Returns a
 *				  a bitmap of output links the packet needs
 *				  to be forwarded to.
 *
 *		      - deinit(): frees up resources previously allocated
 *				 by the element.
 *
 *		      - getId(): get string id of the element
 */
/*---------------------------------------------------------------------*/
struct Element;
/* maximum number of channels (per parent) you can have are 32 */
#define BITMAP			uint32_t
/* maximum number of elements you can have in packet-bricks */
#define MAX_ELEMENTS		128
typedef struct element_funcs {		/* element funcs ptrs */
	int32_t (*init)(struct Element *elem, Linker_Intf *li);
	void (*link)(struct Element *elem, Linker_Intf *li);
	BITMAP (*process)(struct Element *elem, unsigned char *pktbuf);
	void (*deinit)(struct Element *elem);
	char *(*getId)();
} element_funcs;// __attribute__((aligned(__WORDSIZE)));
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
 * lays down the topology of BRICKS system...
 * 
 */
/*---------------------------------------------------------------------*/
typedef struct linkdata {
	uint8_t count;		/* # of external links */
	uint8_t init_cur_idx;	/* current index (used during initialization */	
	void **external_links;	/* pointers to external link contexts */
	char ifname[IFNAMSIZ];	/* name of (virtual) source */
	Target tgt;		/* type */	
	unsigned char level;	/* the nested level used during dispatch_pkt() */
	uint8_t hash_split;	/* hash split version if the element is load balancer */
} linkdata __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * List of external element functions & their respective macros...
 * Please start indexing with 3
 */
enum {LINKER_LB = 3, 
      LINKER_DUP,
      LINKER_MERGE,
      LINKER_FILTER};
extern element_funcs lbfuncs;
extern element_funcs dupfuncs;
extern element_funcs mergefuncs;
extern element_funcs elibs[MAX_ELEMENTS];
/*---------------------------------------------------------------------*/
/* creates an Element and initializes the element based on target */
Element *
createElement(Target t);

#define FIRST_ELEM(x)		x[0]

/**
 * Initialize all element function libraries
 */
inline void initElements();

/*---------------------------------------------------------------------*/
/**
 * BITMAP-related routines..
 */
/*---------------------------------------------------------------------*/
/**
 * Initializes the bitmap
 */
#define INIT_BITMAP(x)		x = 0;

/**
 * Set bitmap at a given position
 */
#define SET_BIT(x, val)		x |= (1 << (val))

/**
 * Clear bitmap at a given position
 */
#define CLR_BIT(x, val)		x &= ~(1 << (val))

/**
 * Toggle a bit in a bitmap
 */
#define TOGGLE_BIT(x, val)	x ^= (1 << (val))

/**
 * Check if a given position is set
 */
#define CHECK_BIT(x, val)	(x >> (val)) & 1
/*---------------------------------------------------------------------*/
#endif /* !__ELEMENT_H__ */
