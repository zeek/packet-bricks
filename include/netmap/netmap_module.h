#ifndef __NETMAP_MODULE_H__
#define __NETMAP_MODULE_H__
/*---------------------------------------------------------------------*/
/* for data types */
#include <stdint.h>
/* for netmap specific decls */
#define  NETMAP_WITH_LIBS		1
#include <net/netmap_user.h>
/*---------------------------------------------------------------------*/
/**
 * Private netmap module context. Please see the comments inside the
 * struct for more details
 */
typedef struct netmap_module_context {
	struct nm_desc *nmd;		/* netmap descriptor */
	int32_t fd;			/* fd for ioctl etc. */
	uint64_t nmd_flags;		/* netmap desc flags */
	unsigned char *nmr_config;	/* basic config params */
	int32_t extra_bufs;		/* do we need extra bufs */
	
} netmap_module_context __attribute__((aligned(__WORDSIZE)));
/*---------------------------------------------------------------------*/
#define DEFAULT_EXTRA_RINGS		8
/*---------------------------------------------------------------------*/
#endif /* !__NETMAP_MODULE_H__ */
