#include <string.h>

#include "routing_table.h"
#include "list.h"
#include "timer_queue.h"
#include "aodv_timeout.h"
#include "parameters.h"
#include "seek_list.h"
#include "aodv_neighbor.h"
#include "nl.h"
#include "debug.h"

void rt_table_init(void)
{
	u32_t i;

	rt_tbl.num_entries = 0;
	rt_tbl.num_active = 0;

	for(i = 0; i < RT_TABLESIZE; i++)
	{
		list_init_head(&rt_tbl.tbl[i]);
	}
}

void rt_table_destroy(void)
{
	u32_t i;
	list_t *tmp = NULL, *pos = NULL;
	rt_table_t *rt = NULL;

	for(i = 0; i < RT_TABLESIZE; i++)
	{
		list_for_each_safe(pos, tmp, &rt_tbl.tbl[i])
		{
			rt = (rt_table_t *)pos;
			rt_table_delete(rt);
		}
	}
}

static u32_t hashing(struct in_addr *addr, hash_value *hash)
{
	*hash = (hash_value)addr->s_addr;

	return (*hash & RT_TABLEMASK);
}

rt_table_t *rt_table_check(struct in_addr dest)
{
	hash_value hash;
	u32_t index;
	list_t *pos;
	rt_table_t* rt;

	if(rt_tbl.num_entries == 0)
		return NULL;

	index = hashing(&dest, &hash);

	list_for_each(pos, &rt_tbl.tbl[index])
	{
		rt = (rt_table_t *)pos;

		if(rt->hash != hash)
			continue;

		if(dest.s_addr == rt->dest_addr.s_addr)
			return rt;
	}

	return NULL;
}

rt_table_t *rt_table_insert(struct in_addr dest, struct in_addr next, u32_t hops, u32_t seqno, u32_t lifetime, u8_t state, u8_t flags)
{
	rt_table_t *rt;
	list_t *pos;
	u32_t index;
	hash_value hash;

	index = hashing(&dest, &hash);

	list_for_each(pos, &rt_tbl.tbl[index])
	{
		rt = (rt_table_t *)pos;
		if(rt->dest_addr.s_addr == dest.s_addr)
		{
			DEBUG(LOG_DEBUG, 0, "%s already existed in routing table", ip_to_str(dest));
			return NULL;
		}
	}

	if((rt = (rt_table_t *)malloc(sizeof(rt_table_t))) == NULL)
	{
		DEBUG(LOG_WARNING, 0, "rt_table malloc failed");
		exit(-1);
	}

	memset(rt, 0, sizeof(rt_table_t));

	rt->dest_addr = dest;
	rt->next_hop = next;
	rt->dest_seqno = seqno;
	rt->flags = flags;
	rt->hopcnt = hops;
	rt->hash = hash;
	rt->state = state;
	
	timer_init(&rt->rt_timer, &route_expire_timeout, rt);
	timer_init(&rt->ack_timer, &rrep_ack_timeout, rt);
	timer_init(&rt->hello_timer, &hello_timeout, rt);
	
	rt->last_hello_time.tv_sec = 0;
	rt->last_hello_time.tv_usec = 0;
	rt->hello_cnt = 0;

	rt->nprecursor = 0;

	list_init_head(&rt->precursors);

	rt_tbl.num_entries++;

	DEBUG(LOG_DEBUG, 0, "Inserting %s (bucket %d) next hop %s", ip_to_str(dest), index, ip_to_str(next));

	list_add(&rt_tbl.tbl[index], &rt->l);

	if(state == INVALID)
	{
		if(flags & RT_REPAIR)
		{
			rt->rt_timer.handler = local_repair_timeout;
			lifetime = ACTIVE_ROUTE_TIMEOUT;
		}
		else
		{
			rt->rt_timer.handler = route_delete_timeout;
			lifetime = DELETE_PERIOD;
		}
	}
	else
	{
		rt_tbl.num_active++;
		nl_send_add_route_msg(dest, next, hops, lifetime, flags, this_host.dev.ifindex);
	}
////////////////////////////////////
//gateway set
///////////////////////////////////
	DEBUG(LOG_DEBUG, 0, "New timer for %s, life= %d", ip_to_str(rt->dest_addr), lifetime);

	if(lifetime != 0)
		timer_set_timeout(&rt->rt_timer, lifetime);

	if(rt->state == VALID && seek_list_remove(seek_list_check(dest)))
	{
		/////////////////////////on new route?
	}

	return rt;
}

