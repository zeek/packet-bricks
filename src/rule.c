/* for rule functions def'ns */
#include "rule.h"
/* for engine def'n */
#include "pkt_engine.h"
/* for logging */
#include "pacf_log.h"
/* for strcmp */
#include <string.h>
/*---------------------------------------------------------------------*/
Rule *
add_new_rule(void *engptr, const uint8_t *from_cname,
	     Filter *filt, Target tgt)
{
	TRACE_RULE_FUNC_START();
	engine *eng = (engine *)engptr;

	/* first-time rule insertion */
	if (eng->r == NULL) {
		eng->r = calloc(1, sizeof(Rule));
		if (eng->r == NULL) {
			TRACE_LOG("Can't allocate mem to add "
				  "a new rule for engine: %s!\n",
				  eng->name);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
		eng->r->tgt = tgt;
		strcpy((char *)eng->r->ifname,
		       (char *)from_cname);
	}

	if (!strcmp((char *)eng->r->ifname, (char *)from_cname) &&
	    eng->r->tgt == tgt) {
		/* if a COPY rule is added, register it with the engine */
		if (eng->r->tgt == COPY) eng->mark_for_copy = 1;
		eng->r->count++;
		eng->r->destInfo = realloc(eng->r->destInfo, 
					   sizeof(void *) * eng->r->count);
		if (eng->r->destInfo == NULL) {
			TRACE_LOG("Can't re-allocate to add a new "
				  "rule for engine %s!\n",
				  eng->name);
			if (eng->r->count == 1) free(eng->r);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
	}
	
	UNUSED(filt);
	
	TRACE_RULE_FUNC_END();
	return eng->r;
}
/*---------------------------------------------------------------------*/
void
delete_all_rules(void *engptr)
{
	TRACE_RULE_FUNC_START();
	engine *eng = (engine *)engptr;

	eng->iom.delete_all_channels(eng, eng->r);
	free(eng->r->destInfo);
	free(eng->r);

	TRACE_RULE_FUNC_END();
}
/*---------------------------------------------------------------------*/
