all: test write read aggregate-read aggregate-write
API_HEADER = /usr/local/include/glusterfs/api

test:  test.c test.h
	gcc -O2 -o test test.c -L /usr/local/lib -lgfapi -I $(API_HEADER) -g 
write:   write.c test.h
	gcc -O2 -o write write.c -L /usr/local/lib  -lgfapi -I $(API_HEADER) -g -lpthread
read:   read.c test.h
	gcc -O2 -o read read.c -L /usr/local/lib  -lgfapi -I $(API_HEADER) -g
aggregate-write:  aggregate-write.c test.h
	gcc -O2 -o aggregate-write aggregate-write.c -L /usr/local/lib -lgfapi -I $(API_HEADER) -g 
aggregate-read:  aggregate-read.c test.h
	gcc -O2 -o aggregate-read aggregate-read.c -L /usr/local/lib -lgfapi -I $(API_HEADER) -g 
clean:
	rm test write read aggregate-read aggregate-write
