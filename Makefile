PWD := $(shell pwd)

USER_CFLAGS ?= -O1 -g -std=c11 -Wall -Wextra
#USER_CFLAGS ?= -O1 -g  -Wall -Wextra -fsanitize=thread

.PHONY:  user clean

NETWORKFRAME = libnetframe.so

all:  user

user: net_work

net_work: main.c  util-hash.c  util-hash.h  datatype.h siphash.h  util.h util.c networkframe.c networkframe.h rpc.h rpc.c
	$(CC) $(USER_CFLAGS)  -o $@ main.c util.c util-hash.c networkframe.c rpc.c -lpthread

NETWORKFRAME : networkframe.c util.c
	$(CC) -fPIC -shared -o   $@ $^


clean:
	$(RM) net_work
