#ifndef _BATTERY__H_
#define _BATTERY__H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <utils/Log.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "batterysrv"
#ifndef LOGD
#define LOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif
#ifndef LOGE
#define LOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

int read_usb(void);
int read_ac(void);
void get_nv(void);
void write_nv(void);
int uevent_init(void);
int uevent_next_event(char *buffer, int buffer_length);

#ifdef __cplusplus
}
#endif
#endif
