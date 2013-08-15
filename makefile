LINUX_ENV=$(shell getconf LONG_BIT)

ifeq ($(LINUX_ENV), 32)
MACHINE=i686
else
MACHINE=x86-64
endif

all: lf_queue.c lf_queue.h
	gcc -o server test_server.c lf_queue.c -march=${MACHINE} -g -Wall
	gcc -o client test_client.c lf_queue.c -march=${MACHINE} -g -Wall

