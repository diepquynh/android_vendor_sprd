#ifndef EX_COMMON_H
#define EX_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <utils/Log.h>
#define LOG_TAG "TFA9890"
//#include <cutils/xlog.h>
#include <cutils/log.h>
#include "tfa9890_cust.h"

#if 0
#ifdef ALOGE
	#undef ALOGE
#endif
#ifdef ALOGW
	#undef ALOGW
#endif 
#ifdef ALOGI
	#undef ALOGI
#endif	
#ifdef ALOGD
	#undef ALOGD
#endif
#ifdef ALOGV
	#undef ALOGV
#endif

#define ALOGE XLOGE
#define ALOGW XLOGW
#define ALOGI XLOGI
#define ALOGD XLOGD
#define ALOGV XLOGV
#define printf ALOGI
#else
#define XLOGE ALOGE
#define XLOGW ALOGW
#define XLOGI ALOGI
#define XLOGD ALOGD
#define XLOGV ALOGV
#define printf ALOGI
#endif


static int tfa_debug = 1;
#define TFA_LOGD(fmt, arg...) do {\
    if(tfa_debug) \
        ALOGD("[%s: %d]"fmt, __func__, __LINE__, ##arg); \
}while(0)
#define TFA_LOGE(fmt, arg...) ALOGE("[%s: %d]"fmt, __func__, __LINE__, ##arg)

#define assert(condition) do {\
    if(!(condition)) \
        ALOGE("[%s: %d]", __func__, __LINE__); \
}while(0)

#ifndef WIN32
#define Sleep(ms) usleep((ms)*1000)
#define _fileno fileno
#define _GNU_SOURCE   /* to avoid link issues with sscanf on NxDI? */
#endif

//#define I2C_ADDRESS  0x6C
//#define SAMPLE_RATE 48000

#define TRACEIN  if(tfa98xxRun_trace) printf("Enter %s\n", __FUNCTION__);
#define TRACEOUT if(tfa98xxRun_trace) printf("Leave %s\n", __FUNCTION__);

//#define TFA_I2CSLAVEBASE		(0x36)              // tfa device slave address of 1st (=left) device

#define TFA98XX_DSP_STABLE_TIME           (25000)
#define TFA98XX_TRY_TIMES                 (300)
#define TFA98XX_MUTE_AMP_WAITING_TIME     (1000)
#define TFA98XX_COLD_STARTUP_WAITING_TIME (2000)
#define TFA98XX_CALIBRATION_WAITING_TIME  (25000)
#define TFA98XX_WS_FLAG                   (1)
#define TFA98XX_MUSIC_BYPASS_FLAG         (0)
#define TFA98XX_PHONE_BYPASS_FLAG         (0)

char* stateFlagsStr(int stateFlags);
void dump_state_info(Tfa98xx_StateInfo_t* pState);

void waitCalibration(Tfa98xx_handle_t handle, int *calibrateDone);

int tfaRun_CheckEvents(unsigned short regval);
Tfa98xx_Error_t tfaRun_PowerCycleCF(Tfa98xx_handle_t handle);
float tCoefFromSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes);
void tCoefToSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, float tCoef);

void setPatch(const char* fileName);

void setConfig(Tfa98xx_handle_t handle, const char* fileName);
void setConfigStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName);

void muteAmplifier(Tfa98xx_handle_t handle);
void setPreset(Tfa98xx_handle_t handle, const char* fileName);
void setPresetMono(Tfa98xx_handle_t handle, const char* fileName);
void setPresetStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName);

void setSpeaker(Tfa98xx_handle_t handle, const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes);
void setSpeakerStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes);
void saveSpeaker(Tfa98xx_SpeakerParameters_t speakerBytes, const char* fileName);

void loadSpeakerFile(const char* fileName, Tfa98xx_SpeakerParameters_t speakerBytes);
int dspSupporttCoef(Tfa98xx_handle_t handle);

void setEQ(Tfa98xx_handle_t handle, const char* fileName);
void setEQStereo(int handle_cnt, Tfa98xx_handle_t handles[], const char* fileName);

void resetMtpEx(Tfa98xx_handle_t handle);
void coldStartup(Tfa98xx_handle_t handle, const int rate, const char* fileName);
void dspPatch(Tfa98xx_handle_t handle, const char* fileName);

void statusCheck(Tfa98xx_handle_t handle);

void setOtc(Tfa98xx_handle_t handle, int otcOn);
int checkMTPEX(Tfa98xx_handle_t handle);




#ifdef __cplusplus
}
#endif

#endif                          // TFA98XX_H