rt_table_t *rt_table_update(rt_table_t *rt, struct in_addr next, u32_t hops, u32_t seqno, u32_t lifetime, u8_t state, u8_t flags)
{
	if(rt->state == INVALID && state == VALID)
	{
		rt_tbl.num_active++;

		if(rt->flags & RT_REPAIR)
			flags &= ~RT_REPAIR;

		nl_send_add_route_msg(rt->dest_addr, next, hops, lifetime, flags, this_host.dev.ifindex);
	}
	else if(rt->next_hop.s_addr != 0 && rt->next_hop.s_addr != next.s_addr)
	{
		DEBUG(LOG_DEBUG, 0, "rt->next_hop= %s, new_next_hop= %s", ip_to_str(rt->next_hop), ip_to_str(next));

		nl_send_add_route_msg(rt->dest_addr, next, hops, lifetime, flags, this_host.dev.ifindex);
	}

	if(hops > 1 && rt->hopcnt == 1)
	{
		rt->last_hello_time.tv_sec = 0;
		rt->last_hello_time.tv_usec = 0;
		rt->hello_cnt = 0;
		timer_remove(&rt->hello_timer);

		neighbor_link_break(rt);
	}

	rt->flags = flags;
	rt->dest_seqno = seqno;
	rt->next_hop = next;
	rt->hopcnt = hops;

	rt->rt_timer.handler = route_expire_timeout;

	if(!(rt->flags & RT_INET_DEST))
		rt_table_update_timeout(rt, lifetime);

	rt->state = state;
	
	if(rt->state == VALID && seek_list_remove(seek_list_check(rt->dest_addr)))
	{
		/////////////////////////on new route?
	}
	
	return rt;
}

rt_table_t *rt_table_update_timeout(rt_table_t *rt, u32_t lifetime)
{
	struct timeval new_timeout;

	if(!rt)
		return NULL;

	if(rt->state == VALID)
	{
		gettimeofday(&new_timeout, NULL);
		timeval_add_msec(&new_timeout, lifetime);

		if(timeval_diff(&rt->rt_timer.timeout, &new_timeout) < 0)
			timer_set_timeout(&rt->rt_timer, lifetime);
	}

	return rt;
}

void rt_table_update_route_timeouts(rt_table_t *fwd_rt, rt_table_t *rev_rt)
{
	rt_table_t *next_hop_rt = NULL;

	if(fwd_rt && fwd_rt->state == VALID)
	{
		if((fwd_rt->flags & RT_INET_DEST) || (fwd_rt->hopcnt != 1) || fwd_rt->hello_timer.used)
		{
			rt_table_update_timeout(fwd_rt, ACTIVE_ROUTE_TIMEOUT);
		}

		next_hop_rt = rt_table_check(fwd_rt->next_hop);

		if(next_hop_rt && (next_hop_rt->state == VALID) && (next_hop_rt->dest_addr.s_addr != fwd_rt->dest_addr.s_addr) && (fwd_rt->hello_timer.used))
		{
			rt_table_update_timeout(next_hop_rt, ACTIVE_ROUTE_TIMEOUT);
		}
	}

	if(rev_rt && rev_rt->state == VALID)
	{
		if((rev_rt->hello_cnt != 1) || rev_rt->hello_timer.used)
		{
			rt_table_update_timeout(rev_rt, ACTIVE_ROUTE_TIMEOUT);
		}

		next_hop_rt = rt_table_check(rev_rt->next_hop);

		if(next_hop_rt && (next_hop_rt->state == VALID) && rev_rt && (next_hop_rt->dest_addr.s_addr != rev_rt->dest_addr.s_addr) && (rev_rt->hello_timer.used))
		{
			rt_table_update_timeout(next_hop_rt, ACTIVE_ROUTE_TIMEOUT);
		}
	}
}

