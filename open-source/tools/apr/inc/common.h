

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
#include <log/logger.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>

#ifndef LOG_TAG
#define LOG_TAG "collect_apr"
#endif

#include <utils/Log.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#define APR_DEBUG

#ifdef APR_DEBUG
#define APR_LOGD(x...) ALOGD(x)
#define APR_LOGE(x...) ALOGE(x)
#else
#define APR_LOGD(x...) do{}while(0)
#define APR_LOGE(x...) do{}while(0)
#endif

void self_inspection_apr_enabled();
int64_t getdate(char* strbuf, size_t max, const char* format);
int64_t uptime(char* strbuf);

#ifdef __cplusplus
}
#endif

#endif

