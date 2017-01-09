/* * SocketInfo.cpp
 *
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>

#include <SocketInfo.h>

namespace android {

SocketInfo::SocketInfo() {}

SocketInfo::~SocketInfo() {
}

static const int FILE_NAME_LEN = 1024;
static const int SOCKET_NAME_LEN = 108;
void SocketInfo::setSocketName(int socket0, int socket1)
{
    struct sockaddr_un sockAddr;
    char sockName[SOCKET_NAME_LEN];
    FILE *fp= NULL;
    char fileName[sizeof("/proc/%u/comm") + sizeof(int)*3];
    char pidName[FILE_NAME_LEN] = {'\0'};
    char tidName[FILE_NAME_LEN] = {'\0'};
    int pid = getpid();
    int tid = gettid();

    snprintf(fileName, sizeof(fileName), "/proc/%d/comm", pid);
    if((fp = fopen(fileName, "r")) != NULL) {
        if (fgets(pidName, FILE_NAME_LEN, fp) != NULL)
            pidName[strlen(pidName) - 1] = '\0';
        fclose(fp);
    }

    snprintf(fileName, sizeof(fileName),"/proc/%d/comm", tid);
    if((fp = fopen(fileName, "r")) != NULL) {
        if (fgets(tidName, FILE_NAME_LEN, fp) != NULL)
            tidName[strlen(tidName) - 1] = '\0';
        fclose(fp);
    }

    if (pidName[0] == '\0')
        snprintf(pidName, sizeof(pidName),"t%d", pid);
    if (tidName[0] == '\0')
        snprintf(tidName, sizeof(tidName),"t%d", tid);

    if (socket0 >= 0) {
        snprintf(sockName, SOCKET_NAME_LEN - 1, "%s-%s-f%d", pidName, tidName, socket0);
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sun_family = AF_UNIX;
        strncpy(sockAddr.sun_path + 1, sockName, sizeof(sockAddr.sun_path) - 2);
        bind(socket0, reinterpret_cast<struct sockaddr *> (&sockAddr), sizeof(struct sockaddr_un));
    }

    if (socket1 >= 0) {
        snprintf(sockName, SOCKET_NAME_LEN - 1, "%s-%s-f%d", pidName, tidName, socket1);
        memset(&sockAddr, 0, sizeof(sockAddr));
        sockAddr.sun_family = AF_UNIX;
        strncpy(sockAddr.sun_path + 1, sockName, sizeof(sockAddr.sun_path) - 2);
        bind(socket1, reinterpret_cast<struct sockaddr *> (&sockAddr), sizeof(struct sockaddr_un));
    }
}

};  // namespace android
