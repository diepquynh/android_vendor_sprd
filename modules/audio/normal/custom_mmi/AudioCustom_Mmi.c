/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AudioCustom_MMI"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <system/audio.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>


#include "AudioCustom_MmiApi.h"

/**
string for MMI TEST KEY
**/

#define MMI_AUDIO_LOOP "audioloop"
#define MMI_TEST_ON "on"
#define MMI_TEST_OFF "off"
#define MMI_ID_KEY "mmi_test"
#define MMI_OUT_DEVICE_KEY "output_device"
#define MMI_IN_DEVICE_KEY "input_device"
#define MMI_SPEAKER "speaker"
#define MMI_RECEIVER "receiver"
#define MMI_HEADPHONE "headphone"
#define MMI_MAIN_MIC "mainmic"
#define MMI_SUBMIC "submic"


/**
udwOutDevice: outdevice
udwInDevice: inDevice
flag: 1: MMI test on  0: MMI test off
**/
typedef struct {
  int dwOutDevice;
  int dwInDevice;
}AUDIO_CUSTOM_DEV_INFO;

typedef struct {
    AUDIO_CUSTOM_DEV_INFO tDevInfo;
    int flag;
}AUDIO_MMI_MGR_INFO;

/**
init for audio mmi test
**/

AUDIO_MMI_HANDLE AudioMMIInit(void) {
  AUDIO_MMI_HANDLE pHanle = NULL;

  pHanle = (AUDIO_MMI_HANDLE)malloc(sizeof(AUDIO_MMI_MGR_INFO));
  if (NULL == pHanle) {
    ALOGE("%s is fail !", __FUNCTION__);
    return NULL;
  }

  memset(pHanle, 0, sizeof(AUDIO_MMI_MGR_INFO));
  return pHanle;
}

/**
get device for audio mmi test
**/

int AudioMMIGetDevice(AUDIO_MMI_HANDLE handle,
                                   int *pOutDevice,
                                   int *pInDevice) {
  AUDIO_MMI_MGR_INFO *pMgrInfo = NULL;
  if ((NULL == handle) || (NULL == pOutDevice) || (NULL == pInDevice)) {
      ALOGE("%s 0x%x, 0x%x, 0x%x Pointer is NULL", __FUNCTION__, handle, pOutDevice, pInDevice);
      return MMI_TEST_ARG_ERR;
  }

  pMgrInfo = (AUDIO_MMI_MGR_INFO *)handle;
  if (MMI_TEST_CLOSE == pMgrInfo->flag) {
    ALOGV("%s MMI test is off", __FUNCTION__);
    return MMI_TEST_IS_OFF;
  }

  *pOutDevice = pMgrInfo->tDevInfo.dwOutDevice;
  *pInDevice = pMgrInfo->tDevInfo.dwInDevice;
  return MMI_TEST_NO_ERR;
}

/**
Parse device for audio mmi test
**/

