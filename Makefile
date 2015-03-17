CC = mipsel-linux-gcc
CCFLAGS = -Wall
INCLUDES =
LIBS = 
SRCS = $(shell echo *.c)
OBJS = $(SRCS:.c = .o)
TARGET = aodv

$(TARGET) : $(OBJS)
	$(CC) $(CCFLAGS) $(INCLUDES) $(LIBS) $^ -o $@ 

aodv_rreq.o: aodv_rreq.c defs.h aodv_rreq.h list.h aodv_socket.h routing_table.h timer_queue.h parameters.h aodv_timeout.h
aodv_socket.o: aodv_socket.c defs.h aodv_socket.h aodv_rreq.h timer_queue.h
list.o: list.c defs.h list.h
main.o: main.c defs.h aodv_socket.h 
routing_table.o: routing_table.c defs.h routing_table.h list.h timer_queue.h
timer_queue.o: timer_queue.c defs.h timer_queue.h list.h
aodv_timeout.o: aodv_timeout.c defs.h aodv_timeout.h aodv_rreq.h list.h

.PHONY:clean 
clean:
	-rm -f *.o $(TARGET)


