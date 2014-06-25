#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/*---------------------------------------------------------------------*/
/**
 * XXX - To be commented later...
 */
/*---------------------------------------------------------------------*/
typedef struct io_module_funcs {
	void (*link_iface)(const unsigned char *iface, 
			   const uint16_t batchsize);
	void (*unlink_iface)(const unsigned char *iface);
	void (*callback)(void *handle, void *pkts);
	void (*shutdown)(void *handle);
	
} io_module_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
extern io_module_funcs netmap_module;
#if 0
extern io_module_funcs pfring_module;
extern io_module_funcs psio_module;
extern io_module_funcs linux_module;
#endif
/*---------------------------------------------------------------------*/
#endif /* !__IO_MODULE_H__ */