int AudioMMIParse(struct str_parms *str_parms, AUDIO_MMI_HANDLE handle) {
  char strVal[32] = {0};
  int dwRc = MMI_TEST_NO_ERR;
  AUDIO_MMI_MGR_INFO *pMgrInfo = NULL;

  if ((NULL == handle) || (NULL == str_parms)) {
      ALOGE("%s 0x%x, 0x%x, Pointer is NULL", __FUNCTION__, str_parms, handle);
      return MMI_TEST_ARG_ERR;
  }

  pMgrInfo = (AUDIO_MMI_MGR_INFO *)handle;
  /*Mmi_test=off/on*/
  dwRc = str_parms_get_str(str_parms, MMI_ID_KEY, strVal, sizeof(strVal));
  if (0 <= dwRc) {
    if(0 == strcmp(strVal, MMI_TEST_ON)) {
        pMgrInfo->flag = MMI_TEST_START;
    } else if (0 == strcmp(strVal, MMI_TEST_OFF)){
      pMgrInfo->flag = MMI_TEST_CLOSE;
    } else {
      ALOGE("%s MMI_ID_KEY value is error! strVal = %s", __FUNCTION__, strVal);
      return MMI_TEST_ARG_ERR;
    }
  }

  if (MMI_TEST_START != pMgrInfo->flag) {
    ALOGI("%s MMI test is close!", __FUNCTION__);
    return MMI_TEST_IS_OFF;
  }
  /*MMI Test outDevice output_device = speaker/receiver/headphone*/
  dwRc = str_parms_get_str(str_parms, MMI_OUT_DEVICE_KEY, strVal, sizeof(strVal));
  if (0 <= dwRc) {
    if (0 == strcmp(strVal, MMI_SPEAKER)) {
      pMgrInfo->tDevInfo.dwOutDevice = AUDIO_DEVICE_OUT_SPEAKER;
    } else if (0 == strcmp(strVal, MMI_RECEIVER)) {
      pMgrInfo->tDevInfo.dwOutDevice = AUDIO_DEVICE_OUT_EARPIECE;
    } else if  (0 == strcmp(strVal, MMI_HEADPHONE)) {
      pMgrInfo->tDevInfo.dwOutDevice = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
    } else {
      ALOGE("%s MMI_OUT_DEVICE_KEY value is error! strVal = %s", __FUNCTION__, strVal);
      return MMI_TEST_ARG_ERR;
    }
  }
  /*MMI Test inDevice input_device=mainmic/submic/dualmic/hsmic*/
  dwRc = str_parms_get_str(str_parms, MMI_IN_DEVICE_KEY, strVal, sizeof(strVal));
  if (0 <= dwRc) {
    if (0 == strcmp(strVal, MMI_MAIN_MIC)) {
      pMgrInfo->tDevInfo.dwInDevice = AUDIO_DEVICE_IN_BUILTIN_MIC;
    } else if (0 == strcmp(strVal, MMI_SUBMIC)) {
      pMgrInfo->tDevInfo.dwInDevice = AUDIO_DEVICE_IN_BACK_MIC;
    } else {
      ALOGE("%s MMI_IN_DEVICE_KEY value is error! strVal = %s", __FUNCTION__, strVal);
      return MMI_TEST_ARG_ERR;
    }
  }
  /*Audio Loop back need modify when get huawei more information */
  dwRc = str_parms_get_str(str_parms, MMI_AUDIO_LOOP, strVal, sizeof(strVal));
  if (0 <= dwRc) {
    if (0 == strcmp(strVal, MMI_MAIN_MIC)) {
      pMgrInfo->tDevInfo.dwInDevice = AUDIO_DEVICE_IN_BUILTIN_MIC;
    } else if (0 == strcmp(strVal, MMI_SUBMIC)) {
      pMgrInfo->tDevInfo.dwInDevice = AUDIO_DEVICE_IN_BACK_MIC;
    } else {
      ALOGE("%s MMI_AUDIO_LOOP value is error! strVal = %s", __FUNCTION__, strVal);
      return MMI_TEST_ARG_ERR;
    }
  }
  return MMI_TEST_NO_ERR;
}


/**
Is Enable for audio mmi test
**/

int AudioMMIIsEnable(AUDIO_MMI_HANDLE handle) {
  AUDIO_MMI_MGR_INFO *pMgrInfo = NULL;
  if (NULL == handle) {
    ALOGE("%s 0x%x,  Pointer is NULL", __FUNCTION__, handle);
    return MMI_TEST_ARG_ERR;
  }
  pMgrInfo = (AUDIO_MMI_MGR_INFO *)handle;
  if (MMI_TEST_START == pMgrInfo->flag) {
    return MMI_TEST_START;
  }else {
    return MMI_TEST_CLOSE;
  }
}


/**
 Debug Function
**/
int AudioMMITestSetInfo(AUDIO_MMI_HANDLE handle) {
  AUDIO_MMI_MGR_INFO *pMgrInfo = NULL;

  if (NULL == handle) {
    ALOGE("%s 0x%x,  Pointer is NULL", __FUNCTION__, handle);
    return MMI_TEST_ARG_ERR;
  }
  pMgrInfo = (AUDIO_MMI_MGR_INFO *)handle;
  pMgrInfo->flag = 1;
  pMgrInfo->tDevInfo.dwOutDevice = AUDIO_DEVICE_OUT_SPEAKER;
  pMgrInfo->tDevInfo.dwInDevice = AUDIO_DEVICE_IN_BUILTIN_MIC;
  return MMI_TEST_NO_ERR;
}

int AudioMMITestSetStr(AUDIO_MMI_HANDLE handle) {
  int dwRc = MMI_TEST_NO_ERR;
  char *pStr = "mmi_test=on";
  struct str_parms *parms = NULL;
  parms = str_parms_create_str(pStr);
  if (NULL == parms) {
    ALOGE("%s str_parms_create_str is Fail", __FUNCTION__);
  }
  dwRc = AudioMMIParse(parms, handle);
  if (dwRc) {
    ALOGE("%s dwRc = 0x%x", dwRc, __FUNCTION__);
  }

  char *pStr1 = "output_device=speaker";
  parms = str_parms_create_str(pStr1);
  if (NULL == parms) {
    ALOGE("%s str_parms_create_str is Fail", __FUNCTION__);
  }
  dwRc = AudioMMIParse(parms, handle);
  if (dwRc) {
    ALOGE("%s dwRc = 0x%x", dwRc, __FUNCTION__);
  }

  char *pStr2 = "input_device=mainmic";
  parms = str_parms_create_str(pStr2);
  if (NULL == parms) {
    ALOGE("%s str_parms_create_str is Fail", __FUNCTION__);
  }
  dwRc = AudioMMIParse(parms, handle);
  if (dwRc) {
    ALOGE("%s dwRc = 0x%x", dwRc, __FUNCTION__);
  }

  return MMI_TEST_NO_ERR;
}

