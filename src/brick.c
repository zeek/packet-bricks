/* for Brick def'n */
#include "brick.h"
/* for logging */
#include "bricks_log.h"
/*---------------------------------------------------------------------*/
brick_funcs elibs[MAX_BRICKS];
/*---------------------------------------------------------------------*/
/**
 * Initialize all brick function libraries
 */
inline void
initBricks()
{
	TRACE_BRICK_FUNC_START();
	/* register your bricks here */
	/* 
	 * indices 0, 1, 2..5 are reserved. Please start 
	 * your indexing from 6.
	 *
	 * Maximum # of bricks packet-bricks can support
	 * is MAX_BRICKS.
	 */
	elibs[3] = lbfuncs;
	elibs[4] = dupfuncs;
	elibs[5] = mergefuncs;
	/* delimiter */
	elibs[6] = (brick_funcs){NULL, NULL, NULL, NULL, NULL};
	TRACE_BRICK_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * See brick.h for comments. Initializes the brick given
 * the target.
 */
inline Brick *
createBrick(Target t)
{
	TRACE_BRICK_FUNC_START();
	Brick *brick = calloc(1, sizeof(Brick));
	if (brick == NULL) {
		TRACE_ERR("Can't create brick: %s\n",
			  (t == (Target)LINKER_LB) ? 
			  "LoadBalancer" : "Duplicator");
		TRACE_BRICK_FUNC_END();
		return NULL;
	}
	brick->elib = &elibs[t];

	TRACE_BRICK_FUNC_END();
	return brick;
}
/*---------------------------------------------------------------------*/

