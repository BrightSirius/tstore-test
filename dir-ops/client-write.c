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

#define MAXOPENFD 128
#define MAXFILESIZE 1ll*1024*1024*1024
#define Giga 1ll*1024*1024*1024

char data_buf[MAXFILESIZE];
char output_buf[1024];


void generate_data(){
    srand(time(NULL));
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

int recursive_create_fn(char *path, int cur_dep, int dir_depth, int dir_cnt, int file_size, char *signature){
    char* path_buf = (char *)malloc(256);
    strcpy(path_buf, path);
    int err = 0;
    for(int i = 0; i < dir_cnt; i++){
        int len = strlen(path);
        assert(len < 256);
        if(cur_dep < dir_depth){
            if(cur_dep != 1) sprintf(output_buf, "dep%d-i%d", cur_dep, i);
            else sprintf(output_buf, "%s-dep%d-i%d", signature, cur_dep, i);
            path_buf[len] = '/';
            strcpy(path_buf+len+1,output_buf);
            if((err = mkdir(path_buf, 0700)) == -1){
                printf("error when mkdir %s\n", path_buf);
                goto end;
            }
            recursive_create_fn(path_buf, cur_dep+1, dir_depth, dir_cnt, file_size, signature);
        }else if(cur_dep == dir_depth){
            if(cur_dep != 1) sprintf(output_buf, "dep%d-f%d", cur_dep, i);
            else sprintf(output_buf, "%s-dep%d-f%d", signature, cur_dep, i);
            path_buf[len] = '/';
            strcpy(path_buf+len+1,output_buf);

            FILE * file = fopen(path_buf, "w");
            if(file == NULL){
                printf("error when creat file %s, errno %s\n", path_buf, strerror(errno));
                goto end;
            }
            struct timeval t1, t2;
            gettimeofday(&t1, NULL);
            int ret = fputs(data_buf, file);
            if(ret == EOF){
                printf("error when write file %s, errno %s\n", path_buf, strerror(errno));
                goto end;
            }
            fclose(file);
            gettimeofday(&t2, NULL);
            printf("write file %s with bindwidth %.2fGB/s\n", path_buf, (1.0*MAXFILESIZE)/(Giga)/TIMEs(t1, t2));
            
            /*
            if((err = creat(path_buf, S_IRUSR | S_IWUSR) == -1)){
                printf("error when creat file %s, errno %s\n", path_buf, strerror(errno));
                goto end;
            }
            close(err);
            if((err = truncate(path_buf, file_size)) == -1){
                printf("error when truncate file %s\n", path_buf);
                goto end;
            }
            */
        }
    }
end:
    free(path_buf);
    return err;
}

int main(int argc, char* argv[]){
    int dir_cnt = 50;
    int dir_depth = 3;
    long long file_size = MAXFILESIZE;
    int ret;

    //char root_dir[]="/home/sirius/tstore_test_dir";
    char root_dir[]="/mnt/fuse";
    //printf("root_dir %s\n", root_dir);

    struct stat st;
    if(stat(root_dir, &st) == -1){
        sprintf(output_buf, "root_dir: %s stat error %s", root_dir, strerror(errno));
        //printf("%s\n", output_buf);
        EXIT(output_buf);
    }
    /*
    if(cleandir(root_dir) == -1){
        sprintf(output_buf, "rmdir %s error %s", root_dir, strerror(errno));
        EXIT(output_buf);
    }
    */

    generate_data();
    // mkdir, dir_depth layers, with dir_cnt subdirs of files in each dir
    if(recursive_create_fn(root_dir, 1, dir_depth, dir_cnt, file_size, "fuse") == -1){
        sprintf(output_buf, "recursive mkdir/files %s error %s", root_dir, strerror(errno));
        EXIT(output_buf);
    }
    return 0;
}
