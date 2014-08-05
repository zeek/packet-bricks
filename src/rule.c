/* for rule functions def'ns */
#include "rule.h"
/* for engine def'n */
#include "pkt_engine.h"
/* for logging */
#include "pacf_log.h"
/* for strcmp */
#include <string.h>
/*---------------------------------------------------------------------*/
inline void
init_rules_table(void *engptr)
{
	TRACE_RULE_FUNC_START();
	engine *eng = (engine *)engptr;

#if 0
	TAILQ_INIT(&eng->r_list);
#else
	UNUSED(eng);
#endif
	TRACE_RULE_FUNC_END();
}
/*---------------------------------------------------------------------*/
#if 0
static Rule *
rule_find(engine *eng, const uint8_t *from_cname, Target tgt)
{
	TRACE_RULE_FUNC_START();
	Rule *r;
	TAILQ_FOREACH(r, &eng->r_list, entry) {
		if (r->tgt == tgt && 
		    !strcmp((char *)r->ifname, (char *)from_cname)) {
			TRACE_RULE_FUNC_END();
			return r;
		}
	}
	TRACE_RULE_FUNC_END();
	return NULL;
}
#endif
/*---------------------------------------------------------------------*/
Rule *
add_new_rule(void *engptr, const uint8_t *from_cname, Filter *filt, Target tgt)
{
	TRACE_RULE_FUNC_START();
#if 0
	Rule *r;
#endif
	engine *eng = (engine *)engptr;

#if 0
	r = rule_find(eng, from_cname, tgt);
	if (r == NULL) {
		r = calloc(1, sizeof(Rule));
		if (r == NULL) {
			TRACE_LOG("Can't allocate mem to add a new rule for engine %s!\n",
				  eng->name);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
		r->tgt = tgt;
		strcpy((char *)r->ifname, (char *)from_cname);
	}
#else
	/* first-time rule insertion */
	if (eng->r == NULL) {
		eng->r = calloc(1, sizeof(Rule));
		if (eng->r == NULL) {
			TRACE_LOG("Can't allocate mem to add a new rule for engine: %s!\n",
				  eng->name);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
		eng->r->tgt = tgt;
		strcpy((char *)eng->r->ifname, (char *)from_cname);
	}
#endif
	if (!strcmp((char *)eng->r->ifname, (char *)from_cname) &&
	    eng->r->tgt == tgt) {
		eng->r->count++;
		eng->r->destInfo = realloc(eng->r->destInfo, sizeof(void *) * eng->r->count);
		if (eng->r->destInfo == NULL) {
			TRACE_LOG("Can't re-allocate to add a new rule for engine %s!\n",
				  eng->name);
			if (eng->r->count == 1) free(eng->r);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
	}
	
#if 0	
	if (r->count == 1)
		TAILQ_INSERT_TAIL(&eng->r_list, r, entry);
#endif
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
#if 0
	Rule *r_iter;

	/* Free the entire tail queue. */
        while ((r_iter = TAILQ_FIRST(&eng->r_list))) {
                TAILQ_REMOVE(&eng->r_list, r_iter, entry);
		eng->iom.delete_all_channels(eng, r_iter);
		free(r_iter->destInfo);
                free(r_iter);
        }
#else
	eng->iom.delete_all_channels(eng, eng->r);
	free(eng->r->destInfo);
	free(eng->r);
#endif	
	TRACE_RULE_FUNC_END();
}
/*---------------------------------------------------------------------*/
