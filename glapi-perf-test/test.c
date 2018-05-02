#include <stdio.h>
#include "glfs.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include "test.h"


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

void write_file(glfs_t *vol, const char *filePath, char *data, int len) {

    glfs_fd_t *fd;
    perf start, end;
    printf("-------Begin to write %s------------------------\n", filePath);
    fd = glfs_creat(vol, filePath, O_RDWR, 0644);
    perf_start(&start);
    glfs_write(fd, data, len, 0);
    glfs_close(fd);
    perf_stop(&end);
    perf_print_bd(end, start, len);
    printf("-------Finish write %s------------------------\n", filePath);

}

void read_file(glfs_t *vol, const char *filePath, char *buffer, int len) {
    glfs_fd_t *fd;
    perf start, end;
    printf("-------Begin to read %s------------------------\n", filePath);
    fd = glfs_creat(vol, filePath, O_RDWR, 0644);
    perf_start(&start);
    glfs_read(fd, buffer, len, 0644);
    glfs_close(fd);
    perf_stop(&end);
    perf_print_bd(end, start, len);
    printf("-------Finish read %s------------------------\n", filePath);
}


void kill_glusterfsd(int serverid, int brickid) {
    char command[512];
    /*
 *     printf("---------killing tstore%02d:/disk%d---------------------\n",serverid,brickid);
 *     */
    sprintf(command,
            "ssh root@tstore%02d \" ps -aux | grep disk%d/ | awk {\'print \\$2\'} | xargs kill -9 1>/dev/null 2>&1\" ",
            serverid, brickid);

    system(command);
/*
 *     printf("%s\n",command);
 *
 *         */
    printf("---------tstore%02d:/disk%d killed--------------\n", serverid, brickid);
}


void write_file_with_process_killing(glfs_t *vol, const char *filePath, char *data, int len) {
    int i, j;
    int pid,status;
    glfs_fd_t *fd;
    fd = glfs_creat(vol, filePath, O_RDWR, 0644);
    if (!(pid = fork())) {
        int total_killing = 16;
        int status;
        int server_list[] = {0, 1, 2, 3};
        int brick_list[] = {0, 1, 2, 3};
        int pids[] = {0, 1, 2, 3};
        for (i = 0; i < sizeof(server_list) / sizeof(int); i++)
            for (j = 0; j < sizeof(brick_list) / sizeof(int); j++) {
                if (!(pids[i] = fork())) {
                    kill_glusterfsd(server_list[i], brick_list[j]);
                    exit(0);
                }
                total_killing--;
            }
        for (i = 0; i < sizeof(server_list) / sizeof(int); i++)
            waitpid(pids[i], &status, 0);
        exit(0);

    } else {

        perf start, end;
        printf("-------Begin to write %s with killing------------------------\n", filePath);

        perf_start(&start);
        glfs_write(fd, data, len, 0);
        glfs_close(fd);
        perf_stop(&end);
        perf_print_bd(end, start, len);
        waitpid(pid,&status,0);
        printf("-------Finish write %s with killing------------------------\n", filePath);
    }
}

void read_file_with_process_killing(glfs_t *vol, const char *filePath, char *buffer, int len) {
    int i, j;
    int pid,status;
    glfs_fd_t *fd;
    fd = glfs_creat(vol, filePath, O_RDWR, 0644);
    if (!(pid = fork())) {
        int total_killing = 16;
        int status;
        int server_list[] = {0, 1, 2, 3};
        int brick_list[] = {0, 1, 2, 3};
        int pids[] = {0, 1, 2, 3};
        for (i = 0; i < sizeof(server_list) / sizeof(int); i++)
            for (j = 0; j < sizeof(brick_list) / sizeof(int); j++) {
                if (!(pids[i] = fork())) {
                    kill_glusterfsd(server_list[i], brick_list[j]);
                    exit(0);
                }
                total_killing--;
            }

        for (i = 0; i < sizeof(server_list) / sizeof(int); i++)
            waitpid(pids[i], &status, 0);
        exit(0);

    } else {
        perf start, end;
        sleep(1);
        printf("-------Begin to read %s with killing------------------------\n", filePath);
        perf_start(&start);
        glfs_read(fd, buffer, len, 0);
        glfs_close(fd);
        perf_stop(&end);
        perf_print_bd(end, start, len);
        waitpid(pid,&status,0);
        printf("-------Finish read %s with killing------------------------\n", filePath);
    }
}

void restart_glusterd() {
    int server_list[] = {0, 1, 2, 3};
    int pids[] = {0, 1, 2, 3};
    int i, status;
    for (i = 0; i < sizeof(server_list) / sizeof(int); i++) {
        if (!(pids[i] = fork())) {
            char command[512];
            sprintf(command, "ssh tstore%02d \' service glusterd restart 1>/dev/null 2>&1\' ", server_list[i]);
            system(command);
            printf("-----------glusterd on tstore%02d restarted------------------------\n", server_list[i]);
            exit(0);
        }
    }

    for (i = 0; i < sizeof(server_list) / sizeof(int); i++)
        waitpid(pids[i], &status, 0);
}

int main(int argc, char *argv[]) {

    int size = 1 << 30;

    assert(argc == 2);

    char *data = gen_data(size);
    char *buffer = alloc_buffer(size);

    glfs_t *vol = mount_volume(argv[1]);

    write_file(vol, "test.dat", data, size);

    read_file(vol, "test.dat", buffer, size);

    printf("-------Check Correcness----%s------------\n", !memcmp(data, buffer, size) ? "OK!" : "Error!");


    write_file_with_process_killing(vol, "test-write-killing.dat", data, size);

    read_file(vol, "test-write-killing.dat", buffer, size);

    printf("-------Check Correcness----%s------------\n", !memcmp(data, buffer, size) ? "OK!" : "Error!");


    printf("-----------------Recovering killed disks-----------------------------------\n");


    restart_glusterd();


    printf("-----------------Disk killed recovered--------------------------------------\n");

    write_file(vol, "test-read-killing.dat", data, size);

    read_file_with_process_killing(vol, "test-read-killing.dat", buffer, size);

    printf("-------Check Correcness----%s------------\n", !memcmp(data, buffer, size) ? "OK!" : "Error!");

    printf("-----------------Cleaning-----------------------------------\n");

    glfs_fini(vol);

    restart_glusterd();


    return 0;
}
