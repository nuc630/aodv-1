#ifndef _AODV_SOCKET_H_
#define _AODV_SOCKET_H_

#include "defs.h"

#define RECE_BUF_SIZE 1024
#define SEND_BUF_SIZE RECE_BUF_SIZE

void aodv_socket_init(void);
u8_t *aodv_socket_new_msg(void);
void aodv_socket_send(AODV_msg *aodv_msg, struct in_addr dest, u32_t len, u8_t ttl, struct dev_info *dev);

#endif
