all: fill dir-op p-write

OBJS = fill dir-op p-write
CC = g++
DEBUG = 
CFLAGS = -O2
IFLAGS = .
LFLAGS = -Wall $(DEBUG)

fill: client-write.c
	$(CC) $(CFLAGS) $< -I $(IFLAGS) -o $@

dir-op: client-dir-ops.c
	$(CC) $(CFLAGS) $< -I $(IFLAGS) -o $@

p-write: parallel_write.c
	$(CC) $(CFLAGS) $< -I $(IFLAGS) -o $@
#run-fill
	#./$<

clean:
	$(RM) $(OBJS)

