#ifndef __FILTER_H__
#define __FILTER_H__
/*---------------------------------------------------------------------*/
#include "pacf_interface.h"
/*---------------------------------------------------------------------*/
/**
 * Template file for filter functions
 */

/**
 * Test if a given filter blocks the incoming packet.
 * If the filter accepts the pkt, it returns 1.
 * Otherwise it returns 0
 */
uint32_t
pass_pkt_against_filter(Filter *f, unsigned char *buf);


/**
 * Adds a new filter to the registered interface
 */
void
process_filter_request(unsigned char *ifname, Filter *f);
/*---------------------------------------------------------------------*/
#endif /* !__FILTER_H__ */
