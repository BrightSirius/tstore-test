
#ifndef GLUSTERFS_TEST_H
#define GLUSTERFS_TEST_H

#include <sys/time.h>
#include <stdio.h>

typedef struct _perf{
    struct timeval tv;
}perf;

inline int perf_start(perf *p){
    return gettimeofday(&(p->tv),0);
}

inline int perf_stop(perf *p){
    return gettimeofday(&(p->tv),0);
}

inline long long get_usecs(perf stop,perf start){
    long long secs = stop.tv.tv_sec - start.tv.tv_sec;
    long long usecs = (secs * 1000000 + stop.tv.tv_usec - start.tv.tv_usec) ;

	return usecs;

}
inline void perf_print_sec(perf stop,perf start){
    long long secs = stop.tv.tv_sec - start.tv.tv_sec;
    long long usecs = (secs * 1000000 + stop.tv.tv_usec - start.tv.tv_usec) ;

    printf("runtime = %.4f s\n",(double)usecs / 1e6);
}

inline void perf_print_bd(perf stop,perf start,long long dsize){
    long long secs = stop.tv.tv_sec - start.tv.tv_sec;
    long long usecs = (secs * 1000000 + stop.tv.tv_usec - start.tv.tv_usec) ;

    printf("bandwidth: %lld MB in %.4f s = %.2f MB/s\n",dsize>>20 ,(double)usecs/1e6,(double)dsize / (double)usecs);
}

#endif //GLUSTERFS_TEST_H

