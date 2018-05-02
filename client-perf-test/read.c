#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "perf.h"
#include "volume.h"


void warmup_memory(void *mem, int size) {
		memset(mem, 0, size);
}

char *alloc_buffer(int size) {
		void *data = NULL;
		int i;
		posix_memalign(&data, 64, size);
		warmup_memory(data, size);
		return data;
}

void read_file(const char *filePath, char *data, int len) {
    FILE *file = fopen(filePath, "r");
    if(file == NULL) {
        perror(filePath);
        return;
    }
    fgets(data, len, file);
    fclose(file);
}



int main(int argc, char *argv[]) {
        //printf("%s\n", argv[0]);

        char hostname[32];
        gethostname(hostname, 32);
        //printf("hello, %s begins...\n", hostname);
		long long size = 1 << 30;

		int i;
        if(argc != 7){
            printf("usage: ./read start times servername volname mountpoint mountmode\n");
            return 0;
        }
#ifdef DEBUG
        system("hostname");
        printf("get parameters:\n");
		for(i = 0; i < argc; i++){
            printf("%s ", argv[i]);
		}
        printf("\n");
#endif


		perf begin,end;

		int start = atoi(argv[1]);
		int times = atoi(argv[2]);

		char *data = alloc_buffer(size);

        char *servername = argv[3];
        char *volname = argv[4];
        char *mountpoint = argv[5];
        char *mountmode = argv[6];
        assert(strcmp(mountmode, "nfs") == 0 || strcmp(mountmode, "glusterfs") == 0);
        if(mount_volume(servername, volname, mountpoint, mountmode) != 0){
            goto out;
        }
        /*
        char cmd[128];
        sprintf(cmd, "df -TH  | grep %s", mode);
		system(cmd);
        */
		perf_start(&begin);
		for(i=start;i<start + times;i++) {
				char name[256];
				sprintf(name,"%s/test%d.dat",mountpoint, i);
				read_file(name, data, size);
                //printf("read file %s\n", name);
		}
		perf_stop(&end);

        char prefix[128];
        sprintf(prefix, "%s-%s", hostname, "read");
		perf_print_bd(prefix,end,begin,size * times);

		long long usec = get_usecs(end,begin);

		char path[64];
		sprintf(path,"/tmp/read.time");

		FILE * file = fopen(path,"w+");
        if(file == NULL){
            perror(path);
            goto out;
        }
		fprintf(file,"%lld\n",usec);
		fclose(file);
out:
        free(data);
        umount_volume(mountpoint);
		return 0;
}
