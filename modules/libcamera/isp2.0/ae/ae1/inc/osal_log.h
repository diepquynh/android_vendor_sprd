/*
 *
 */
#ifndef _OSAL_LOG
#define _OSAL_LOG

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
typedef enum _LOG_LEVEL {
    _LOG_TRACE,
    _LOG_INFO,
    _LOG_WARNING,
    _LOG_ERROR
} _LOG_LEVEL;

#define OMX_DEBUG_LEVEL 3 /* _LOG_INFO */
//#define ALOGV(...) OSAL_Log(_LOG_TRACE, __VA_ARGS__)
//#define ALOGI(...) OSAL_Log(_LOG_INFO, __VA_ARGS__)
//#define ALOGW(...) OSAL_Log(_LOG_WARNING, __VA_ARGS__)
//#define ALOGE(...) OSAL_Log(_LOG_ERROR, __VA_ARGS__)
void OSAL_Log(_LOG_LEVEL logLevel, const char *msg, ...);
#endif /* _OSAL_LOG */
