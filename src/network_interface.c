/* for network interface function prototypes */
#include "network_interface.h"
/* for logging */
#include "pacf_log.h"
/* for libc string functions */
#include <string.h>
/*---------------------------------------------------------------------*/
static nlist niface_list;
/*---------------------------------------------------------------------*/
void
interface_init()
{
	TRACE_IFACE_FUNC_START();

	/* creating the engine list */
	TAILQ_INIT(&niface_list);

	TRACE_IFACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
netiface *
interface_find(const char *iface)
{
	TRACE_IFACE_FUNC_START();
	netiface *nif;
	TAILQ_FOREACH(nif, &niface_list, entry) {
		if (!strcmp((char *)iface, (char *)nif->ifname)) {
			TRACE_IFACE_FUNC_END();
			return nif;
		}
	}

	TRACE_IFACE_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
netiface *
create_interface_entry(const unsigned char *iface,
		       unsigned char queues_enabled,
		       io_type iot,
		       void *ctxt,
		       engine *eng)
{
	TRACE_IFACE_FUNC_START();
	netiface *nif = calloc(1, sizeof(netiface));
	if (nif == NULL) {
		TRACE_ERR("Could not allocate memory to create "
			  "network interface for iface: %s\n", 
			  iface);
		TRACE_IFACE_FUNC_END();
		return NULL;
	}
	
	nif->ifname = (unsigned char *)strdup((char *)iface);
	if(nif->ifname == NULL) {
		TRACE_ERR("Could not strdup string\n");
		TRACE_IFACE_FUNC_END();
		return NULL;
	}
	nif->multiqueue_enabled = queues_enabled;
	nif->iot = iot;
	nif->context = ctxt;
	
	TAILQ_INIT(&nif->registered_engines);
	TAILQ_INSERT_TAIL(&nif->registered_engines, eng, if_entry);
	TAILQ_INSERT_TAIL(&niface_list, nif, entry);

	TRACE_IFACE_FUNC_END();
	return nif;
}
/*---------------------------------------------------------------------*/
void *
retrieve_and_register_interface_entry(const unsigned char *iface, 
				      unsigned char queues_enabled, 
				      io_type io,
				      engine *eng)
{
	TRACE_IFACE_FUNC_START();
	netiface *nif;
	engine *e_iter;
	/* retrieve the right interface entry */
	nif = interface_find((char *)iface);
	if (nif == NULL) {
		TRACE_ERR("interface %s not found!!!!\n", iface);
		TRACE_IFACE_FUNC_END();
		return NULL;
	}
	/* 1: check whether this iface has not already been registered by this eng */
	TAILQ_FOREACH(e_iter, &nif->registered_engines, if_entry) {
		if (!strcmp((char *)e_iter->name, (char *)eng->name)) {
			TRACE_LOG("Engine %s already has a link "
				  "to interface: %s\n",
				  eng->name, iface);
			TRACE_IFACE_FUNC_END();
			return NULL;
		}
		if (e_iter->run == 1) {
			TRACE_LOG("Please stop engine %s before linking "
				  "engine %s to interface %s\n",
				  e_iter->name, eng->name, iface);
			TRACE_IFACE_FUNC_END();
			return NULL;
		}
	}
	

	/* 2: check if it supports multiple hardware queues */
	if (nif->multiqueue_enabled == NO_QUEUES) {
		TRACE_LOG("interface %s does not support "
			  "multiple queues\n",
			  iface);
		TRACE_IFACE_FUNC_END();
		return NULL;
	}

	/* check whether I/O type is compatible */
	if (io != nif->iot) {
		TRACE_LOG("interface %s does not support the "
			  "requested I/O type\n",
			  iface);
	}
	UNUSED(queues_enabled);

	/* Check H/W queue no. */
	/* XXX - for the future - XXX */

	/* insert engine entry in the registered engines list */
	TAILQ_INSERT_TAIL(&nif->registered_engines, eng, if_entry);
	TRACE_IFACE_FUNC_END();

	return nif->context;
}
/*---------------------------------------------------------------------*/
void
unregister_interface_entry(const unsigned char *iface, engine *eng)
{
	TRACE_IFACE_FUNC_START();
	netiface *nif;
	engine *e_iter, *e_itertmp;
	netiface *n_iter, *n_itertmp;

	/* retrieve the right interface entry */
	nif = interface_find((char *)iface);
	if (nif == NULL) {
		TRACE_LOG("interface %s not found!!!!\n", iface);
		TRACE_IFACE_FUNC_END();
		return;
	}

	/* locate the right registered engine and remove it */
	TAILQ_FOREACH_SAFE(e_iter, &nif->registered_engines, if_entry, e_itertmp) {
		if (!strcmp((char *)e_iter->name, (char *)eng->name)) {
			TAILQ_REMOVE(&nif->registered_engines, e_iter, if_entry);
			TRACE_DEBUG_LOG("Removing engine %s from interface %s\n",
					eng->name, nif->ifname);
		}
	}

	/* remove the interface object if no engines are registered anymore */
	if (TAILQ_EMPTY(&nif->registered_engines)) {
		TAILQ_FOREACH_SAFE(n_iter, &niface_list, entry, n_itertmp) {
			TAILQ_REMOVE(&niface_list, n_iter, entry);
			TRACE_DEBUG_LOG("Removing interface %s from the system\n",
					nif->ifname);
		}
		free(nif->ifname);
		free(nif->context);
		free(nif);
	}

	TRACE_IFACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
unregister_all_interfaces(engine *eng)
{
	TRACE_IFACE_FUNC_START();
	netiface *n_iter, *n_itertmp;
	
	TAILQ_FOREACH_SAFE(n_iter, &niface_list, entry, n_itertmp) {
		unregister_interface_entry(n_iter->ifname, eng);
	}

	TRACE_IFACE_FUNC_END();
}
/*---------------------------------------------------------------------*/
