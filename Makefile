PWD := $(shell pwd)

USER_CFLAGS ?= -O1  -g -std=c11 -Wall -Wextra -Wl,-rpath,./.libs
#USER_CFLAGS ?= -O1 -g  -Wall -Wextra -fsanitize=thread

.PHONY:  user clean

NETWORKFRAME = libnet

all:  user

user: NETWORKFRAME net_work

SO_SOURCE = networkframe.c util.c rpc.c util-hash.c
NETWORKFRAME : $(SO_SOURCE)
	$(CC) -fPIC -shared -O1 -g -o .libs/libnet.so  $^

NET_SOURCE = main.c
NET_INCLUDE = util-hash.h  datatype.h siphash.h  util.h  networkframe.h rpc.h

net_work: $(NET_SOURCE) $(NET_INCLUDE)
	$(CC) $(USER_CFLAGS)   -o $@ $^  -L./.libs -lnet -lpthread



clean:
	$(RM) net_work ./libs/libnet.so
