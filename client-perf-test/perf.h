
#ifndef GLUSTERFS_TEST_H
#define GLUSTERFS_TEST_H

#include <sys/time.h>
#include <stdio.h>

//#define DEBUG

//int nfs_server_list[] = {0,2,3,5,6,7,8,9,10,11,12,13,14,15};
int server_list[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int pid_list[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int pid_tmp[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

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

inline void perf_print_bd(char *prefix,perf stop,perf start,long long dsize){
    long long secs = stop.tv.tv_sec - start.tv.tv_sec;
    long long usecs = (secs * 1000000 + stop.tv.tv_usec - start.tv.tv_usec) ;

    printf("%s bandwidth: %lld MB in %.4f s = %.2f MB/s\n",prefix, dsize>>20 ,(double)usecs/1e6,(double)dsize / (double)usecs);
}

#endif //GLUSTERFS_TEST_H

