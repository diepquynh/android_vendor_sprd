/* Copyright 2009-2011 Broadcom Corporation
**
** This program is the proprietary software of Broadcom Corporation and/or its
** licensors, and may only be used, duplicated, modified or distributed
** pursuant to the terms and conditions of a separate, written license
** agreement executed between you and Broadcom (an "Authorized License").
** Except as set forth in an Authorized License, Broadcom grants no license
** (express or implied), right to use, or waiver of any kind with respect to
** the Software, and Broadcom expressly reserves all rights in and to the
** Software and all intellectual property rights therein.
** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
** ALL USE OF THE SOFTWARE.
**
** Except as expressly set forth in the Authorized License,
**
** 1.     This program, including its structure, sequence and organization,
**        constitutes the valuable trade secrets of Broadcom, and you shall
**        use all reasonable efforts to protect the confidentiality thereof,
**        and to use this information only in connection with your use of
**        Broadcom integrated circuit products.
**
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
**        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
**        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
**        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
**        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
**        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
**        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
**        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
**        OF USE OR PERFORMANCE OF THE SOFTWARE.
**
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
**        ITS LICENSORS BE LIABLE FOR
**        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
**              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
**              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
**              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
**        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
**              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
**              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
**              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/

#define LOG_TAG "com_broadcom_bt_service_fm_FmReceiverService.cpp"
#define LOG_NDEBUG 0

#include "fm_cfg.h"

//#include "jni.h"
#if IS_STANDALONE_FM != TRUE
//Remove includes used in framework
#include "JNIHelp.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "android_runtime/AndroidRuntime.h"
#else
#include "pthread.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <ctype.h>
#include <CMutex.h>
// for FM-BT_ON-OFF
#ifdef HAVE_BLUETOOTH
#include <dbus/dbus.h>
#include <bluedroid/bluetooth.h>
#endif

//#include <btl_ifc_wrapper.h>

#ifdef BRCM_BT_USE_BTL_IF
extern "C"
{
#include <btl_ifc.h>
};
#endif

#ifdef BRCM_USE_BTAPP
extern "C"
{
#include <btapp_fm.h>
};

#endif

#include "com_broadcom_bt_service_fm_FmReceiverService_int.h"

#if IS_STANDALONE_FM != TRUE
namespace android {
#endif

typedef struct {
    int event;

} tANDROID_EVENT_FM_RX;

#define DTUN_LOCAL_SERVER_ADDR "brcm.bt.dtun"


#define USER_CLASSPATH "."

static bool systemActive;

#ifdef BRCM_BT_USE_BTL_IF
static tCTRL_HANDLE btl_if_fm_handle;
#endif

/* Native callback data. */
typedef struct {
    pthread_mutex_t mutex;
//    jobject object;       // JNI global ref to the Java object
} fm_native_data_t;

/* Local static reference to callback java object. */
//static JNIEnv *local_env;
static fm_native_data_t fm_nat;

static tBTA_FM_RDS_UPDATE      previous_rds_update;
static int rds_type_save;
static bool af_on_save;
static bool rds_on_save;

