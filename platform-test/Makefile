all: check packing

OBJS =  check packing
CC = gcc
DEBUG = 
CFLAGS = -O2 -g -DHAVE_SPINLOCK
IFLAGS = .
LFLAGS = -Wall $(DEBUG) -lpthread


check: check.c list.h common.h mem-pool.h
	$(CC) $(CFLAGS) $<  -o $@ mem-pool.c $(LFLAGS)

packing: packing.c
	$(CC) $(CFLAGS) $<  -o $@

clean:
	$(RM) $(OBJS)

