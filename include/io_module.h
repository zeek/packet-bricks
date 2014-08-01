#ifndef __IO_MODULE_H__
#define __IO_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for target and rule def'ns */
#include "pacf_interface.h"
/* for rule def'n */
#include "rule.h"
/*---------------------------------------------------------------------*/
#define MAX_IFNAMELEN		64
/*---------------------------------------------------------------------*/
/**
 * io_module_funcs - contains template for the various 10Gbps pkt I/O
 * 		   - libraries that can be adopted. Please see netmap_module.c
 *		   - as reference...
 *
 *		   init_context(): Used to initialize the driver library
 *				 : Also use the context to create/initialize
 *				 : a private packet I/O context for the engine
 *
 *		   link_iface(): Used to add interface to the engine. Some
 *				 Some pkt I/O libraries can pass more than
 *				 iface to one engine (e.g. PSIO)
 *
 *		   unlink_iface(): Used to remove interface from the engine.
 *				  Call it only when the engine is not running
 *
 *		   callback(): Function that reads the packets and runs 
 *				appropriate handler
 *
 *		   create_channel(): Function that establishes comm interface
 *				     with userland processes
 *
 *		   delete_all_channels(): Function that tears down all channels
 *					 associated with a particular rule
 *
 *
 *		   shutdown(): Used to destroy the private pkt I/O-specifc 
 *			       context
 */
/*---------------------------------------------------------------------*/
typedef struct io_module_funcs {
	int32_t (*init_context)(void **ctxt, void *engptr);
	int32_t (*link_iface)(void *ctxt,
			      const unsigned char *iface, 
			      const uint16_t batchsize,
			      int8_t qid);
	void	(*unlink_iface)(const unsigned char *iface, void *engptr);
	int32_t (*callback)(void *engptr, Rule *r);
	int32_t (*create_channel)(void *engptr, Rule *r, char *in_name, char *out_name);
	int32_t (*set_action)(void *engptr, Rule *r, char *rule_args);
	void	(*delete_all_channels)(void *engptr, Rule *r);
	int32_t (*shutdown)(void *engptr);
	
} io_module_funcs __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
/* only netmap module is enabled at the moment */
extern io_module_funcs netmap_module;
#if 0
extern io_module_funcs dpdk_module;
extern io_module_funcs pfring_module;
extern io_module_funcs psio_module;
extern io_module_funcs linux_module;
extern io_module_funcs file_module;
#endif
/*---------------------------------------------------------------------*/
#endif /* !__IO_MODULE_H__ */
