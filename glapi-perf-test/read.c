#include <stdio.h>
#include "glfs.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "test.h"


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

glfs_t *mount_volume(const char *volname) {
		glfs_t *vol;
		perf start, end;
		printf("-------Init Volume----------------------------------\n");
		perf_start(&start);
		vol = glfs_new(volname);
		glfs_set_volfile_server(vol, "tcp", "tstore00", 24007);
		glfs_set_logging(vol, "./write_log", 3);
		glfs_init(vol);
		perf_stop(&end);
		perf_print_sec(end, start);
		printf("-------Init Volume Completed------------------------\n");
		return vol;
}

void read_file(glfs_t *vol, const char *filePath, char *data, int len) {

		glfs_fd_t *fd;
		perf start, end;
		printf("-------Begin to read %s------------------------\n", filePath);
		fd = glfs_creat(vol, filePath, O_RDWR, 0644);
		assert(fd != NULL);
		perf_start(&start);
		glfs_read(fd, data, len, 0);
		glfs_close(fd);
		perf_stop(&end);
		perf_print_bd(end, start, len);
		printf("-------Finish read %s------------------------\n", filePath);

}



int main(int argc, char *argv[]) {

		long long size = 1 << 30;

		int i;

		assert(argc == 4);

		perf begin,end;

		int start = atoi(argv[1]);
		int times = atoi(argv[2]);

		char *data = alloc_buffer(size);

		glfs_t *vol = mount_volume(argv[3]);

		perf_start(&begin);
		for(i=start;i<start + times;i++) {
				char name[64];
				sprintf(name,"test%d.dat",i);
				read_file(vol, name, data, size);
		}
		perf_stop(&end);

		perf_print_bd(end,begin,size * times);

		glfs_fini(vol);


		long long usec = get_usecs(end,begin);

		char path[64];
		sprintf(path,"/tmp/read.time");

		FILE * file = fopen(path,"w+");
		fprintf(file,"%lld\n",usec);
		fclose(file);


		return 0;
}
