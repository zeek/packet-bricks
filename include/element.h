#ifndef __ELEMENT_H__
#define __ELEMENT_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/*---------------------------------------------------------------------*/
struct Element;
typedef struct element_funcs {		/* element funcs ptrs */
	void (*init)(struct Element *elem, struct Element *from);
	void (*process)(struct Element *elem, unsigned char *pktbuf);
	void (*deinit)(struct Element *elem);
} element_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
typedef struct Element
{
	void *private_data;

	struct element_funcs elib;
} Element __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
extern element_funcs lbfuncs;
extern element_funcs dupfuncs;
extern element_funcs mergefuncs;
/*---------------------------------------------------------------------*/
#endif /* !__ELEMENT_H__ */
