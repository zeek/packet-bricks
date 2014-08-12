/* header declarations */
#include "pkt_engine.h"
/* for prints etc */
#include "pacf_log.h"
/* for string functions */
#include <string.h>
/* for io modules */
#include "io_module.h"
/* for accessing pc_info variable */
#include "main.h"
/* for accessing affinity-related macros */
#include "util.h"
/* for backend */
#include "backend.h"
/*---------------------------------------------------------------------*/
static elist engine_list;
/*---------------------------------------------------------------------*/
/**
 * Load all the IO modules currently available in the system
 */
static inline void
load_io_module(engine *e) {
	TRACE_PKTENGINE_FUNC_START();
	switch(e->iot) {
	case IO_NETMAP:
		e->iom = netmap_module;
		break;
#if 0 /* disabled for the moment */
	case IO_DPDK:
		e->iom = dpdk_module;
		break;
	case IO_PFRING:
		e->iom = pfring_module;
		break;
	case IO_PSIO:
		e->iom = psio_module;
		break;
	case IO_LINUX:
		e->iom = linuxio_module;
		break;
	case IO_FILE:
		e->iom = fileio_module;
		break;
#endif
	default:
		TRACE_ERR("Control can never reach here!\n");
	}
	TRACE_PKTENGINE_FUNC_END();
	
}
/*---------------------------------------------------------------------*/
inline engine *
engine_find(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	TAILQ_FOREACH(eng, &engine_list, entry) {
		if (!strcmp((char *)name, (char *)eng->name)) {
			TRACE_PKTENGINE_FUNC_END();
			return eng;
		}
	}

	TRACE_PKTENGINE_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
static inline void
engine_remove(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng, *tmp_eng;
	for (eng = TAILQ_FIRST(&engine_list); eng != NULL; eng = tmp_eng) {
		tmp_eng = TAILQ_NEXT(eng, entry);
		if (!strcmp((char *)eng->name, (char *)name)) {
			/* Remove the eng entry from the queue. */
			TAILQ_REMOVE(&engine_list, eng, entry);
			TRACE_PKTENGINE_FUNC_END();
			return;
		}
	}
	
	/* control only comes here if the entry in the list didn't exist */
	TRACE_LOG("Engine %s did not exist in the system\n", name);
	TRACE_PKTENGINE_FUNC_END();
	return;
}
/*---------------------------------------------------------------------*/
static inline void *
engine_spawn_thread(void *engptr)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng = (engine *)engptr;

	if (eng->cpu != -1) {
		/* Affinitizing thread to engine->cpu */
	  if (set_affinity(eng->cpu, &eng->t) != 0) {
			TRACE_LOG("Failed to affintiize engine "
				  "%s thread: %p to core %d\n",
				  eng->name, &eng->t, eng->cpu);
		}
	}

	/* Flip the engine to run == 1 */
	eng->run = 1;

	/* this actually 'starts' the engine */
	initiate_backend(eng);

	TRACE_PKTENGINE_FUNC_END();
	return NULL;
}
/*---------------------------------------------------------------------*/
static inline void
engine_run(engine *eng)
{
	TRACE_PKTENGINE_FUNC_START();

	/* Start your engines now */
	TRACE_DEBUG_LOG("Engine %s starting...\n", eng->name);
	if (pthread_create(&eng->t, NULL, engine_spawn_thread, (void *)eng) != 0) {
		TRACE_PKTENGINE_FUNC_END();
		TRACE_ERR("Can't spawn thread for starting the engine!\n");
	}

	TRACE_PKTENGINE_FUNC_END();
	return;
}
/*---------------------------------------------------------------------*/
void
pktengine_init()
{
	TRACE_PKTENGINE_FUNC_START();

	/* creating the engine list */
	TAILQ_INIT(&engine_list);

	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_new(const unsigned char *name, const unsigned char *type, 
	      const int8_t cpu)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	
	eng = engine_find(name);
	if (eng != NULL) {
		TRACE_LOG("Engine with name: %s already exists\n", name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}
	
	/* add new engine */
	eng = calloc(1, sizeof(engine));
	if (eng == NULL) {
		TRACE_ERR("Can't allocate mem for engine: %s\n", name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* duplicating engine name */
	eng->name = (unsigned char *)strdup((char *)name);
	if (eng->name == NULL) {
		free(eng);
		TRACE_ERR("Can't strdup engine name: %s\n", name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/*
	 * XXX
	 * for the future: maybe we can support more 
	 * I/O engine types??? 
	 * XXX
	 */
#if 0	
	/* selecting pkt engine I/O type */
	if (!strcmp((char *)type, "netmap"))
		eng->iot = IO_NETMAP;
	else if (!strcmp((char *)type, "dpdk"))
		eng->iot = IO_DPDK;
	else if (!strcmp((char *)type, "psio"))
		eng->iot = IO_PSIO;
	else if (!strcmp((char *)type, "pfring"))
		eng->iot = IO_PFRING;
	else if (!strcmp((char *)type, "linux"))
		eng->iot = IO_LINUX;
	else if (!strcmp((char *)type, "file"))
		eng->iot = IO_FILE;
#endif

	/* default pkt I/O engine is NETMAP for the moment */
	eng->iot = IO_DEFAULT;
	
	/* load the right I/O module */
	load_io_module(eng);

	/* initialize type-specific private context */
	if (eng->iom.init_context(&eng->private_context, eng) == -1) {
		/* if init fails, free up everything */
		if (eng->private_context != NULL)
			free(eng->private_context);
		free(eng->name);
		free(eng);
		TRACE_ERR("Can't create private context for engine: %s\n", 
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* setting the cpu no. on which the engine runs (if reqd.) */
	eng->cpu = cpu;

	/* finally add the engine entry in elist */
	TAILQ_INSERT_TAIL(&engine_list, eng, entry);

	UNUSED(type);
	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_delete(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;

	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n", 
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* check if engine is running */
	if (eng->run != 0) {
		TRACE_LOG("Can't delete engine %s.. it is already running\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* delete all channels for now */
	eng->iom.delete_all_channels(eng, eng->elem);
	
	/* check if ifaces have been unlinked */
	eng->iom.unlink_iface((const unsigned char *)"all", eng);

	/* remove the entry from the engine list */
	engine_remove(eng->name);

	/* now delete it */
	free(eng->name);

	/* free engine link name, if possible */
	if (eng->link_name) {
		free(eng->link_name);
		eng->link_name = NULL;
	}
	/* free the private context as well */
	if (eng->private_context != NULL) {
		free(eng->private_context);
		eng->private_context = NULL;
	}
	free(eng);
	TRACE_DEBUG_LOG("Engine %s has successfully been deleted\n",
			name);

	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_link_iface(const unsigned char *name, 
		     const unsigned char *iface,
		     const int16_t batch_size,
		     const int8_t queue)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	
	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}
	
	/* check if the engine is already running */
	if (eng->run != 0) {
		TRACE_LOG("Can't link interface %s." 
			  " Engine %s is already running.\n",
			  iface, name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* 
	 * Link the interface now...
	 * If the batch size is not given, then use
	 * the default batch size taken from pc_info.
	 * If the queue is negative, use the interface
	 * without queues.
	 */
	eng->link_name = (uint8_t *)strdup((char *)iface);
	if (eng->link_name == NULL) {
		TRACE_LOG("Could not strdup link_name while linking %s to engine %s\n",
			  iface, eng->name);
	}
	eng->dev_fd = eng->iom.link_iface(eng->private_context, iface, 
					  (batch_size == -1) ? 
					  pc_info.batch_size : batch_size,
					  queue);
	if (eng->dev_fd == -1) 
		TRACE_LOG("Could not link!!!\n");
	
	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_unlink_iface(const unsigned char *name,
		       const unsigned char *iface)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	
	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* check if the engine is already running */
	if (eng->run != 0) {
		TRACE_LOG("Can't unlink interface %s." 
			  " Engine %s is already running.\n",
			  iface, name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* unlink */
	eng->iom.unlink_iface(iface, eng);
	
	/* free up the link_name */
	free(eng->link_name);
	eng->link_name = NULL;

	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_start(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	
	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* check if the engine is already running */
	if (eng->run != 0) {
		TRACE_LOG("Can't start the engine." 
			  " Engine %s is already running.\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* start sniffing (to be set-up) */
	engine_run(eng);

	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_stop(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	int rc;
	
	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* check if the engine has already stopped */
	if (eng->run == 0) {
		TRACE_LOG("Can't stop the engine." 
			  " Engine %s is already non-functional.\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* stop sniffing */
	rc = eng->iom.shutdown(eng);
	if (rc == -1) {
		TRACE_LOG("Engine was not running\n");
	}
	
	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengine_dump_stats(const unsigned char *name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;

	eng = engine_find(name);
	if (eng == NULL) {
		TRACE_LOG("Can't find engine with name: %s\n",
			  name);
		TRACE_PKTENGINE_FUNC_END();
		return;
	}

	/* Engine stats */
	fprintf(stdout, "---------- ENGINE (%s) STATS------------\n", name);
	fprintf(stdout, "Byte count: %llu\n", (long long unsigned int)eng->byte_count);
	fprintf(stdout, "Packet count: %llu\n", (long long unsigned int)eng->pkt_count);
	fprintf(stdout, "----------------------------------------\n\n");
	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
void
pktengines_list_stats(FILE *f)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	uint64_t total_pkts, total_bytes;

	total_pkts = total_bytes = 0;
	fprintf(f, "----------------------------------------- ENGINE STATISTICS");
	fprintf(f, " --------------------------------------------\n");
	fprintf(f, "Engine \t\t Packet Cnt \t\t    Byte Cnt \t\t Listen-port\n");
	TAILQ_FOREACH(eng, &engine_list, entry) {
		fprintf(f, "%s \t\t %10llu \t\t %11llu \t\t\t%d\n",
			eng->name, 
			(long long unsigned int)eng->pkt_count, 
			(long long unsigned int)eng->byte_count,
			eng->listen_port);
		total_pkts += eng->pkt_count;
		total_bytes += eng->byte_count;
	}
	fprintf(f, "====================================================");
	fprintf(f, "====================================================\n");
	fprintf(f, "Total \t\t %10llu \t\t %11llu\n",
		(long long unsigned int)total_pkts,
		(long long unsigned int)total_bytes);
	fprintf(f, "----------------------------------------------------");
	fprintf(f, "----------------------------------------------------\n\n\n");
	
	TRACE_PKTENGINE_FUNC_END();
}
/*---------------------------------------------------------------------*/
/**
 * Extremely untidy version.. Can this function be improved??.. meaning
 * more concise and less branched statements????
 */
uint8_t
is_pktengine_online(const unsigned char *eng_name)
{
	TRACE_PKTENGINE_FUNC_START();
	engine *eng;
	uint8_t any = 0;
	uint8_t all = 1;

	if (TAILQ_EMPTY(&engine_list))
		return 0;
	
	TAILQ_FOREACH(eng, &engine_list, entry) {
		if (eng->run == 0) {
			if (!strcmp((char *)eng->name, (char *)eng_name)) {
				TRACE_PKTENGINE_FUNC_END();
				return 0;
			}
			all = 0;
		} else { /* eng->run == 1 */
			if (!strcmp((char *)eng->name, (char *)eng_name)) {
				TRACE_PKTENGINE_FUNC_END();
				return 1;
			}
			any = 1;
		}
	}

	if (!strcmp((char *)eng_name, "all"))
		return all;
	if (!strcmp((char *)eng_name, "any"))
		return any;

	/* 
	 * control will only come here if the engine 
	 * with $eng_name does not exist in the system
	 */
	TRACE_PKTENGINE_FUNC_END();
	return 0;
}
/*---------------------------------------------------------------------*/
