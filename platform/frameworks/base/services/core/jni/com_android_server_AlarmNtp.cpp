/* //device/libs/android_runtime/android_server_AlarmManagerService.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <utils/Log.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <linux/android_alarm.h>
#include <linux/rtc.h>
#include <cutils/sockets.h>
#include <netdb.h>
#include <cutils/properties.h>
#include <iostream>
#include <sstream>

using namespace std;

#ifdef __LP64__
typedef time_t time64_t;
#else
#include <time64.h>
#endif

#ifndef ANDROID_NTPSOCKET_DIR
#define ANDROID_NTPSOCKET_DIR      "/data/tmp/socket"
#endif

#define NTP_SERVER0                "0.asia.pool.ntp.org"
#define NTP_SERVER1                "1.asia.pool.ntp.org"
#define NTP_SERVER2                "2.asia.pool.ntp.org"
#define NTP_SERVER3                "3.asia.pool.ntp.org"
#define NTP_SERVER4                "0.cn.pool.ntp.org"
#define NTP_SERVER5                "0.hk.pool.ntp.org"
#define NTP_SERVER6                "3.tw.pool.ntp.org"
#define NTP_SERVER7                "0.jp.pool.ntp.org"
#define NTP_SERVER8                "1.jp.pool.ntp.org"
#define NTP_SERVER9                "2.jp.pool.ntp.org"
#define NTP_SERVER10               "3.jp.pool.ntp.org"
#define NTP_SERVER11               "0.kr.pool.ntp.org"
#define NTP_SERVER12               "0.us.pool.ntp.org"
#define NTP_SERVER13               "1.us.pool.ntp.org"
#define NTP_SERVER14               "2.us.pool.ntp.org"
#define NTP_SERVER15               "3.us.pool.ntp.org"

#define NTP_PORT                    123
#define JAN_1970                    0x83aa7e80
#define NTPFRAC(x) (4294 * (x) + ((1981 * (x))>>11))
#define NTP_CONNECT_MAX_TIME        30
#define NTP_RECV_TIMEOUT            10
#define random(x) (rand()%x+1)
#define DELTA_TIME                  "persist.delta.time"

namespace android {

pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ntp_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
time64_t _delta = 0;
time64_t delta_alarm = 0;
int firstConnectNetwork = 0;
int ntp_no_thread_exist = 1;

typedef struct NtpTime {
    unsigned int coarse;
    unsigned int fine;
} NTPTIME;

typedef struct ntpheader {
    union {
        struct {
            char local_precision;
            char Poll;
            unsigned char stratum;
            unsigned char Mode :3;
            unsigned char VN :3;
            unsigned char LI :2;
        };
        unsigned int headData;
    };
} NTPHEADER;

typedef struct NtpPacked {
    NTPHEADER header;

    unsigned int root_delay;
    unsigned int root_dispersion;
    unsigned int refid;
    NTPTIME reftime;
    NTPTIME orgtime;
    NTPTIME recvtime;
    NTPTIME trantime;
} NTPPACKED, *PNTPPACKED;

int ntp_property_set(time64_t delta_time) {
    char delta_prop[PROPERTY_VALUE_MAX];
    stringstream stream;
    string result;
    stream << delta_time;
    stream >> result;
    strcpy(delta_prop, result.c_str());
    return property_set(DELTA_TIME, delta_prop);
}

time64_t ntp_property_get() {
    char delta_prop[PROPERTY_VALUE_MAX];
    property_get(DELTA_TIME, delta_prop, "1LL<<63");
    if(0 == (strcmp(delta_prop, "1LL<<63"))) {
        return 1LL<<63;
    } else {
        time64_t delta_time;
        istringstream is(delta_prop);
        is >> delta_time;
    return delta_time;
    }
}

const char* server_list[] = {NTP_SERVER0, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3,
                       NTP_SERVER4, NTP_SERVER5, NTP_SERVER6, NTP_SERVER7,
                       NTP_SERVER8, NTP_SERVER9, NTP_SERVER10, NTP_SERVER11,
                       NTP_SERVER12, NTP_SERVER13, NTP_SERVER14, NTP_SERVER15};

int createNTPClientSockfd() {
    int sockfd;
    int addr_len;
    struct sockaddr_in addr_src;

    addr_len = sizeof(struct sockaddr_in);
    memset(&addr_src, 0, addr_len);
    addr_src.sin_family = AF_INET;
    addr_src.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_src.sin_port = htons(0);
    /* create socket. */
    if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
        ALOGE("drm_ntp: create socket error! %s", strerror(errno));
        return -1;
    }
    ALOGD("drm_ntp: CreateNtpClientSockfd sockfd=%d\n", sockfd);
    return sockfd;
}

