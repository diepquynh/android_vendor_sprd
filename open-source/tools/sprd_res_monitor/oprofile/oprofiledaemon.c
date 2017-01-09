/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cutils/properties.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <cutils/log.h>
#include "oprofiledaemon.h"

static void do_oprofile(unsigned long profiletime)
{
   char profile[256];
   sprintf(profile , "/system/bin/capture_oprofile.sh %lu" , profiletime);
   system(profile);
}

int start_oprofile(unsigned long time)
{
    int client;
    profileinfo info;
    char property[PROPERTY_VALUE_MAX];
    struct sockaddr_un addr;
    int slen = sizeof(addr);
    property_get(OPROFILE_DEBUG_SWITCHER , property , "0");
    if(atoi(property) == 1)
    {
        client = socket(AF_UNIX , SOCK_DGRAM , 0);
        if(client == -1)
        {
            ALOGD("start oprofile faild creat socket error");
            return -1;
        }
        memset(&addr , 0 , sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path , OPROFILE_SOCKET_NAME);
        info.cmd = OPROFILE_START;
        info.profiletime = time;
        int ret = sendto(client , &info , sizeof(info) , 0 , (struct sockaddr*)&addr , slen);
        close(client);
        if(ret < 0)
        {
            ALOGD("start oprofile failed");
            return -1;
        }
        else
        {
            ALOGD("start oprofile");
            return 0;
        }
    }
    else
    {
        return -2;
    }
}

void* oprofile_daemon(void* param)
{
    int serv;
    profileinfo pinfo;
    int len;
    char property[PROPERTY_VALUE_MAX];
    struct sockaddr_un addr;
    memset(&addr , 0 , sizeof(addr));
    socklen_t addr_len = sizeof(addr);
    umask(0);
    int ret = mkdir(OPROFILE_SOCKET_PATH , S_IRWXU | S_IRWXG | S_IRWXO);
    if (-1 == ret && (errno != EEXIST)) {
        ALOGE("oprofile daemon create socket path failed");
        return NULL;
    }

    serv = socket(AF_UNIX , SOCK_DGRAM , 0);
    if(serv == -1)
    {
        ALOGE("socket create fail in oprofile daemon");
        return NULL;
    }
    //setsockopt();
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path , OPROFILE_SOCKET_NAME);
    unlink(addr.sun_path);
    if(bind(serv , (struct sockaddr*) &addr , sizeof(addr)) < 0)
    {
        close(serv);
        ALOGE("start oprofile daemon failed");
        return NULL;
    }
    for(;;) {
        len = recvfrom(serv , &pinfo , sizeof(pinfo) , 0 , (struct sockaddr*)&addr , &addr_len);
        ALOGD("----------------------receive from client---------------------");
        property_get(OPROFILE_DEBUG_SWITCHER , property , "0");
        // if property is not set continue
        if(atoi(property) != 1)
            continue;
        if(len < 0)
            continue;
        ALOGD("----------------------receive from client--------------------- cmd is:%d" , pinfo.cmd);
        switch(pinfo.cmd)
        {
        case OPROFILE_START:
             ALOGD("do_oprofile time is:%lu" , pinfo.profiletime);
             do_oprofile(pinfo.profiletime);
             break;
        default:
             ALOGW("oprofiledaemon cmd invalid");
             break; 
        }
    }
    return NULL;
}
