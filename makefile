LINUX_ENV=$(shell getconf LONG_BIT)

ifeq ($(LINUX_ENV), 32)
MACHINE=i686
else
MACHINE=x86-64
endif

all: lf_queue.c lf_queue.h
	gcc -c -g -Wall -march=${MACHINE} lf_queue.c -I /usr/local/commlib/baselib/
