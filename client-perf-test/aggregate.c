#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include <errno.h>
#include "perf.h"

long long usecs[sizeof(pid_list) / sizeof(int)];

static int client_cnt;

#define TIMES 10

void dispatch_binary(char *mode){
    int i,status;

    for(i=0;i<client_cnt;i++){
        if(!(pid_list[i] = fork())) {
            char command[512];
            sprintf(command, "scp ./%s tstore%02d:/tmp/%s", mode, server_list[i], mode);
            system(command);
			exit(0);
        }
    }

    for(i=0;i<client_cnt;i++)
        waitpid(pid_list[i],&status,0);
}

void test_aggregate_performance(char *volname, char *mode, char *mountmode){
    int i,status;
    perf start,end;
	long long size = 1<<30;

    perf_start(&start);
	printf("--------Begin to test--------\n");
    for(i=0;i<client_cnt;i++){
        if(!(pid_list[i] = fork())) {
			char command[512];
            char servername[16];
            sprintf(servername,"tstore%02d", server_list[i]);
            sprintf(command, "ssh tstore%02d \'numactl --cpunodebind=0 --membind=0 /tmp/%s %d %d %s %s %s %s\' ", server_list[i], mode, i * TIMES, TIMES, servername, volname, "/mnt/debug", mountmode);
            system(command);
			exit(0);
        }
    }
    for(i=0;i<client_cnt;i++)
        waitpid(pid_list[i],&status,0);
    perf_stop(&end);
	
    //perf_print_bd(end,start,size * times * sizeof(server_list) / sizeof(int));
	printf("---------Finish test--------\n");
}

void performance_analysis(char *mode){

    int i,status;

    for(i=0;i<client_cnt;i++){
        if(!(pid_tmp[i] = fork())) {
            char command[512];
            sprintf(command, "scp tstore%02d:/tmp/%s.time ./times/%d.time", i, mode, i);
            system(command);
			exit(0);
        }
    }

    for(i=0;i<client_cnt;i++)
        waitpid(pid_tmp[i],&status,0);

	for(i=0;i<client_cnt;i++){
		char path[256];
		sprintf(path,"./times/%d.time",i);
		FILE * file = fopen(path,"r");
        if(file == NULL){
            perror(path);
            exit(errno);
        }
		fscanf(file,"%lld",&usecs[i]);
		fclose(file);
	}

	long long avg_time = 0;
	for(i=0;i<client_cnt;i++){
		avg_time += usecs[i];
	}
	avg_time /= client_cnt;

	long long size = 1<<30;
	size = size * TIMES * client_cnt;
	printf("Total Bandwidth: %lld MB in %.4f s = %.2f MB/s\n",size >> 20,(double)avg_time / 1e6,(double)size / (double)avg_time);
}


int main(int argc, char *argv[]) {
    if(argc != 5)  goto help_args;
    char *opmode = argv[3];
    char *mountmode = argv[4];
    if(strcmp(opmode, "read") != 0 && strcmp(opmode, "write") != 0) goto help_args;
    if(strcmp(mountmode, "nfs") != 0 && strcmp(mountmode, "glusterfs") != 0) goto help_args;
    
    client_cnt = atoi(argv[2]);
    dispatch_binary(opmode);
    test_aggregate_performance(argv[1], opmode, mountmode);
	performance_analysis(opmode);
    return 0;
help_args:
    printf("usage: ./aggregate <volname> <client_count(1-16)> <opmode(read, write)> <mountmode>(nfs, glusterfs)\n");
    return 0;
}
