#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "perf.h"
#include "volume.h"

static int seed = 2;

inline char fastrand() {
		seed = (21013 * seed + 2531011);
		return (seed >> 16) % 26 + 'a';
}

char *gen_data(int size) {
		void *data = NULL;
		int i;
		posix_memalign(&data, 64, size);
		for (i = 0; i < size; i++)
				*((char *) data + i) = fastrand();
        *((char *)data + size - 1) = '\0';
		return data;
}



void write_file(const char *filePath, char *data, int len) {
    FILE *file = fopen(filePath, "w");
    if(file == NULL) {
        perror(filePath);
        return;
    }
    fputs(data, file);
    fclose(file);
}



int main(int argc, char *argv[]) {
        char hostname[32];
        gethostname(hostname, 32);
        //printf("hello, %s begins...\n", hostname);
        //return 0;

		long long size = 1 << 30;

		int i;
        if(argc != 7){
            printf("usage: ./read start times servername volname mountpoint mountmode\n");
            return 0;
        }
        
#ifdef DEBUG
        printf("get parameters:\n");
		for(i = 0; i < argc; i++){
            printf("%s ", argv[i]);
		}
        printf("\n");
#endif

		perf begin,end;

		int start = atoi(argv[1]);
		int times = atoi(argv[2]);

		char *data = gen_data(size);

        char *servername = argv[3];
        char *volname = argv[4];
        char *mountpoint = argv[5];
        char *mountmode = argv[6];
        if(mount_volume(servername, volname, mountpoint, mountmode) != 0){
            goto out;
        }
//system("df -TH | grep glusterfs");

		perf_start(&begin);
		for(i=start;i<start + times;i++) {
				char name[256];
				sprintf(name,"%s/test%d.dat",mountpoint, i);
				write_file(name, data, size);
                //printf("write file %s\n", name);
		}
		perf_stop(&end);
        char prefix[128];
        sprintf(prefix, "%s-%s", hostname, "write");
		perf_print_bd(prefix, end,begin,size * times);

		long long usec = get_usecs(end,begin);

		char path[64];
		sprintf(path,"/tmp/write.time");

		FILE * file = fopen(path,"w+");
        if(file == NULL){
            perror(path);
            goto out;
        }
		fprintf(file,"%lld\n",usec);
		fclose(file);
        umount_volume(mountpoint);
out:
        free(data);
		return 0;
}
