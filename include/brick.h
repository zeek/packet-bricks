/* See LICENSE in the main directory for copyright. */
#ifndef __BRICK_H__
#define __BRICK_H__
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
 * BRICK DEFINITION
 *
 * Template for Bricks defined as below. The fields are defined
 * as below:
 *
 *		private_data: Used to hold brick instance-specific
 *			      private data context
 *
 *		eng: the engine that the current brick belongs
 *		      (connects) to.
 *
 *		next_brick: the next brick that connects to this
 *			  brick (This may not be needed) XXX
 *
 *		elib: libary of brick functions
 *		      - init(): initializes the brick. This function
 *				can also be used to private contexts.
 *				(private_data)
 *
 *		      - link(): link connecting bricks to their parents.
 *
 *		      - process(): run the Brick's action function that
 *				  processes incoming packet. Returns a
 *				  a bitmap of output links the packet needs
 *				  to be forwarded to.
 *
 *		      - deinit(): frees up resources previously allocated
 *				 by the brick.
 *
 *		      - getId(): get string id of the brick
 */
/*---------------------------------------------------------------------*/
struct Brick;
/* maximum number of channels (per parent) you can have are 32 */
#define BITMAP			uint32_t
/* maximum number of bricks you can have in packet-bricks */
#define MAX_BRICKS		128
typedef struct brick_funcs {		/* brick funcs ptrs */
	int32_t (*init)(struct Brick *brick, Linker_Intf *li);
	void (*link)(struct Brick *brick, Linker_Intf *li);
	BITMAP (*process)(struct Brick *brick, unsigned char *pktbuf);
	void (*deinit)(struct Brick *brick);
	char *(*getId)();
} brick_funcs;// __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
typedef struct Brick
{
	void *private_data;
	struct engine *eng;
	struct Brick *next_brick;
	
	struct brick_funcs *elib;
} Brick __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 *
 * LINK DATA BRICK'S PRIVATE CONTEXT DEFINITION
 *
 * This struct is used to declare private contexts for
 * special linker bricks (e.g. LoadBalancers and Duplicators) that
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
	uint8_t hash_split;	/* hash split version if the brick is load balancer */
} linkdata __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/**
 * List of external brick functions & their respective macros...
 * Please start indexing with 3
 */
enum {LINKER_LB = 3, 
      LINKER_DUP,
      LINKER_MERGE,
      LINKER_FILTER};
extern brick_funcs lbfuncs;
extern brick_funcs dupfuncs;
extern brick_funcs mergefuncs;
extern brick_funcs elibs[MAX_BRICKS];
/*---------------------------------------------------------------------*/
/* creates an Brick and initializes the brick based on target */
Brick *
createBrick(Target t);

#define FIRST_BRICK(x)		x[0]

/**
 * Initialize all brick function libraries
 */
inline void initBricks();

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
#endif /* !__BRICK_H__ */
