#ifndef _AODV_NEIGHBOR_H_
#define _AODV_NEIGHBOR_H_

#include "defs.h"
#include "routing_table.h"

void neighbor_add(AODV_msg *aodv_msg, struct in_addr src);
void neighbor_link_break(rt_table_t *rt);

#endif
