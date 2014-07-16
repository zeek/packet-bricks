/* for rule functions def'ns */
#include "rule.h"
/* for engine def'n */
#include "pkt_engine.h"
/* for logging */
#include "pacf_log.h"
/*---------------------------------------------------------------------*/
inline void
init_rules_table(void *engptr)
{
	TRACE_RULE_FUNC_START();
	engine *eng = (engine *)engptr;

	TAILQ_INIT(&eng->r_list);
	TRACE_RULE_FUNC_END();
}
/*---------------------------------------------------------------------*/
static Rule *
rule_find(engine *eng, Target tgt)
{
	TRACE_RULE_FUNC_START();
	Rule *r;
	TAILQ_FOREACH(r, &eng->r_list, entry) {
		if (r->tgt == tgt) {
			TRACE_RULE_FUNC_END();
			return r;
		}
	}
	TRACE_RULE_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
Rule *
add_new_rule(void *engptr, Filter *filt, Target tgt)
{
	TRACE_RULE_FUNC_START();
	Rule *r;
	engine *eng = (engine *)engptr;

	r = rule_find(eng, tgt);
	if (r == NULL) {
		r = calloc(1, sizeof(Rule));
		if (r == NULL) {
			TRACE_LOG("Can't allocate mem to add a new rule for engine %s!\n",
				  eng->name);
			TRACE_RULE_FUNC_END();
			return NULL;
		}
		r->tgt = tgt;
	}

	r->count++;
	r->destInfo = realloc(r->destInfo, sizeof(void *) * r->count);
	if (r->destInfo == NULL) {
		TRACE_LOG("Can't re-allocate to add a new rule for engine %s!\n",
			  eng->name);
		if (r->count == 1) free(r);
		TRACE_RULE_FUNC_END();
		return NULL;
	}
	
	if (r->count == 1)
		TAILQ_INSERT_TAIL(&eng->r_list, r, entry);
	UNUSED(filt);

	TRACE_RULE_FUNC_END();
	return r;
}
/*---------------------------------------------------------------------*/
void
delete_all_rules(void *engptr)
{
	TRACE_RULE_FUNC_START();
	engine *eng = (engine *)engptr;
	Rule *r_iter;

	/* Free the entire tail queue. */
        while ((r_iter = TAILQ_FIRST(&eng->r_list))) {
                TAILQ_REMOVE(&eng->r_list, r_iter, entry);
		eng->iom.delete_all_channels(eng, r_iter);
		free(r_iter->destInfo);
                free(r_iter);
        }
	
	TRACE_RULE_FUNC_END();
}
/*---------------------------------------------------------------------*/