/* Declarations. */
#ifdef BRCM_BT_USE_BTL_IF
static void decodePendingEvent(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
#endif

static void fmReceiverServiceCallbackNative (tBTA_FM_EVT event, tBTA_FM* data);

static void enqueuePendingEvent(tBTA_FM_EVT new_event, tBTA_FM* new_event_data);
static bool setRdsRdbsNative(int rdsType);

CMutex gMutex;
#ifdef BRCM_BT_USE_BTL_IF
tBTA_FM gfm_params;
tBTLIF_CTRL_MSG_ID gCurrentEventID = BTLIF_FM_CMD_BASE ;
tBTA_FM_CTX gfm_context = {0};

/* Callback handler for stack originated events. */
static void decodePendingEvent(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params) {

    tBTA_FM *fm_params = &gfm_params;
    gCurrentEventID = id;
    LOGI("%s: Event ID: %d",__FUNCTION__,id);

    /* Deecode the events coming from the BT-APP and enqueue them for further upwards transport. */
    switch (id) {
        case BTLIF_FM_ENABLE:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->status = (tBTA_FM_STATUS)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_ENABLE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_DISABLE:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->status = (tBTA_FM_STATUS)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_DISABLE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_TUNE:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            LOGI("%s: Event ID: %d, Frequence:%d\n",__FUNCTION__,id, (UINT16)(params->fm_IIII_param.i3));

            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->chnl_info.freq = gfm_context.cur_freq = (UINT16)(params->fm_IIII_param.i3);
            fm_params->chnl_info.snr = (INT8)(params->fm_IIII_param.i4);
            enqueuePendingEvent(BTA_FM_TUNE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_MUTE:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->mute_stat.status = (tBTA_FM_STATUS)(params->fm_IZ_param.i1);
            fm_params->mute_stat.is_mute = (BOOLEAN)(params->fm_IZ_param.z1);
            enqueuePendingEvent(BTA_FM_MUTE_AUD_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_SEARCH:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            LOGI("%s: Event ID: %d, Frequence:%d\n",__FUNCTION__,id, (UINT16)(params->fm_III_param.i2));
            fm_params->scan_data.rssi = (UINT8)(params->fm_III_param.i1);
            fm_params->scan_data.freq = (UINT16)(params->fm_III_param.i2);
            fm_params->scan_data.snr = (INT8)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_SEARCH_EVT, fm_params);
            break;
        case BTLIF_FM_SEARCH_CMPL_EVT:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->chnl_info.freq = gfm_context.cur_freq = (UINT16)(params->fm_IIII_param.i3);
            fm_params->chnl_info.snr = (INT8)(params->fm_IIII_param.i4);

            enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, fm_params);
            gMutex.signal();
            LOGI("BTLIF_FM_SEARCH_CMPL_EVT: %d, %d, %d, %d",fm_params->chnl_info.status, fm_params->chnl_info.rssi, fm_params->chnl_info.freq, fm_params->chnl_info.snr);
            break;
        case BTLIF_FM_SEARCH_ABORT:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_III_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_III_param.i2);
            fm_params->chnl_info.freq = gfm_context.cur_freq = (UINT16)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, fm_params);
            gMutex.signal();
            LOGI("BTLIF_FM_SEARCH_ABORT: %d, %d, %d",fm_params->chnl_info.status, fm_params->chnl_info.rssi, fm_params->chnl_info.freq);
            break;
        case BTLIF_FM_SET_RDS_MODE:
            LOGI("%s: RDS MODE EVENT",__FUNCTION__);
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_mode.status = (tBTA_FM_STATUS)(params->fm_IZZ_param.i1);
            fm_params->rds_mode.rds_on = (BOOLEAN)(params->fm_IZZ_param.z1);
            fm_params->rds_mode.af_on = (BOOLEAN)(params->fm_IZZ_param.z2);
            enqueuePendingEvent(BTA_FM_RDS_MODE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_SET_RDS_RBDS:
            LOGI("%s: RDS TYPE EVENT",__FUNCTION__);
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_type.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->rds_type.type = (tBTA_FM_RDS_B)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_RDS_TYPE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_RDS_UPDATE:
            LOGI("%s: RDS UPDATE EVENT",__FUNCTION__);
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_update.status = (tBTA_FM_STATUS)(params->fm_IIIS_param.i1);
            fm_params->rds_update.data = (UINT8)(params->fm_IIIS_param.i2);
            fm_params->rds_update.index = (UINT8)(params->fm_IIIS_param.i3);
            memcpy(fm_params->rds_update.text, params->fm_IIIS_param.s1, 65);
            enqueuePendingEvent(BTA_FM_RDS_UPD_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_AUDIO_MODE:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->mode_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->mode_info.audio_mode = (tBTA_FM_AUDIO_MODE)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_AUD_MODE_EVT, fm_params);
            gMutex.signal();
            break;
        case BTLIF_FM_AUDIO_PATH:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->path_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->path_info.audio_path = (tBTA_FM_AUDIO_PATH)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_AUD_PATH_EVT, fm_params);
            break;
        case BTLIF_FM_SCAN_STEP:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->scan_step = (tBTA_FM_STEP_TYPE)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_SCAN_STEP_EVT, fm_params);
            break;
        case BTLIF_FM_SET_REGION:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->region_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->region_info.region = (tBTA_FM_REGION_CODE)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_SET_REGION_EVT, fm_params);
            break;
        case BTLIF_FM_CONFIGURE_DEEMPHASIS:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->deemphasis.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->deemphasis.time_const = (tBTA_FM_DEEMPHA_TIME)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_SET_DEEMPH_EVT, fm_params);
            break;
        case BTLIF_FM_ESTIMATE_NFL:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->nfloor = (UINT8)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_NFL_EVT, fm_params);
            break;
        case BTLIF_FM_GET_AUDIO_QUALITY:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->audio_data.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->audio_data.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->audio_data.audio_mode = (tBTA_FM_AUDIO_QUALITY)(params->fm_IIII_param.i3);
            fm_params->audio_data.snr = (INT8)(params->fm_IIII_param.i4);
            enqueuePendingEvent(BTA_FM_AUD_DATA_EVT, fm_params);
            break;
        case BTLIF_FM_AF_JMP_EVT:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_III_param.i1);
            fm_params->chnl_info.freq = (UINT16)(params->fm_III_param.i2);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_AF_JMP_EVT, fm_params);
			break;
        case BTLIF_FM_SET_VOLUME_EVT:
            //fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
	        fm_params->volume.status= (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->volume.volume= (UINT16)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_VOLUME_EVT, fm_params);
            gMutex.signal();
            LOGI("BTLIF_FM_SET_VOLUME_EVT: volume %d, %d",fm_params->volume.status, fm_params->volume.volume);
            break;

        default: gCurrentEventID = BTLIF_FM_CMD_BASE ; break;
    }

}

