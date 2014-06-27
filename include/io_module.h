#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/*---------------------------------------------------------------------*/
#define MAX_IFNAMELEN		64
/*---------------------------------------------------------------------*/
/**
 * XXX - To be commented later...
 */
/*---------------------------------------------------------------------*/
typedef struct io_module_funcs {
	int  (*init_context)(void **ctxt);
	int  (*link_iface)(void *ctxt,
			   const unsigned char *iface, 
			   const uint16_t batchsize);
	void (*unlink_iface)(const unsigned char *iface, void *engptr);
	void (*callback)(void *engptr);
	int (*shutdown)(void *engptr);

} io_module_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
extern io_module_funcs netmap_module;
#if 0
extern io_module_funcs dpdk_module;
extern io_module_funcs pfring_module;
extern io_module_funcs psio_module;
extern io_module_funcs linux_module;
#endif
/*---------------------------------------------------------------------*/
#endif /* !__IO_MODULE_H__ */
