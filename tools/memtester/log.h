#ifndef LOG_H_
#define LOG_H_

#define socket_name "memtest"

#define TAG_NORMAL "[NORMAL]"
#define TAG_REFRESH "[REFRESH]"

#ifdef MEMTEST_DEBUG
#define MEMTEST_LOGD(x...) ALOGD( x )
#define MEMTEST_LOGE(x...) ALOGE( x )
#else
#define MEMTEST_LOGD(x...) do {} while(0)
#define MEMTEST_LOGE(x...) do {} while(0)
#endif

void ConnectSrv();
void DisconnectSrv();
int my_printf(const char *fmt, ...);
int my_fprint_f(int fd, const char *fmt, ...);

#endif