#endif

/* Assumes data exists. Screens the text field for illegal (non ASCII) characters. */
static void screenData(tBTA_FM* data) {
    int i = 0;
    char c = data->rds_update.text[i];

    while ((i < 65) && (0 != c)) {
        /* Check legality of all characters. */
        if ((c < 32) || (c > 126)) {
            data->rds_update.text[i] = '*';
        }
        /* Look at next character. */

		i+=1;
		if(i <= 64){
            c = data->rds_update.text[i];
        }
    }
    /* Cap for safety at end of 64 bytes. */
    data->rds_update.text[64] = 0;
}

/* Forwards the event to the application. */
static void enqueuePendingEvent(tBTA_FM_EVT new_event, tBTA_FM* new_event_data) {

}


boolean enableNative(int functionalityMask)
{
    LOGI("[JNI] - enableNative :    functionalityMask = %d", functionalityMask);

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_I_PARAM params;
    tBTL_IF_Result res;
    // Call fm_enable to load btld if needed
    #if IS_STANDALONE_FM != TRUE
    //if (fm_enable() != -1)
    //{
    #endif
        /* Initialize datapath client  */
	    res = BTL_IFC_ClientInit();

	    LOGI("[JNI] - enableNative :    INIT = %i", (int)res);

	    /* Register with BTL-IF as subsystem client. */
	    res = BTL_IFC_RegisterSubSystem(&btl_if_fm_handle, SUB_FM, NULL, decodePendingEvent);

    LOGI("[JNI] - enableNative :    REGISTER = %i", (int)res);

    /* Set up bt-app parameters. */
    params.i1 = (INT32) functionalityMask;

    res = BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ENABLE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

	    LOGI("[JNI] - enableNative :    ENABLE = %i", (int)res);
	if(res!= BTL_IF_SUCCESS)
	    return false;
    #if IS_STANDALONE_FM != TRUE
   // }
    #endif

#else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

        /* Simulate a matched thread environment. */
        local_env = env;
        fm_nat.object = obj;

    if (NULL != data) {
        data->status = BTA_FM_OK;
        enqueuePendingEvent(BTA_FM_ENABLE_EVT, data);
    }
#endif
    return true;
}