int connectNTPServer(int sockfd, char * serverAddr, int serverPort, struct sockaddr_in * ServerSocket_in) {

    struct addrinfo hints;
    struct addrinfo* result = 0;
    struct addrinfo* iter = 0;
    int ret;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;
    ret = getaddrinfo((const char*)serverAddr, 0, &hints, &result);
    if (ret != 0) {
        ALOGE("drm_ntp: get hostname %s address info failed! %s", serverAddr, gai_strerror(ret));
        return -1;
    }

    char host[1025] = "";
    for (iter = result; iter != 0; iter = iter->ai_next) {
        ret = getnameinfo(result->ai_addr, result->ai_addrlen, host, sizeof(host), 0, 0, NI_NUMERICHOST);
        if (ret != 0) {
            ALOGE("drm_ntp: get hostname %s name info failed! %s", serverAddr, gai_strerror(ret));
            continue;
        } else {
            ALOGD("drm_ntp: hostname %s -> ip %s", serverAddr, host);
            struct sockaddr_in addr_dst;
            int addr_len;
            addr_len = sizeof(struct sockaddr_in);
            memset(&addr_dst, 0, addr_len);
            addr_dst.sin_family = AF_INET;
            addr_dst.sin_addr.s_addr = inet_addr(host);
            addr_dst.sin_port = htons(serverPort);
            memcpy(ServerSocket_in, &addr_dst, sizeof(struct sockaddr_in));

            /* connect to ntp server. */
            ALOGE("drm_ntp: try to connect ntp server %s", serverAddr);
            ret = connect(sockfd, (struct sockaddr*) &addr_dst, addr_len);
            if (-1 == ret) {
                ALOGE("drm_ntp: connect to ntp server %s failed, %s", serverAddr, strerror(errno));
                continue;
            } else {
                ALOGD("drm_ntp: ConnectNtpServer sucessful!\n");
                break;
            }
        }
    }

    if (result) freeaddrinfo(result);
        return sockfd;
}

void sendQueryTimePacked(int sockfd) {
    NTPPACKED SynNtpPacked;
    time_t timer;
    memset(&SynNtpPacked, 0, sizeof(SynNtpPacked));

    SynNtpPacked.header.local_precision = -6;
    SynNtpPacked.header.Poll = 4;
    SynNtpPacked.header.stratum = 0;
    SynNtpPacked.header.Mode = 3;
    SynNtpPacked.header.VN = 3;
    SynNtpPacked.header.LI = 0;

    SynNtpPacked.root_delay = 1 << 16; /* Root Delay (seconds) */
    SynNtpPacked.root_dispersion = 1 << 16; /* Root Dispersion (seconds) */

    SynNtpPacked.header.headData = htonl((SynNtpPacked.header.LI << 30) | (SynNtpPacked.header.VN << 27) |
                                         (SynNtpPacked.header.Mode << 24)| (SynNtpPacked.header.stratum << 16) |
                                         (SynNtpPacked.header.Poll << 8) | (SynNtpPacked.header.local_precision & 0xff));
    SynNtpPacked.root_delay = htonl(SynNtpPacked.root_dispersion);
    SynNtpPacked.root_dispersion = htonl(SynNtpPacked.root_dispersion);

    time(&timer);
    SynNtpPacked.trantime.coarse = htonl(JAN_1970 + (long)timer);
    SynNtpPacked.trantime.fine = htonl((long)NTPFRAC(timer));

    send(sockfd, &SynNtpPacked, sizeof(SynNtpPacked), 0);
}

int recvNTPPacked(int sockfd, PNTPPACKED pSynNtpPacked, struct sockaddr_in * ServerSocket_in) {
    int receivebytes = -1;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    fd_set sockset;

    FD_ZERO(&sockset);
    FD_SET(sockfd, &sockset);
    struct timeval blocktime = {NTP_RECV_TIMEOUT, 0};

    /* recv ntp server's response. */
    if (select(sockfd+1, &sockset, 0, 0, &blocktime) > 0) {
        receivebytes = recvfrom(sockfd, pSynNtpPacked, sizeof(NTPPACKED), 0,(struct sockaddr *) ServerSocket_in, &addr_len);
        if (-1 == receivebytes) {
            ALOGE("drm_ntp: recvfrom error! %s", strerror(errno));
            return -1;
        } else {
            ALOGD("drm_ntp: recvfrom receivebytes=%d",receivebytes);
        }
    } else {
        ALOGE("drm_ntp: recvfrom timeout! %s", strerror(errno));
    }
    return receivebytes;
}

