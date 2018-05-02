all: write read aggregate

CFLAGS = -O2
CC = gcc
HEADERS = .

write:   write.c perf.h
	$(CC) $(CFLAGS) -I $(HEADERS) $< -o $@ 
read:   read.c perf.h
	$(CC) $(CFLAGS) -I $(HEADERS) $< -o $@ 
aggregate:  aggregate.c perf.h
	$(CC) $(CFLAGS) -I $(HEADERS) $< -o $@ 
clean:
	rm write read aggregate