boolean disableNative(boolean bForcing)
{
    LOGI("[JNI] - disableNative :");

#ifdef BRCM_BT_USE_BTL_IF
    tBTL_IF_Result res,res2;

    #if IS_STANDALONE_FM != TRUE

    if (bForcing == true)
    {
	    fm_disable();
    } else {
    #endif
        pthread_mutex_lock(&fm_nat.mutex);

        res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_DISABLE, NULL, 0);
        pthread_mutex_unlock(&fm_nat.mutex);

	    if((res!= BTL_IF_SUCCESS))
	        return false;
    #if IS_STANDALONE_FM != TRUE
    }
    #endif
#else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->status = BTA_FM_OK;
        enqueuePendingEvent(BTA_FM_DISABLE_EVT, data);
    }
#endif
    return true;
}

boolean muteNative(boolean toggle)
{
    LOGI("[JNI] - muteNative :    toggle = %i", toggle);

#ifdef BRCM_BT_USE_BTL_IF
    tBTL_IF_Result res;
    tBTLIF_FM_REQ_Z_PARAM params;

    /* Set up bt-app parameters. */
    params.z1 = (BOOLEAN) toggle;

    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_MUTE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_Z_PARAM));

    if(res!= BTL_IF_SUCCESS)
		return false;

    #else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->mute_stat.status = BTA_FM_OK;
        data->mute_stat.is_mute = toggle;
        enqueuePendingEvent(BTA_FM_MUTE_AUD_EVT, data);
    }
#endif
    return true;
}

boolean tuneNative(int freq)
{
    LOGI("[JNI] - tuneNative :    freq = %i", freq);

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_I_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (int) freq;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_TUNE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
		return false;
#else

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->chnl_info.status = BTA_FM_OK;
        data->chnl_info.rssi = 120;
        data->chnl_info.freq = (UINT16) freq;
        enqueuePendingEvent(BTA_FM_TUNE_EVT, data);
    }
#endif
    return true;
}

bool   searchNative(int scanMode, int rssiThreshold, int condVal, int condType)
{
    LOGI("[JNI] - searchNative :    scanMode = %i  rssiThreshold = %i  condVal = %i  condType = %i", scanMode, rssiThreshold, condVal, condType);

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_IIII_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (int) scanMode;
    params.i2 = (int) rssiThreshold;
    params.i3 = (int) condVal;
    params.i4 = (int) condType;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_IIII_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
		return false;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->scan_data.rssi = 120;
        data->scan_data.freq = 10000;
        data->scan_data.snr = 20;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return true;
}

bool comboSearchNative(int startFreq,  int endFreq, int rssiThreshold, int direction, int scanMethod, bool multiChannel, int rdsType, int rdsTypeValue)
{
    LOGI("[JNI] - comboSearchNative");

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_IIIIIZII_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (UINT16) startFreq;
    params.i2 = (UINT16) endFreq;
    params.i3 = (int) rssiThreshold;
    params.i4 = (int) direction;
    params.i5 = (int) scanMethod;
    params.z1 = (BOOLEAN) multiChannel;
    params.i6 = (int) rdsType;
    params.i7 = (int) rdsTypeValue;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_COMBO_SEARCH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_IIIIIZII_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
		return false;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->scan_data.rssi = 120;
        data->scan_data.freq = 10000;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return true;
}

bool searchAbortNative()
{
    LOGI("[JNI] - searchAbortNative :");

#ifdef BRCM_BT_USE_BTL_IF
    tBTL_IF_Result res;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH_ABORT, NULL, 0);
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
		return false;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->chnl_info.status = BTA_FM_SCAN_ABORT;
        data->chnl_info.rssi = 30;
        data->chnl_info.freq = 10000;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return true;
}