s32_t rt_table_invalidate(rt_table_t *rt)//route expiry and deletion
{
	struct timeval now;

	gettimeofday(&now, NULL);

	if(!rt)
		return -1;

	if(rt->state == INVALID)
	{
		DEBUG(LOG_DEBUG, 0, "Route %s already invalidated", ip_to_str(rt->dest_addr));
		return -1;
	}

	if(rt->hello_timer.used)
	{
		DEBUG(LOG_DEBUG, 0, "last HELLO: %ld", timeval_diff(&now, &rt->last_hello_time));
	}

	timer_remove(&rt->rt_timer);
	timer_remove(&rt->hello_timer);
	timer_remove(&rt->ack_timer);

	rt->state = INVALID;
	rt_tbl.num_active--;
	rt->hello_cnt = 0;

	seqno_incr(rt->dest_seqno);

	rt->last_hello_time.tv_sec = 0;
	rt->last_hello_time.tv_usec = 0;

	nl_send_del_route_msg(rt->dest_addr, rt->next_hop, rt->hopcnt);
	
	//no gateway
	

	if(rt->flags & RT_REPAIR)
	{
		rt->rt_timer.handler = local_repair_timeout;
		timer_set_timeout(&rt->rt_timer, ACTIVE_ROUTE_TIMEOUT);

		DEBUG(LOG_DEBUG, 0, "%s kept for repairs during %d msecs", ip_to_str(rt->dest_addr), ACTIVE_ROUTE_TIMEOUT);
	}
	else
	{
		rt->rt_timer.handler = route_delete_timeout;
		timer_set_timeout(&rt->rt_timer, DELETE_PERIOD);

		DEBUG(LOG_DEBUG, 0, "%s removed in %d msecs", ip_to_str(rt->dest_addr), DELETE_PERIOD);
	}

	return 0;
}

void rt_table_delete(rt_table_t *rt)
{
	if(!rt)
	{
		DEBUG(LOG_DEBUG, 0, "No route entry to delete");
		return;
	}

	list_remove(&rt->l);

	precursor_list_destroy(rt);

	if(rt->state == VALID)
	{
		nl_send_del_route_msg(rt->dest_addr, rt->next_hop, rt->hopcnt);
		rt_tbl.num_active--;
	}

	timer_remove(&rt->rt_timer);
	timer_remove(&rt->hello_timer);
	timer_remove(&rt->ack_timer);

	rt_tbl.num_entries--;

	free(rt);
}

/********************************************************************************/

/*precursor part*/

void precursor_add(rt_table_t *rt, struct in_addr addr)
{
	precursor_t *pr;
	list_t *pos;

	if(!rt)
		return;

	list_for_each(pos, &rt->precursors)
	{
		pr = (precursor_t *)pos;

		if(pr->neighbor.s_addr == addr.s_addr)
			return;
	}

	if((pr = (precursor_t *)malloc(sizeof(precursor_t))) == NULL)
	{
		DEBUG(LOG_WARNING, 0, "Prenode malloc failed");
		exit(-1);
	}

	DEBUG(LOG_DEBUG, 0, "Adding precursor %s to rte %s", ip_to_str(addr), ip_to_str(rt->dest_addr));

	pr->neighbor.s_addr = addr.s_addr;

	list_add(&rt->precursors, &pr->l);
	rt->nprecursor++;
}

void precursor_remove(rt_table_t *rt, struct in_addr addr)
{
	list_t *pos, *tmp;
	precursor_t *pr;

	if(!rt)
		return;

	list_for_each_safe(pos, tmp, &rt->precursors)
	{
		pr = (precursor_t *)pos;
		if(pr->neighbor.s_addr == addr.s_addr)
		{
			DEBUG(LOG_DEBUG, 0, "Removing precursor %s from rte %s", ip_to_str(addr), ip_to_str(rt->dest_addr));

			list_remove(pos);
			rt->nprecursor--;
			free(pr);
			return;
		}
	}
}

void precursor_list_destroy(rt_table_t *rt)
{
	list_t *pos, *tmp;
	precursor_t *pr;

	if(!rt)
		return;

	list_for_each_safe(pos, tmp, &rt->precursors)
	{
		pr = (precursor_t *)pos;
		list_remove(pos);
		rt->nprecursor--;
		free(pr);
	}
}
