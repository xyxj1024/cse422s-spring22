CFLAGS = -g -Wall -D_REENTRANT

SRCS = server.c server2.c server3.c server4.c client.c client2.c write_to_file.c
OBJS = $(SRCS:%.c=%.o)
PROGS = $(SRCS:%.c=%)

all : $(PROGS)

server4 : server4.o
	$(CC) -o $@ server4.o -lpthread

clean :
	rm -f $(OBJS) $(PROGS)
