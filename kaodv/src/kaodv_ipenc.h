#ifndef _KAODV_IPENC_H_
#define _KAODV_IPENC_H_

#include <linux/ip.h>
#include <linux/skbuff.h>
#include <asm/byteorder.h>

#define IPPROTO_MIPE 55

struct min_ipenc_hdr
{
	u_int8_t protocol;
	//little endian
	u_int8_t res:7;
	u_int8_t s:1;
	u_int16_t check;
	u_int32_t daddr;
};

struct sk_buff *ip_pkt_encapsulate(struct sk_buff *skb, __u32 dest);
struct sk_buff *ip_pkt_decapsulate(struct sk_buff *skb);

#endif
