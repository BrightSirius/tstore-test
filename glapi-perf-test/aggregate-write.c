#include <stdio.h>
#include "glfs.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "test.h"

int server_list[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int pid_list[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int pid_tmp[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
long long usecs[sizeof(pid_list) / sizeof(int)];

#define TIMES 10

void dispatch_write_binary(){
    int i,status;

    for(i=0;i<sizeof(server_list)/sizeof(int);i++){
        if(!(pid_list[i] = fork())) {
            char command[512];
            sprintf(command, "scp ./write tstore%02d:/tmp/write", server_list[i]);
            system(command);
			exit(0);
        }
    }

    for(i=0;i<sizeof(server_list)/sizeof(int);i++)
        waitpid(pid_list[i],&status,0);
}

void test_aggregate_performance(char *volname){
    int i,status;
    perf start,end;
	long long size = 1<<30;

    perf_start(&start);
	printf("--------Begin to test--------\n");
    for(i=0;i<sizeof(server_list)/sizeof(int);i++){
        if(!(pid_list[i] = fork())) {
			char command[512];
            sprintf(command, "ssh tstore%02d \'numactl --cpunodebind=0 --membind=0 /tmp/write %d %d %s\' ", server_list[i], i * TIMES, TIMES, volname);
            system(command);
			exit(0);
        }
    }
    for(i=0;i<sizeof(server_list)/sizeof(int);i++)
        waitpid(pid_list[i],&status,0);
    perf_stop(&end);
	
    //perf_print_bd(end,start,size * times * sizeof(server_list) / sizeof(int));
	printf("---------Finish test--------\n");
}

void performance_analysis(){

    int i,status;

    for(i=0;i<sizeof(server_list)/sizeof(int);i++){
        if(!(pid_tmp[i] = fork())) {
            char command[512];
            sprintf(command, "scp tstore%02d:/tmp/write.time ./times/%d.time", i,i);
            system(command);
			exit(0);
        }
    }

    for(i=0;i<sizeof(server_list)/sizeof(int);i++)
        waitpid(pid_tmp[i],&status,0);

	for(i=0;i<sizeof(server_list)/sizeof(int);i++){
		char path[256];
		sprintf(path,"./times/%d.time",i);
		FILE * file = fopen(path,"r");
		fscanf(file,"%lld",&usecs[i]);
		fclose(file);
	}

	long long avg_time = 0;
	for(i=0;i<sizeof(server_list)/sizeof(int);i++){
		avg_time += usecs[i];
	}
	avg_time /= (sizeof(server_list)/sizeof(int));

	long long size = 1<<30;
	size = size * TIMES * sizeof(server_list)/sizeof(int);
	printf("Total Bandwidth: %lld MB in %.4f s = %.2f MB/s\n",size >> 20,(double)avg_time / 1e6,(double)size / (double)avg_time);
}


int main(int argc, char *argv[]) {

    assert(argc == 2);
    dispatch_write_binary();
	
    test_aggregate_performance(argv[1]);
	performance_analysis();

    return 0;
}
