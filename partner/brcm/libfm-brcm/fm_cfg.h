#ifndef _FM_CFG_H_
#define _FM_CFG_H_


#define IS_STANDALONE_FM TRUE

/* Use BTL-IF */
#ifndef BRCM_BT_USE_BTL_IF
#define BRCM_BT_USE_BTL_IF
#endif


//Define logging calls
#include <android/log.h>

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGV(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)

#endif
//_FM_CFG_H_
