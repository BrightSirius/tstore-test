#include <stdio.h>
#include "glfs.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "test.h"
#include <pthread.h>


static int seed = 2;

inline char fastrand() {
		seed = (21013 * seed + 2531011);
		return (seed >> 16) & 0x7f;
}

char *gen_data(int size) {
		void *data = NULL;
		int i;
		posix_memalign(&data, 64, size);
		for (i = 0; i < size; i++)
				*((char *) data + i) = fastrand();
		return data;
}

void warmup_memory(void *mem, int size) {
		memset(mem, 0, size);
}

glfs_t *mount_volume(const char *volname) {
		glfs_t *vol;
		perf start, end;
		printf("-------Init Volume----------------------------------\n");
		perf_start(&start);
		vol = glfs_new(volname);
		//glfs_set_volfile_server(vol, "rdma", "tstore00", 24007);
		glfs_set_volfile_server(vol, "tcp", "tstore00", 24007);
		glfs_set_logging(vol, "./write_log", 3);
		glfs_init(vol);
		perf_stop(&end);
		perf_print_sec(end, start);
		printf("-------Init Volume Completed------------------------\n");
		return vol;
}

void write_file(glfs_t *vol, const char *filePath, char *data, int len) {

		glfs_fd_t *fd;
		perf start, end;
		printf("-------Begin to write %s------------------------\n", filePath);
		fd = glfs_creat(vol, filePath, O_RDWR, 0644);
		assert (fd != NULL);
		perf_start(&start);
		glfs_write(fd, data, len, 0);
		glfs_close(fd);
		perf_stop(&end);
		perf_print_bd(end, start, len);
		printf("-------Finish write %s------------------------\n", filePath);

}

struct param {
		int begin;
		int end;
		char *data;
		glfs_t *vol;
		int size;
};

int pid[64];

void *write_thread(void *pa){
		struct param par = *(struct param*)pa;
		int begin = par.begin;
		int end = par.end;
		int i;
		for(i=begin;i<end;i++){
				char name[64];
				sprintf(name,"test%d.dat",i);
				write_file(par.vol, name, par.data, par.size);
		}
}

pthread_t threads[64];

int main(int argc, char *argv[]) {

		long long size = 1ll << 30;

		int i,c;

		assert(argc == 4);

		int start = atoi(argv[1]);
		int times = atoi(argv[2]);
		perf begin,end;

		char *data = gen_data(size);


		glfs_t *vol = mount_volume(argv[3]);
		perf_start(&begin);

		int client = 1;
		int status = 0;

		struct param pa[64];

		for(c=0;c<client;c++){

				int begin = start + (times/client) * c;
				int end = start + (times/client) * (c+1);
				pa[c].begin = begin;
				pa[c].end = end;
				pa[c].vol = vol;
				pa[c].data = data;
				pa[c].size = size;
				pthread_create(&threads[c],NULL,write_thread,&pa[c]);
		}

		for(c=0;c<client;c++){
				pthread_join(threads[c],NULL);
		}

		perf_stop(&end);
		perf_print_bd(end,begin,size * times);


		long long usec = get_usecs(end,begin);

		char path[64];
		sprintf(path,"/tmp/write.time");

		FILE * file = fopen(path,"w+");
		fprintf(file,"%lld\n",usec);
		fclose(file);

		return 0;
}
