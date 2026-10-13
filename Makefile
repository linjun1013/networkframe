PWD := $(shell pwd)

USER_CFLAGS ?= -O1 -g  -Wall -Wextra
#USER_CFLAGS ?= -O1 -g  -Wall -Wextra -fsanitize=thread

.PHONY:  user clean

all:  user

user: net_work

net_work: main.c datatype.h util-hash.c  util-hash.h siphash.h  util.h util.c networkframe.c networkframe.h
	$(CC) $(USER_CFLAGS)  -o $@ main.c util.c networkframe.c -lpthread -levent

clean:
	$(RM) net_work