bool setRdsModeNative(bool rdsOn, bool afOn, int rdsType)
{
    LOGI("[JNI] - setRdsModeNative :");

#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_ZZ_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.z1 = (BOOLEAN) rdsOn;
     params.z2 = (BOOLEAN) afOn;
     rds_type_save = (int) rdsType;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_MODE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_ZZ_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->rds_mode.status = BTA_FM_OK;
        data->rds_mode.rds_on = rdsOn;
        data->rds_mode.af_on = afOn;
        enqueuePendingEvent(BTA_FM_RDS_MODE_EVT, data);
    }
#endif
    return true;
}

//static bool setRdsRdbsNative(JNIEnv *env, jobject obj, int rdsType)
bool setRdsRdbsNative(int rdsType)
{
    LOGI("[JNI] - setRdsRdbsNative :");

#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) rdsType;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_RBDS, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
   // local_env = env;
    //fm_nat.object = obj;

    if (NULL != data) {
        data->rds_type.status = BTA_FM_OK;
        data->rds_type.type = (tBTA_FM_RDS_B) rdsType;
        enqueuePendingEvent(BTA_FM_RDS_TYPE_EVT, data);
    }
#endif
    return true;
}

bool setAudioModeNative(int audioMode)
{
    LOGI("[JNI] - setAudioModeNative :    audioMode = %i", audioMode);

#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) audioMode;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_MODE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->mode_info.status = BTA_FM_OK;
        data->mode_info.audio_mode = (tBTA_FM_AUDIO_MODE)audioMode;
        enqueuePendingEvent(BTA_FM_AUD_MODE_EVT, data);
    }
#endif

    return true;
}

bool setAudioPathNative(int audioPath)
{
    LOGI("[JNI] - setAudioPathNative :    audioPath = %i", audioPath);

#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) audioPath;

     res = BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_PATH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;
#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->path_info.status = BTA_FM_OK;
        data->path_info.audio_path = (tBTA_FM_AUDIO_PATH)audioPath;
        enqueuePendingEvent(BTA_FM_AUD_PATH_EVT, data);
    }
#endif

    return true;
}




bool setScanStepNative(int stepSize)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) stepSize;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SCAN_STEP, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->scan_step = (tBTA_FM_STEP_TYPE)stepSize;
        enqueuePendingEvent(BTA_FM_SCAN_STEP_EVT, data);
    }
#endif
    return true;
}

bool setRegionNative(int region)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) region;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_REGION, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->region_info.status = BTA_FM_OK;
        data->region_info.region = (tBTA_FM_REGION_CODE) region;
        enqueuePendingEvent(BTA_FM_SET_REGION_EVT, data);
    }
#endif
    return true;
}

bool configureDeemphasisNative(int time)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) time;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_CONFIGURE_DEEMPHASIS,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;


#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->deemphasis.status = BTA_FM_OK;
        data->deemphasis.time_const = (tBTA_FM_DEEMPHA_TIME) time;
        enqueuePendingEvent(BTA_FM_SET_DEEMPH_EVT, data);
    }
#endif
    return true;
}

bool estimateNoiseFloorNative(int level)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) level;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ESTIMATE_NFL,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;


#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->nfloor = (tBTA_FM_NFE_LEVL) level;
        enqueuePendingEvent(BTA_FM_NFL_EVT, data);
    }
#endif
    return true;
}

bool getAudioQualityNative(bool enable)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_Z_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.z1 = (BOOLEAN) enable;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_GET_AUDIO_QUALITY,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_Z_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

#endif
    return true;
}

bool configureSignalNotificationNative(int pollInterval)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) pollInterval;

     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

#endif
    return true;
}

bool setFMVolumeNative(int volume)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) volume;
    LOGI("[JNI] - setFMVolumeNative volume =%d", volume);
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_VOLUME, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else


#endif
    return true;
}

bool setSnrThresholdNative(int snr_thres)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) snr_thres;
    LOGI("[JNI] - setSnrThresholdNative snr =%d", snr_thres);
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_SNR_THRES, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
		return false;

#else

#endif
    return true;
}