static void startPollingNTPTime() {
    int i = 0;
    int list_num = sizeof(server_list)/sizeof(server_list[0]);
    int ret = -1;
    int sockfd = -1;

    pthread_mutex_lock(&ntp_thread_mutex);
    ntp_no_thread_exist = 0;
    pthread_mutex_unlock(&ntp_thread_mutex);

    do {
        sockfd = createNTPClientSockfd();
        if (sockfd == -1) {
            ALOGE("drm_ntp: create socket failed");
            continue;
        }
        struct sockaddr_in ServerSocketn;
        ret= connectNTPServer(sockfd, (char *)server_list[i++%list_num], NTP_PORT, &ServerSocketn);
        if (ret == -1) {
            if(i == NTP_CONNECT_MAX_TIME) {
                pthread_mutex_lock(&ntp_thread_mutex);
                ntp_no_thread_exist = 1;
                pthread_mutex_unlock(&ntp_thread_mutex);
            }
            close(sockfd);
            srand(i);
            int y = random(6);
            sleep(y);
             ALOGE("drm_ntp: sleep %ds and reconnect...", y);
            continue;
        }

        /* send ntp protocol packet. */
        sendQueryTimePacked(sockfd);

        NTPPACKED syn_ntp_packed;
        ret = recvNTPPacked(sockfd,&syn_ntp_packed,&ServerSocketn);
        if (ret == -1) {
            if(i == NTP_CONNECT_MAX_TIME) {
                pthread_mutex_lock(&ntp_thread_mutex);
                ntp_no_thread_exist = 1;
                pthread_mutex_unlock(&ntp_thread_mutex);
            }
            ALOGE("drm_ntp: recv from ntp server failed");
                close(sockfd);
                continue;
        }

        NTPTIME trantime;
        time64_t systemTime;
        trantime.coarse = ntohl(syn_ntp_packed.trantime.coarse) - JAN_1970;
        pthread_mutex_lock(&_mutex);
        systemTime = time(NULL);
        _delta = trantime.coarse - systemTime;
        firstConnectNetwork = 1;
        if( 0 != ntp_property_set(_delta)) {
            pthread_mutex_lock(&ntp_thread_mutex);
            ntp_no_thread_exist = 1;
            pthread_mutex_unlock(&ntp_thread_mutex);
        }
        pthread_mutex_unlock(&_mutex);
        ALOGD("drm_ntp:  trantime.coarse:%d, time(NULL):%lld", trantime.coarse, (long long)systemTime);

        close(sockfd);
        ALOGD("drm_ntp: quit polling NTP time!");
        break;
    } while (i < NTP_CONNECT_MAX_TIME);
    if(i >= NTP_CONNECT_MAX_TIME)
        ALOGE("drm_ntp: query ntp failed!");
}

void android_server_AlarmManagerService_pollingNTPTime() {
    time64_t delta;
    pthread_mutex_lock(&_mutex);
    delta = ntp_property_get();
    pthread_mutex_unlock(&_mutex);
    ALOGD("drm_ntp, pollingNTPTime, delta: %lld.", (long long)delta);
    if (delta == 1LL<<63) {  // not synced with ntp yet
        if(ntp_no_thread_exist) {
            pthread_t tid;
            pthread_attr_t attr;
            if(0!=pthread_attr_init(&attr))
                ALOGE("drm_ntp, pthread_attr_init error.");
            if(0!=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
                ALOGE("drm_ntp, =pthread_attr_setdetachstate error.");
            if(pthread_create(&tid,&attr,(void*(*)(void*))(&startPollingNTPTime),NULL))
                ALOGE("drm_ntp, pthread_create (%s)\n", strerror(errno));
            pthread_attr_destroy(&attr);
        } else {
            ALOGE("drm_ntp, thread has already created.");
        }
    } else {
        firstConnectNetwork = 1;
    }
}

} /* namespace android */
