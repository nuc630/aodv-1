#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/if.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/route.h>

#include "kaodv_mod.h"
#include "kaodv_expl.h"
#include "kaodv_netlink.h"
#include "kaodv_queue.h"
#include "kaodv_ipenc.h"
#include "kaodv.h"

#define ACTIVE_ROUTE_TIMEOUT active_route_timeout

static int qual = 0;
static unsigned long pkts_dropped = 0;
int qual_th = 0;
int active_route_timeout = 3000;

MODULE_DESCRIPTION("aodv-project");
MODULE_AUTHOR("lbw");
MODULE_LICENSE("GPL");

#define ADDR_HOST 1
#define AODV_BROADCAST 2

