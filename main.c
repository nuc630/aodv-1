#include "defs.h"
#include "aodv_socket.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

s32_t wait_on_reboot = 1;
s32_t ratelimit = 1;

int main(int argc, char *argv[])
{
/*	fd_set listener;
	struct timeval tv;
	s32_t retval;
	struct in_addr dest;

	this_host.seqno = 10;
	this_host.rreq_id = 10;
	this_host.last_broadcast_time.tv_sec = 0;
	this_host.last_broadcast_time.tv_usec = 0;
	this_host.dev.enabled = 1;
	strcpy(this_host.dev.ifname, "br-lan");
	this_host.dev.ipaddr.s_addr = inet_addr("192.168.1.1");
		
	aodv_socket_init();

	printf("Init succeed!\n");

	if(argc > 1)
	{
		dest.s_addr = inet_addr(argv[1]);
		rreq_send(dest, atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	}

	FD_ZERO(&listener);
	FD_SET(this_host.dev.sock, &listener);
	while(1)
	{
		tv.tv_sec = tv.tv_usec = 0;	
		retval = select(this_host.dev.sock + 1, &listener, NULL, NULL, &tv);
		if(retval < 0)
			printf("Select failed!\n");
		if(FD_ISSET(this_host.dev.sock, &listener))
			aodv_socket_read(this_host.dev.sock);
	}*/

	while(1)
	{

	}

	return 0;
}
