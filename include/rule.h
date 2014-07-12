#ifndef __RULE_H__
#define __RULE_H__
/*---------------------------------------------------------------------*/
/* for tailq */
#include "queue.h"
/* for Rule def'n fields */
#include "pacf_interface.h"
/*---------------------------------------------------------------------*/
/** 
 * XXX: This is the rule definition that will make the entire rule table
 * in the system. This is under construction...
 */
typedef struct Rule {
	Target tgt;
	unsigned count;
	void **destInfo;
	
	TAILQ_ENTRY(Rule) entry;
} Rule __attribute__((aligned(__WORDSIZE)));

typedef TAILQ_HEAD(rule_list, Rule) rule_list;
/*---------------------------------------------------------------------*/
#endif /* !__RULE_H__*/
