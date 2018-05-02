#ifndef _VOLUME
#define _VOLUME
#include <stdlib.h>
#include <stdio.h>


int mount_volume(char* servername, char *volname, char *path, char *mode){
    char cmd[256];
    sprintf(cmd, "mount -t %s %s:/%s %s", mode, servername, volname, path);
    int ret = system(cmd);
    if(ret != 0) {
        perror(cmd);
        return ret;
    }
}

int umount_volume(char *path){
    char cmd[256];
    sprintf(cmd, "umount %s", path);
    return system(cmd);
}
#endif
