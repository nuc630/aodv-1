#include "aodv_timeout.h"
#include "aodv_rreq.h"
#include "list.h"

void rreq_record_timeout(void *arg)
{
	struct rreq_record *rec = (struct rreq_record *)arg;

	list_remove(&rec->l);
	free(rec); 
}

void rreq_blacklist_timeout(void *arg)
{
	struct rreq_blacklist *bl = (struct rreq_blacklist *)arg;

	list_remove(&bl->l);
	free(bl);
}
