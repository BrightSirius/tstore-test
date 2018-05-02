#include <stdio.h>
#include <ftw.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

#define CLIENT_ID "DIRTEST"

#define MAXOPENFD 128
#define MAXFILESIZE 4096
#define Giga 1ll*1024*1024*1024

char data_buf[MAXFILESIZE];
char output_buf[1024];


void generate_data(){
    for(int i = 0; i < MAXFILESIZE-1; i++) 
        data_buf[i] = (rand() % 26) + 'a';
    data_buf[MAXFILESIZE-1] = '\0';
}


int remove_fn(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw){
    //printf("ftw depth %d\n", ftw->level);
    if(ftw->level) return remove(fpath);
    return 0;
}

int cleandir(char *dir){
   return nftw(dir, remove_fn, 128, FTW_DEPTH | FTW_PHYS);
}

int recursive_create_fn(char *path, int cur_dep, int dir_depth){
    char* path_buf = (char *)malloc(256);
    strcpy(path_buf, path);
    int dir_cnt = 5 + rand() % 95; // each dir has 5~99 files/subdirs
    int err = 0;
    for(int i = 0; i < dir_cnt; i++){
        int len = strlen(path);
        assert(len < 256);
        if(cur_dep < dir_depth){
            if(cur_dep != 1) sprintf(output_buf, "dep%d-i%d", cur_dep, i);
            else sprintf(output_buf, "%s-dep%d-i%d", CLIENT_ID, cur_dep, i);
            path_buf[len] = '/';
            strcpy(path_buf+len+1,output_buf);
            if((err = mkdir(path_buf, 0700)) == -1){
                printf("error when mkdir %s\n", path_buf);
                goto end;
            }
            recursive_create_fn(path_buf, cur_dep+1, dir_depth);
        }else if(cur_dep == dir_depth){
            if(cur_dep != 1) sprintf(output_buf, "dep%d-f%d", cur_dep, i);
            else sprintf(output_buf, "%s-dep%d-f%d", CLIENT_ID, cur_dep, i);
            path_buf[len] = '/';
            strcpy(path_buf+len+1,output_buf);

            FILE * file = fopen(path_buf, "w");
            if(file == NULL){
                printf("error when creat file %s, errno %s\n", path_buf, strerror(errno));
                goto end;
            }
            generate_data();
            int ret = fputs(data_buf, file);
            if(ret == EOF){
                printf("error when write file %s, errno %s\n", path_buf, strerror(errno));
                goto end;
            }
            fclose(file);
        }
    }
end:
    free(path_buf);
    return err;
}

int main(int argc, char* argv[]){
    srand(time(NULL));
    long long file_size = MAXFILESIZE;
    int ret;

    char root_dir[]="/mnt/fuse/dir-test";
    //printf("root_dir %s\n", root_dir);

    struct stat st;
    if(stat(root_dir, &st) == -1){
        sprintf(output_buf, "root_dir: %s stat error %s", root_dir, strerror(errno));
        //printf("%s\n", output_buf);
        EXIT(output_buf);
    }
    printf("first to clean dir %s yes/no?\n", root_dir);
    char c = getchar();
    if(c != 'y') return 0;
    if(cleandir(root_dir) == -1){
        sprintf(output_buf, "rmdir %s error %s", root_dir, strerror(errno));
        EXIT(output_buf);
    }
    int iter = 0;
    while(true){
        int dir_depth = 1 + rand() % 5;
        iter ++;
        printf("directory %s test: %d-th iteration begin\n", root_dir, iter);
        // mkdir, dir_depth layers, with dir_cnt subdirs of files in each dir
        if(recursive_create_fn(root_dir, 1, dir_depth) == -1){
            sprintf(output_buf, "recursive mkdir/files %s error %s", root_dir, strerror(errno));
            EXIT(output_buf);
        }
        sleep(3);
        printf("recursive create ok\n");
        if(cleandir(root_dir) == -1){
            sprintf(output_buf, "rmdir %s error %s", root_dir, strerror(errno));
            EXIT(output_buf);
        }
        printf("recursive delete ok\n");
        printf("directory %s test: %d-th iteration ok\n", root_dir, iter);
    }
    return 0;
}
