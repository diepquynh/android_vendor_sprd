/************************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/
#include "bt_target.h"

#if( defined BTA_FM_INCLUDED ) && (BTA_FM_INCLUDED == TRUE)

#include "gki.h"
#include "bta_api.h"
#include "bta_fm_api.h"
#include "bta_prm_api.h"
#include "bta_rds_api.h"
#include "bd.h"
#include "bte_appl.h"
#include "btui.h"
#include "btui_int.h"
#include "btapp_fm.h"

#include "btl_ifs.h"

#include <ctype.h>


#define LOG_TAG "BTAPP_FM"
#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

/* defines BT chip HW path to by default. see bta_fm_api.h for valid infos */
#ifndef BTAPP_FM_AUDIO_PATH
#define BTAPP_FM_AUDIO_PATH BTA_FM_AUDIO_DAC
#endif

/* if true only singe HW routing paths is assumed, DAC or I2S but not both. If false, API selection
 * may allow i2s and dac */
#ifndef BTAPP_FM_SINGLE_PATH_ONLY
#define BTAPP_FM_SINGLE_PATH_ONLY TRUE
#endif

#ifndef BTAPP_FM_USE_HW_POKE_VOLUME
#define BTAPP_FM_USE_HW_POKE_VOLUME TRUE
#endif

/* By default, Customer-specific host platform audio routing should be called before calling BTA_FmConfigAudioPath().
 * However it's recommended to set it to TRUE in btld.txt. This way, the result on Fm chip side
 * is known! */
#ifndef BTAPP_FM_AUDIO_PATH_POST_SET
#define BTAPP_FM_AUDIO_PATH_POST_SET FALSE
#endif

/* originally 85. however 85 is quite low causing continuous jumps. NFL should be run too
 * to give a better estimation */
#ifndef BTAPP_FM_AF_THRESH
#define BTAPP_FM_AF_THRESH 95
#endif

const UINT8* audio_mode_name[4] =
{
    "Auto",
    "Stereo",
    "Mono",
    "Switch"
};

static UINT8 rssi_thresh_audio[4] =
{
    85, /* "Auto"   */
    70, /* "Stereo" */
    85, /* "Mono"   */
    85  /* "Switch"  */
};

static const char * region_name[BTA_FM_REGION_MAX + 2] =
{
    "North America",
    "Europe",
    "Japan",
    "Japan II",
    "Russia-Ext",
    "China",
    NULL
};

static int count = 0;
tCTRL_HANDLE btl_if_fm_handle;

/* Pending operation parameters. */

static BOOLEAN pending_search = FALSE;
static BOOLEAN search_in_porgress = FALSE;
static tBTA_FM_SCAN_MODE pending_search_scan_mode = 0;
static UINT8 pending_search_rssi = 0;
static tBTA_FM_SCH_RDS_TYPE pending_search_cond_type = 0;
static UINT8 pending_search_cond_val = 0;

#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
static UINT16 SetVolume_CB_Needed = 0;
static UINT16 FMVolumeData = 0;
#define HCI_BRCM_SUPER_PEEK_POKE_LENGTH    9
static UINT8  hci_SetVolumePeekPokedata[HCI_BRCM_SUPER_PEEK_POKE_LENGTH] =
              {0x05,0xC0,0x41,0x0F,0x00,0x20,0x00,0x00,0x00};
#endif

#define BTUI_RDS_STR_LEN_8      8
#define BTUI_RDS_STR_LEN_64     64

/* maximum FM audio volume */
#define     BTA_FM_VOLUME_MAX      255 //0x100
/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static void btapp_fm_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);


tBTUI_FM_CB btui_fm_cb;
tBTUI_RDS_CB btui_rds_cb;

static int save_audio_path = 0;

/* remapping of bta values. BT over SCO and A2DP are missing. keep in sync with is sent from jni! */
#define BTAPP_FM_AUDIO_PATH_NONE            0
#define BTAPP_FM_AUDIO_PATH_SPEAKER         1
#define BTAPP_FM_AUDIO_PATH_WIRED_HEADSET   2
#define BTAPP_FM_AUDIO_PATH_DIGITAL         3

#if ( BTAPP_FM_SUPPORT_AUDIOPATH==TRUE )
// ***************************************************
// *** Customer specific Audio Path variables go gere
// ***************************************************

/* Example of host platform volume control block. ADAPT to customer */
typedef struct tBTAPP_FM_PLAT_CB_tag {
    /* tBTAPP_HOST_AUDIO_CHNL  current_path; btlif host specific audio routing. OPTIONAL */
    tBTA_FM_AUDIO_PATH      current_bta_path;  /* BTA AudioPath used to decide if newly request
                                                  requires an API call */
#if 0
    const char              *init_audio_path;
    const char              *fs_control_path;
#endif
} tBTAPP_FM_PLAT_CB;

#if 0
#define HEAD_PHONE_OFF_STR "0 1 8 3 30"
static const char host_init_audio_path_str[] = HEAD_PHONE_OFF_STR;
#define MAX_AUDIO_STR_LEN (2*sizeof(HEAD_PHONE_OFF_STR))
static const char host_audio_control_path[] = "/sys/class/sound/controlC0/FMDirectPlay";
#endif

static tBTAPP_FM_PLAT_CB btapp_fm_plat_cb = {
        /* BTAPP_FM_AUDIO_PATH_NONE, btlif host specific */
        BTA_FM_AUDIO_DAC,
#if 0
        host_init_audio_path_str,
        host_audio_control_path
#endif
};

#endif

#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
// Callback method declaration
static void hci_SuperPeekPoke_cback(tBTM_VSC_CMPL *p);
#endif


/*******************************************************************************
**
** Function         btui_fm_init
**
** Description      Initializes FM Radio module.
**
** Returns          void
*******************************************************************************/
void btapp_fm_init(void)
{
    tBTA_FM_FUNC_MASK   enable_mask = btui_cfg.fm_func_mask|BTA_FM_REGION_NA;
    tBTL_IF_Result result;

    /* Initialize datapath server */
    result = BTL_IF_ServerInit();

    LOGI("BTAPP_FM: Initialized IFS (res = %d)", result);
    btui_fm_cb.volume = BTA_FM_VOLUME_MAX;
    /* Register handler for subsystem FM */
    result = BTL_IF_RegisterSubSystem(&btl_if_fm_handle, SUB_FM, NULL, btapp_fm_on_rx_ctrl);
    LOGI("BTAPP_FM: Registered IFS (res = %d)", result);

    btui_cfg.fm_snr_thresh = bte_appl_cfg.fm_snr;
    LOGI("BTAPP_FM: fm_snr = 0x%x", btui_cfg.fm_snr_thresh);

    /* Set dtun server property flag to signal the bluedroid that FM has turned on successfully */
    property_set(DTUN_PROPERTY_SERVER_ACTIVE, "1");

    if (btui_cfg.fm_included && !btui_fm_cb.enabled)
    {
        if (btui_cfg.fm_rssi_stereo != 0)
            rssi_thresh_audio[1] = btui_cfg.fm_rssi_stereo;

        if (btui_cfg.fm_rssi_mono != 0)
            rssi_thresh_audio[0] = rssi_thresh_audio[2] = rssi_thresh_audio[3] = btui_cfg.fm_rssi_mono;
    }
}

/*******************************************************************************
**
** Function         btapp_fm_disable
**
** Description      Disable FM.
**
** Returns          void
*******************************************************************************/
void btapp_fm_disable(void)
{
    tBTUI_FM_Params     fm_params;

    memcpy(&fm_params, &(btui_fm_cb.parms), sizeof(tBTUI_FM_Params));
    memset(&btui_fm_cb, 0, sizeof(tBTUI_FM_CB));
    memset(&btui_rds_cb, 0, sizeof(tBTUI_RDS_CB));

    /* The following fields should not be cleared after FM is turned off. */
    memcpy(&(btui_fm_cb.parms), &fm_params, sizeof(tBTUI_FM_Params));

    BTA_FmDisable();

    BTL_IF_UnregisterSubSystem(SUB_FM);
}


/*******************************************************************************
**
** Function         btapp_fm_tune_freq
**
** Description      Connects to the selected device
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_tune_freq(UINT16 freq)
{
    btui_rds_cb.ps_on = btui_rds_cb.pty_on = btui_rds_cb.ptyn_on = btui_rds_cb.rt_on = FALSE;
    BTA_FmTuneFreq(freq);

}

/*******************************************************************************
**
** Function         btapp_fm_search_freq
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_search_freq(tBTA_FM_SCAN_MODE scn_mode, BOOLEAN combo)
{
    UINT8 rssi;
    BOOLEAN multi = FALSE;
    UINT16 freq = (scn_mode == BTA_FM_SCAN_DOWN) ? btui_fm_cb.cur_freq - BTUI_FM_DEFAULT_STEP :
                        btui_fm_cb.cur_freq + BTUI_FM_DEFAULT_STEP;
    tBTA_FM_CVALUE      criteria;

    tBTA_FM_SCAN_METHOD method = (scn_mode == BTA_FM_FAST_SCAN)?\
                       BTA_FM_PRESET_SCAN : BTA_FM_NORMAL_SCAN;
    tBTA_FM_SCAN_DIR dir = (scn_mode == BTA_FM_SCAN_DOWN) ? scn_mode : BTA_FM_SCAN_UP;

    if (btui_cfg.addi_thresh)
    {
        criteria.snr = btui_cfg.fm_snr_thresh;
        if (BTA_FmSetSchCriteria(BTA_FM_CTYPE_SNR, &criteria) != BTA_FM_OK)
        {
            APPL_TRACE_ERROR0("write SNR criteria failed");
        }
        criteria.cos = btui_cfg.fm_cos_thresh;
        if (BTA_FmSetSchCriteria(BTA_FM_CTYPE_QUALITY, &criteria) != BTA_FM_OK)
        {
            APPL_TRACE_ERROR0("write COS criteria failed");
        }
    }

    if (scn_mode == BTA_FM_FAST_SCAN || scn_mode == BTA_FM_SCAN_FULL)
    {
//        memset(&btui_fm_db, 0, sizeof(tBTUI_FM_DB));
        memset(btui_fm_cb.stat_lst, 0, sizeof(UINT16) * BTUI_FM_MAX_STAT_LST);
        btui_fm_cb.stat_num = 0;
        multi = TRUE;
    }
    rssi = btui_fm_cb.nfe_thresh ? 0 : rssi_thresh_audio[btui_fm_cb.mode];

    if (TRUE == combo)
    {
        if (scn_mode == BTA_FM_SCAN_FULL || scn_mode == BTA_FM_FAST_SCAN)
            freq = btui_fm_cb.cur_freq;
        else if (scn_mode == BTA_FM_SCAN_DOWN)
            freq = btui_fm_cb.cur_freq - BTUI_FM_DEFAULT_STEP ;
        else
            freq = btui_fm_cb.cur_freq + BTUI_FM_DEFAULT_STEP;

        if (freq > BTUI_FM_MAX_FREQ)
            freq = BTUI_FM_MAX_FREQ;
        if (freq < BTUI_FM_MIN_FREQ)
            freq = BTUI_FM_MIN_FREQ;

        BTA_FmComboSearch(freq, freq,  rssi_thresh_audio[btui_fm_cb.mode],
                      dir, method, multi, NULL);
    }
    else
        BTA_FmSearchFreq(scn_mode, rssi, NULL);

    btui_rds_cb.ps_on = btui_rds_cb.pty_on = btui_rds_cb.ptyn_on = btui_rds_cb.rt_on = FALSE;

    return;
}
/*******************************************************************************
**
** Function         btapp_fm_rds_search
**
** Description
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_rds_search(tBTA_FM_SCAN_MODE scn_mode, tBTA_FM_SCH_RDS_COND rds_cond)
{
    UINT8 rssi;

    if (scn_mode == BTA_FM_SCAN_FULL)
    {
//        memset(&btui_fm_db, 0, sizeof(tBTUI_FM_DB));
        memset(btui_fm_cb.stat_lst, 0, sizeof(UINT16) * BTUI_FM_MAX_STAT_LST);
        btui_fm_cb.stat_num = 0;
    }

    rssi = btui_fm_cb.nfe_thresh ? 0 : rssi_thresh_audio[btui_fm_cb.mode];

    BTA_FmSearchFreq(scn_mode, rssi, &rds_cond);

    btui_rds_cb.ps_on = btui_rds_cb.pty_on = btui_rds_cb.ptyn_on = btui_rds_cb.rt_on = FALSE;

    return;
}

/*******************************************************************************
**
** Function         btapp_fm_set_scan_step
**
** Description      Config FM scan step: 50KHz or 100 KHz
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_scan_step(tBTA_FM_STEP_TYPE step)
{
    BTA_FmSetScanStep(step);

    return;
}
/*******************************************************************************
**
** Function         btapp_fm_set_rds_mode
**
** Description      Turn on/off RDS/AF mode.
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_rds_mode(BOOLEAN rds_on, BOOLEAN af_on)
{
	UINT8   i = 0;
    tBTA_FM_AF_PARAM *p_af_param = NULL;
    tBTA_FM_AF_LIST af_list = {2, {BTA_FM_AF_FM, BTA_FM_AF_FM}, { 1079, 880}};
    LOGI("BTAPP_FM: btapp_fm_set_rds_mode rds_on %i af_on %i ", rds_on, af_on);

    if (af_on && btui_fm_cb.af_cb.af_avail)
    {
        LOGI("BTAPP_FM: btapp_fm_set_rds_mode af_avail");
        if ((p_af_param = GKI_getbuf(sizeof(tBTA_FM_AF_PARAM))) != NULL)
        {
			p_af_param->af_thresh = (UINT8) BTAPP_FM_AF_THRESH;
            p_af_param->pi_code = btui_fm_cb.af_cb.pi_code;
            memcpy(&p_af_param->af_list, /*&af_list,*/  &btui_fm_cb.af_cb.af_list,
                sizeof(tBTA_FM_AF_LIST));
            while(i < p_af_param->af_list.num_af)
            {
				LOGI(">>>>>>>>>>>>>>>>>>>>>>>>>pi_code [%d]", p_af_param->pi_code);
				LOGI(">>>>>>>>>>>>>>>>>>>>>>>>>AF[%d] [%d]", i+1,
				p_af_param->af_list.af_list[i ++]);
            }

        }
    }

    /* enable RDS data polling with once every 3 seconds */
    BTA_FmSetRDSMode(rds_on, af_on, p_af_param);

    if (p_af_param)
        GKI_freebuf((void *)p_af_param);

    return;
}

/*******************************************************************************
**
** Function         btapp_fm_abort
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_fm_abort(void)
{
    BTA_FmSearchAbort();
}

/*******************************************************************************
**
** Function         btapp_fm_set_audio_mode
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_audio_mode(tBTA_FM_AUDIO_MODE mode)
{
    BTA_FmSetAudioMode(mode);
}

/*******************************************************************************
**
** Function         btapp_fm_read_rds
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_fm_read_rds(void)
{
    BTA_FmReadRDS();
}


/*******************************************************************************
**
** Function         btapp_fm_set_deemphasis
**
** Description
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_deemphasis(tBTA_FM_DEEMPHA_TIME interval)
{
    BTA_FmConfigDeemphasis(interval);
}

/*******************************************************************************
**
** Function         btapp_fm_mute_audio
**
** Description      Mute/unmute FM audio
**
** Returns          void
*******************************************************************************/
void btapp_fm_mute_audio(BOOLEAN mute)
{
    BTA_FmMute(mute);
}
/*******************************************************************************
**
** Function         btapp_fm_rssi_notification
**
** Description      Enable/disable dynamic RSSI read
**
** Returns          void
*******************************************************************************/
void btapp_fm_rssi_notification(BOOLEAN dyn_rssi)
{
    BTA_FmReadAudioQuality(dyn_rssi);
}

/*******************************************************************************
**
** Function         btapp_fm_set_region
**
** Description      Set region for band selection.
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_region(tBTA_FM_REGION_CODE region)
{
    BTA_FmSetRegion(region);
}
/*******************************************************************************
**
** Function         btapp_fm_set_rds
**
** Description      Set RDS/RBDS type.
**
** Returns          void
*******************************************************************************/
void btapp_fm_set_rds(tBTA_FM_RDS_B rds_type)
{
    BTA_FmSetRdsRbds(rds_type);
}

/*******************************************************************************
**
** Function         btapp_fm_get_nfe
**
** Description      Estimate noise floor.
**
** Returns          void
*******************************************************************************/
void btapp_fm_get_nfe (tBTA_FM_NFE_LEVL level)
{
    BTA_FmEstNoiseFloor(level);
}

/*******************************************************************************
**
** Function         btapp_fm_cfg_blend_mute
**
** Description      Configure the stereo/mono blend and soft mute parameters.
**
** Returns          void
*******************************************************************************/
void btapp_fm_cfg_blend_mute(UINT8 start_snr, UINT8 stop_snr, INT8 start_rssi,
                             INT8 stop_rssi, UINT8 start_mute, INT8 stop_atten,
                             UINT8 mute_rate, INT8 snr_40)
{
    tBTA_FM_BLEND_MUTE cfg;

    APPL_TRACE_DEBUG4("start_snr = %d stop_snr = %d start_rssi = %d stop_rssi = %d", start_snr, stop_snr, start_rssi, stop_rssi);
    APPL_TRACE_DEBUG4("start_mute = %d stop_atten = %d mute_rate = %d snr_40 = %d", start_mute, stop_atten, mute_rate, snr_40);

    cfg.blend.start_snr = start_snr;
    cfg.blend.stop_snr = stop_snr;
    cfg.blend.start_rssi = start_rssi;
    cfg.blend.stop_rssi = stop_rssi;

    cfg.soft_mute.mute_rate = mute_rate;
    cfg.soft_mute.snr40 = snr_40;
    cfg.soft_mute.start_mute = start_mute;
    cfg.soft_mute.stop_atten = stop_atten;

    BTA_FmConfigBlendSoftMuteParams(&cfg);

    return;
}
/*******************************************************************************
**
** Function         btapp_fm_volume_control
**
** Description      FM audio volume control.
**
** Returns          void
*******************************************************************************/
void btapp_fm_volume_control(UINT16 volume)
{
    APPL_TRACE_DEBUG2( "btapp_fm_volume_control(volume: %d) on current_bta_path: %d",
                       volume, btapp_fm_plat_cb.current_bta_path );
    if (BTA_FM_AUDIO_DAC==btapp_fm_plat_cb.current_bta_path)
    {
#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
        /* TODO: move this HW dependent code into BTA! Use BTA_FmVolumeControl() instead! */
        {
            tBTM_STATUS result;
            APPL_TRACE_DEBUG1("btapp_fm_volume_control() volume = %d", volume);
            FMVolumeData = volume;
            SetVolume_CB_Needed = 1;
            result = BTM_VendorSpecificCommand( HCI_BRCM_SUPER_PEEK_POKE,
                                        HCI_BRCM_SUPER_PEEK_POKE_LENGTH,(UINT8 *)hci_SetVolumePeekPokedata,
                                        (tBTM_CMPL_CB*) hci_SuperPeekPoke_cback);
            LOGI("BTAPP_FM: BTLIF_FM_SET_VOLUME set SUPER_PEEK_POKE result = %d",result);
        }
#else
    BTA_FmVolumeControl(volume);
#endif
    }
    else
    {
        tBTA_FM_VOLUME vol;
        /* tell that volume is not supported. it may still do something in the event handler based
         * on audiopath knowledge. report error if path not yet set!
         */
        vol.status = (BTA_FM_AUDIO_NONE!=btapp_fm_plat_cb.current_bta_path) ?
                    BTA_FM_UNSPT_ERR:BTA_FM_ERR;
        vol.volume = volume;
        /* in case of i2s or fm over BT, we still may want to do something. call callback directly */
        btui_fm_cback(BTA_FM_VOLUME_EVT, (tBTA_FM *)&vol);
    }
}
/*******************************************************************************
**
** Function         btapp_fm_clear_af_info
**
** Description      Reset AF information if a new frequency is set.
**
**
** Returns          void
*******************************************************************************/
static void btapp_fm_clear_af_info(void)
{
    BOOLEAN af_on = btui_fm_cb.af_cb.af_on ;
    LOGI("BTAPP_FM: clear_af_info");

    memset(&btui_fm_cb.af_cb, 0, sizeof(tBTUI_FM_AF_CB));

    if ((btui_fm_cb.af_cb.af_on = af_on)  == TRUE)
        btapp_fm_set_rds_mode(btui_fm_cb.rds_on, btui_fm_cb.af_cb.af_on);

}

/*******************************************************************************
**
** Function         btapp_fm_af_merge_list
**
** Description      For method-B AF encoding, merge up the AF list for seperate
**                  short lists based on the same currently tuned service.
**
**
** Returns          BOOLEAN: TRUE: when AF list is updated
**                           FALSE: No AF list updating needed.
**
*******************************************************************************/
static BOOLEAN btapp_fm_af_merge_list(tBTA_FM_AF_LIST *p_af_data)
{
    UINT8 i, j ;
    BOOLEAN duplicate, add_up = FALSE;
    tBTA_FM_AF_LIST *p_local_lst = &btui_fm_cb.af_cb.af_list;
    LOGI("BTAPP_FM: af_merge_list");

    for (i = 0; i < p_af_data->num_af; i ++)
    {
        duplicate = FALSE;

        for (j = 0; j < p_local_lst->num_af; j ++)
        {
            if (p_af_data->af_list[i] == p_local_lst->af_list[j])
            {
                duplicate = TRUE;
                break;
            }
        }
        if ( ! duplicate)
        {
            add_up = TRUE;
            p_local_lst->af_type[p_local_lst->num_af] = p_af_data->af_type[i];
            p_local_lst->af_list[p_local_lst->num_af ++] = p_af_data->af_list[i];
        }
    }
    return add_up;
}
/*******************************************************************************
**
** Function         btui_fm_cback
**
** Description      process the events from FM
**
**
** Returns          void
*******************************************************************************/
void btui_fm_cback(tBTA_FM_EVT event, tBTA_FM *p_data)
{
    tBTUI_FM_RDS_EVT *p_event_msg;

    LOGI("BTAPP_FM: btui_fm_cback: %i", event);

    /* Encode the events for transport via BTL-IF. */

    switch (event) {
        case BTA_FM_ENABLE_EVT:
            // wait 300ms for the FM engine to stabilize in stress situation
            //usleep(300000);
            usleep(150000);  // Decreased the wait to 150 ms during FM Enable
            LOGI("BTAPP_FM: BTA_FM_ENABLE_EVT");
            {
               tBTUI_FM_Params     fm_params;

               memcpy(&fm_params, &(btui_fm_cb.parms), sizeof(tBTUI_FM_Params));
               memset(&btui_fm_cb, 0, sizeof(tBTUI_FM_CB));
               memset(&btui_rds_cb, 0, sizeof(tBTUI_RDS_CB));

               /* The following fields should not be cleared after FM is turned off. */
               memcpy(&(btui_fm_cb.parms), &fm_params, sizeof(tBTUI_FM_Params));
            }

            if (btui_cfg.fm_snr_thresh != FM_SNR_INVALID)
            {
                tBTA_FM_CVALUE criteria;
                criteria.snr = btui_cfg.fm_snr_thresh;
                if (BTA_FmSetSchCriteria(BTA_FM_CTYPE_SNR, &criteria) != BTA_FM_OK) {
                    APPL_TRACE_ERROR0("btapp_fm_init: write SNR criteria failed");
                }
                LOGI("BTAPP_FM: set fm_snr to 0x%x", criteria.snr);
            }

            // send through bte_appl task
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_ENABLE_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->st.status = p_data->status;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
            break;

        case BTA_FM_DISABLE_EVT:
            LOGI("BTAPP_FM: BTA_FM_DISABLE_EVT");
            usleep(300000);

            {
               tBTUI_FM_Params     fm_params;

               memcpy(&fm_params, &(btui_fm_cb.parms), sizeof(tBTUI_FM_Params));
               memset(&btui_fm_cb, 0, sizeof(tBTUI_FM_CB));
               memset(&btui_rds_cb, 0, sizeof(tBTUI_RDS_CB));

               /* The following fields should not be cleared after FM is turned off. */
               memcpy(&(btui_fm_cb.parms), &fm_params, sizeof(tBTUI_FM_Params));
            }

            // send through bte_appl task
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_DISABLE_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->st.status = p_data->status;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;
        case BTA_FM_AF_JMP_EVT:

            LOGI("BTAPP_FM: BTA_FM_AF_JMP_EVT status = %i rssi = %i freq = %i ",(int)p_data->chnl_info.status,(int)p_data->chnl_info.rssi ,(int)p_data->chnl_info.freq );

            /* keep track of current radio frequency as required by AF */
            btui_fm_cb.cur_freq = p_data->chnl_info.freq;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_AF_JMP_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->af_jmp.status = p_data->chnl_info.status;
              p_event_msg->af_jmp.freq = p_data->chnl_info.freq;
              p_event_msg->af_jmp.rssi = p_data->chnl_info.rssi;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

        break;
        case BTA_FM_TUNE_EVT:
            LOGI("BTAPP_FM: BTA_FM_TUNE_EVT");

            search_in_porgress = FALSE;
            btapp_fm_clear_af_info();

            if (btui_fm_cb.rds_on)
            {

                // Reset the RDS engine via bte_appl task
                bta_fm_co_reset_rds_engine();
            }

            /* keep track of current radio frequency as required by AF for example */
            btui_fm_cb.cur_freq = p_data->chnl_info.freq;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_TUNE_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->tune.status = p_data->chnl_info.status;
              p_event_msg->tune.freq = p_data->chnl_info.freq;
              p_event_msg->tune.rssi = p_data->chnl_info.rssi;
              p_event_msg->tune.snr = p_data->chnl_info.snr;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }


            btui_fm_cb.init_cfg = FALSE;
            break;

    case BTA_FM_SEARCH_CMPL_EVT:
            LOGI("BTAPP_FM: BTA_FM_SEARCH_CMPL_EVT");
#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
            LOGI("btapp_fm_volume_control = %d", FMVolumeData);
#endif
            search_in_porgress = FALSE;

            btapp_fm_clear_af_info();

            if (btui_fm_cb.rds_on)
            {

              // Call the RDS engine via bt_appl task
               bta_fm_co_reset_rds_engine();
            }

            /* keep track of current radio frequency as required by AF for example */
            btui_fm_cb.cur_freq = p_data->chnl_info.freq;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_SEARCH_CMPL_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->search_cmpl.status = p_data->chnl_info.status;
              p_event_msg->search_cmpl.freq = p_data->chnl_info.freq;
              p_event_msg->search_cmpl.rssi = p_data->chnl_info.rssi;
              p_event_msg->search_cmpl.snr = p_data->chnl_info.snr;
              LOGI("BTUI_FM_SEARCH_CMPL_EVT freq:%d rssi:%d snr:%d", p_event_msg->search_cmpl.freq,
                p_event_msg->search_cmpl.rssi, p_event_msg->search_cmpl.snr);
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

           break;
    case BTA_FM_SEARCH_EVT:
            LOGI("BTAPP_FM: BTA_FM_SEARCH_EVT");

            search_in_porgress = FALSE;
            btapp_fm_clear_af_info();

            /* keep track of current radio frequency as required by AF for example */
            btui_fm_cb.cur_freq = p_data->chnl_info.freq;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_SEARCH_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->search.freq = p_data->scan_data.freq;
              p_event_msg->search.rssi = p_data->scan_data.rssi;
              p_event_msg->search.snr = p_data->scan_data.snr;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;

    case BTA_FM_AUD_MODE_EVT:
            LOGI("BTAPP_FM: BTA_FM_AUD_MODE_EVT");

            btui_fm_cb.mode = p_data->mode_info.audio_mode;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_AUDIO_MODE_EVT;
              p_event_msg->hdr.offset = 0;
              p_event_msg->audio_mode.status = p_data->mode_info.status;
              p_event_msg->audio_mode.audio_mode = p_data->mode_info.audio_mode;
              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;

    case BTA_FM_RDS_UPD_EVT:
            /* Only transmit events that have valid data. */
           if (NULL != p_data) {
                LOGI("BTAPP_FM: BTA_FM_RDS_UPD_EVT");

               if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
               {
                  p_event_msg->hdr.event            = BTUI_FM_EVENT;
                  p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
                  p_event_msg->hdr.layer_specific   = BTUI_FM_RDS_UPD_EVT;
                  p_event_msg->hdr.offset = 0;
                  p_event_msg->rds_upd.status = p_data->rds_update.status;
                  p_event_msg->rds_upd.data = p_data->rds_update.data;
                  p_event_msg->rds_upd.index = p_data->rds_update.index;
                  memcpy(p_event_msg->rds_upd.s1, p_data->rds_update.text, 65);
                  GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

               }

            } else {
                LOGI("BTAPP_FM: BTA_FM_RDS_UPD_EVT FAILED!!! (DATA == NULL)");
            }

          break;

    case BTA_FM_AUD_DATA_EVT:
            LOGI("BTAPP_FM: BTA_FM_AUD_DATA_EVT SNR:%d", p_data->audio_data.snr);
            if (search_in_porgress == FALSE)
            {
               if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
               {
                  p_event_msg->hdr.event            = BTUI_FM_EVENT;
                  p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
                  p_event_msg->hdr.layer_specific   = BTUI_FM_AUDIO_DATA_EVT;
                  p_event_msg->hdr.offset = 0;
                  p_event_msg->audio_data.status = p_data->audio_data.status;
                  p_event_msg->audio_data.rssi = p_data->audio_data.rssi;
                  p_event_msg->audio_data.snr= p_data->audio_data.snr;
                  p_event_msg->audio_data.audio_mode = p_data->audio_data.audio_mode;
                  GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

               }
            }
            break;

    case BTA_FM_AUD_PATH_EVT:
            LOGI("btui_fm_cback(BTA_FM_AUD_PATH_EVT, bta audio_path: %d, status: %d, btapp_audio_path: %d",
                    p_data->path_info.audio_path, p_data->path_info.status, save_audio_path );

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_AUDIO_PATH_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->audio_path.status = p_data->status;
               p_event_msg->audio_path.audio_path = save_audio_path;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        break;

    case BTA_FM_RDS_MODE_EVT:
            LOGI("BTAPP_FM: BTA_FM_RDS_MODE_EVT (status=%d) RDS=%s, AF=%s",
                (UINT8)(p_data->rds_mode.status),
                p_data->rds_mode.rds_on?"TRUE":"FALSE",
                p_data->rds_mode.af_on?"TRUE":"FALSE");

            btui_fm_cb.af_cb.af_on  = p_data->rds_mode.af_on;
            if (btui_fm_cb.rds_on != p_data->rds_mode.rds_on)
            {
                //rds_change = TRUE;
                btui_fm_cb.rds_on       = p_data->rds_mode.rds_on;
                btui_rds_cb.ps_on        = btui_rds_cb.pty_on = btui_rds_cb.ptyn_on = btui_rds_cb.rt_on = FALSE;
            }

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_RDS_MODE_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->rds_af_mode.status = p_data->rds_mode.status;
               p_event_msg->rds_af_mode.rds_on = (BOOLEAN)p_data->rds_mode.rds_on;
               p_event_msg->rds_af_mode.af_on = (BOOLEAN)p_data->rds_mode.af_on;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
            break;

    case BTA_FM_SET_DEEMPH_EVT:
            LOGI("BTAPP_FM: BTA_FM_SET_DEEMPH_EVT");
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_DEEMPH_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->deemphasis.status = p_data->deemphasis.status;
               p_event_msg->deemphasis.time_const = p_data->deemphasis.time_const;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;
    case BTA_FM_MUTE_AUD_EVT:
            LOGI("BTAPP_FM: BTA_FM_MUTE_AUD_EVT");

            btui_fm_cb.audio_mute = p_data->mute_stat.is_mute;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_MUTE_AUDIO_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->mute_stat.status = p_data->mute_stat.status;
               p_event_msg->mute_stat.is_mute = p_data->mute_stat.is_mute;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }


            break;
    case BTA_FM_SCAN_STEP_EVT:
            LOGI("BTAPP_FM: BTA_FM_SCAN_STEP_EVT");

            btui_fm_cb.scan_step = p_data->scan_step ;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SCAN_STEP_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->scan_step.scan_step = p_data->scan_step;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;
        case BTA_FM_SET_REGION_EVT:
            LOGI("BTAPP_FM: BTA_FM_SET_REGION_EVT");

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_REGION_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->region_info.status = p_data->region_info.status;
               p_event_msg->region_info.region = p_data->region_info.region;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }

            break;

        case BTA_FM_NFL_EVT:
            LOGI("BTAPP_FM: BTA_FM_NFL_EVT");
            LOGI("BTAPP_FM: NFL = %d", (int)p_data->nfloor.rssi);

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_NFL_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->nfloor.rssi = p_data->nfloor.rssi;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
            break;

    case BTA_FM_RDS_TYPE_EVT:
            LOGI("BTAPP_FM: BTA_FM_RDS_TYPE_EVT (status=%d) RDS_TYPE=0x%x",
                (UINT8)(p_data->rds_type.status),
                (UINT8)(p_data->rds_type.type));

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_RDS_TYPE_EVT;
               p_event_msg->hdr.offset = 0;
               p_event_msg->rds_type.status = p_data->rds_type.status;
               p_event_msg->rds_type.type = p_data->rds_type.type;
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
            break;

    case BTA_FM_CFG_BLEND_MUTE_EVT:
        APPL_TRACE_DEBUG1("Config Stere/Mono blend and Soft mute Params %s",
            (p_data->status == BTA_FM_OK) ? "Succeed" : "Failed");
        break;

    case BTA_FM_VOLUME_EVT:
        APPL_TRACE_EVENT2( "btui_fm_cback( BTA_FM_VOLUME_EVT, status: %d, volume: %d )",
                             p_data->volume.status, p_data->volume.volume );
        if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tFM_VOLUME_EVT))) != NULL)
        {
           p_event_msg->hdr.event            = BTUI_FM_EVENT;
           p_event_msg->hdr.len              = sizeof(tFM_VOLUME_EVT) - BT_HDR_SIZE;
           p_event_msg->hdr.layer_specific   = BTUI_FM_VOLUME_EVT;
           p_event_msg->hdr.offset = 0;
           p_event_msg->volume_evt.vol.status = p_data->volume.status;
           p_event_msg->volume_evt.vol.volume = p_data->volume.volume;
           GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
        }
        break;

       default:
            LOGI("BTAPP_FM: DEFAULT EVENT RECEIVED (%d)", event);
            break;
    }
}


/*******************************************************************************
**
** Function         btapp_fm_rdsp_cback
**
** Description      RDS decoder call back function.
**
**
** Returns          void
*******************************************************************************/
void btapp_fm_rdsp_cback(tBTA_RDS_EVT event, tBTA_FM_RDS *p_data, UINT8 app_id)
{

    tBTUI_BTA_MSG * p_event_msg;
    UINT8   i = 0;
    BOOLEAN new_af = TRUE;

    switch(event)
    {
    case BTA_RDS_REG_EVT:
        APPL_TRACE_DEBUG1("Client register status: %d", p_data->status);
        break;
    case BTA_RDS_PI_EVT:
        LOGI("BTA_RDS_PI_EVT received");
        btui_fm_cb.af_cb.pi_code = p_data->pi_code;

        APPL_TRACE_DEBUG1("BTA_RDS_PI_EVT pi_code: %d", p_data->pi_code);
        break;
    case BTA_RDS_AF_EVT:
		LOGI("BTA_RDS_AF_EVT received in %s, tn_freq = %d, num_af = %d",
            (p_data->af_data.method == BTA_RDS_AF_M_B)? "Method-B" :"Method-A",
            p_data->af_data.list.af_list[0],
            btui_fm_cb.af_cb.af_list.num_af);

         if (btui_fm_cb.af_cb.af_on)
         {
            new_af = TRUE; /* needs to be true handle first time setup of AF */

            if (p_data->af_data.method == BTA_RDS_AF_M_B)
            {
               if (p_data->af_data.list.af_list[0] == btui_fm_cb.cur_freq)
                   new_af = btapp_fm_af_merge_list(&p_data->af_data.list);
               else
                /* Application can choose to remember other tuned frequency AF list */
                /* btui_app, other TN af list, skip */
                  new_af = FALSE;
            }
            else
               memcpy(&btui_fm_cb.af_cb.af_list, &p_data->af_data.list, sizeof(tBTA_FM_AF_LIST));
            if (new_af)
            {
                btui_fm_cb.af_cb.af_avail = TRUE;
               /* when a new AF arrives, update AF list by calling set RDS mode */
               if (btui_fm_cb.af_cb.af_on)
               {
				LOGI("BTA_RDS_AF_EVT received : call btapp_fm_set_rds_mode(TRUE, TRUE)");
                  btapp_fm_set_rds_mode(TRUE, TRUE);
               }
            }

            i = 0;

            while(i < btui_fm_cb.af_cb.af_list.num_af)
            {
               APPL_TRACE_DEBUG2(">>>>>>>>>>>>>>>>>>>>>>>>>AF[%d] [%d]", i+1,
                   btui_fm_cb.af_cb.af_list.af_list[i ++]);
            }
        }
        break;
    case BTA_RDS_PS_EVT:
	    {
			LOGI("BTA_RDS_PS_EVT received");
            /* Reformat for transport to app via standard callback event. */
            tBTA_FM rds_event_data;

            rds_event_data.rds_update.status = BTA_FM_OK;
            rds_event_data.rds_update.data = BTA_RDS_PS_EVT;
            rds_event_data.rds_update.index = 0;
            memcpy(rds_event_data.rds_update.text, p_data->p_data, BTUI_RDS_STR_LEN_8+1);

            btui_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
	       }
		break;
    case BTA_RDS_TP_EVT:
        LOGI("BTA_RDS_TP_EVT received");
        break;

    case BTA_RDS_PTY_EVT:
        {
             LOGI("BTA_RDS_TPY_EVT received");
            /* Reformat for transport to app via standard callback event. */
            tBTA_FM rds_event_data;

            rds_event_data.rds_update.status = BTA_FM_OK;
            rds_event_data.rds_update.data = BTA_RDS_PTY_EVT;
            rds_event_data.rds_update.index = p_data->pty.pty_val;
            memcpy(rds_event_data.rds_update.text, p_data->pty.p_str, BTUI_RDS_STR_LEN_8+1);

            btui_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
        }
        break;

    case BTA_RDS_PTYN_EVT:
        {
             LOGI("BTA_RDS_TPYN_EVT received");
            /* Reformat for transport to app via standard callback event. */
            tBTA_FM rds_event_data;

            rds_event_data.rds_update.status = BTA_FM_OK;
            rds_event_data.rds_update.data = BTA_RDS_PTYN_EVT;
            rds_event_data.rds_update.index = 0;
            memcpy(rds_event_data.rds_update.text, p_data->p_data, BTUI_RDS_STR_LEN_8+1);

            btui_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
        }
        break;

    case BTA_RDS_RT_EVT:
        LOGI("BTA_RDS_RT_EVT received");
        if (p_data->rt.complete != FALSE) {
            /* Reformat for transport to app via standard callback event. */
            tBTA_FM rds_event_data;

            rds_event_data.rds_update.status = BTA_FM_OK;
            rds_event_data.rds_update.data = BTA_RDS_RT_EVT;
            rds_event_data.rds_update.index = p_data->rt.g_type;
            memcpy(rds_event_data.rds_update.text, p_data->rt.p_data, BTUI_RDS_STR_LEN_64+1);

            btui_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
        }
        break;
    default:
        APPL_TRACE_DEBUG1("RDS register status: %d", event);

        break;
    }
}

/* direct DAC register access for volume control
 * WARNING this depends on CHIP HW! Does NOT work on 2075Cx, 20780x!
 */
#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
static void hci_SetVolumeDataLowByte_cback(tBTM_VSC_CMPL *p)
{
    tBTA_FM_VOLUME vol;
    /* if it is not completed */
    if (SetVolume_CB_Needed)
    {
        vol.volume = FMVolumeData;
    if (p->p_param_buf[0] == 0)
    {

        btui_fm_cb.volume= FMVolumeData;

        LOGI("BTAPP_FM: hci_SetVolumeDataLowByte_cback volume =%d",(UINT16)FMVolumeData);
            /* tell that volume is not supported. it may still do something in the event handler based
             * on audiopath knowledge
             */
            vol.status = BTA_FM_OK;
    }
    else
    {
        LOGI("BTAPP_FM: hci_SetVolumeDataLowByte_cback status = %d volume =%d",BTA_FM_ERR,FMVolumeData);
            vol.status = BTA_FM_ERR;
    }
        /* call usual callback for volume event result! */
        btui_fm_cback(BTA_FM_VOLUME_EVT, (tBTA_FM *)&vol);
    }
}


static void hci_SetVolumeDataHighByte_cback(tBTM_VSC_CMPL *p)
{
    tBTM_STATUS result;
    tBTA_FM_VOLUME vol;
    UINT8  hci_SetVolumedatalowbit[HCI_BRCM_SUPER_PEEK_POKE_LENGTH] =
    {
        0x05,0xE0,0x41,0x0F,0x00,0x00,0x00,0x00,0x00
    };


    /* if it is not completed */
    if (p->p_param_buf[0] == 0)
    {
            hci_SetVolumedatalowbit[5]= FMVolumeData & 0x00FF;

            result = BTM_VendorSpecificCommand( HCI_BRCM_SUPER_PEEK_POKE,
                                        HCI_BRCM_SUPER_PEEK_POKE_LENGTH,(UINT8 *)hci_SetVolumedatalowbit,
                                        (tBTM_CMPL_CB*) hci_SetVolumeDataLowByte_cback);
	     LOGI("BTAPP_FM: hci_SetVolumeDataHighByte_cback result = %d", result);
        return;
    }
    else if (SetVolume_CB_Needed)
    {
        LOGI("BTAPP_FM: hci_SetVolumeDataHighByte_cback status = %d volume =%d",BTA_FM_ERR,FMVolumeData);
        vol.status = BTA_FM_ERR;
        vol.volume = FMVolumeData;
        /* call usual callback for volume event result! */
        btui_fm_cback(BTA_FM_VOLUME_EVT, (tBTA_FM *)&vol);
    }
}

static void hci_SuperPeekPoke_cback(tBTM_VSC_CMPL *p)
{
    tBTM_STATUS result;
    tBTA_FM_VOLUME vol;
    UINT8  hci_SetVolumedatahighbyte[HCI_BRCM_SUPER_PEEK_POKE_LENGTH] =
    {
        0x05,0xE4,0x41,0x0F,0x00,0x00,0x00,0x00,0x00
    };

    /* if it is not completed */
    if (p->p_param_buf[0] == 0)
    {

            hci_SetVolumedatahighbyte[5]= FMVolumeData >> 8;
            result = BTM_VendorSpecificCommand( HCI_BRCM_SUPER_PEEK_POKE,
                                        HCI_BRCM_SUPER_PEEK_POKE_LENGTH,(UINT8 *)hci_SetVolumedatahighbyte,
                                        (tBTM_CMPL_CB*) hci_SetVolumeDataHighByte_cback);
	     LOGI("BTAPP_FM: hci_SuperPeekPoke_cback result = %d", result);

        return;
    }
    else if (SetVolume_CB_Needed)
    {
        LOGI("BTAPP_FM: hci_SuperPeekPoke_cback status = %d volume =%d",BTA_FM_ERR,FMVolumeData);
        vol.status = BTA_FM_ERR;
        vol.volume = FMVolumeData;
        /* call usual callback for volume event result! */
        btui_fm_cback(BTA_FM_VOLUME_EVT, (tBTA_FM *)&vol);
    }

}
#endif

#if (BTAPP_FM_AUDIO_PATH==BTA_FM_AUDIO_I2S) || (BTAPP_FM_SINGLE_PATH_ONLY==FALSE)
/* sets the volume on the host side. This is typically needed in case of i2s or FM over BT as
 * bt chip does NO scalling on digital stream (yet).
 */
static BOOLEAN btapp_fm_set_host_volume( tBTAPP_FM_PLAT_CB *p_cb, const int volume )
{
    int fd, err, num_bytes, vol;
    char tmp_buf[MAX_AUDIO_STR_LEN]; /* 2 times to leave enough room */
    char *p;
    BOOLEAN ret_val = FALSE;

    APPL_TRACE_API1( "btapp_fm_set_host_volume(new volume: %d)", volume );

// Example Platform-specific code.
#if 0
    if ((fd = open (p_cb->fs_control_path, (O_RDWR|O_NONBLOCK), 0666)) >= 0)
    {
        if ( (num_bytes = read( fd, tmp_buf, MAX_AUDIO_STR_LEN)) >= 0 )
        {
            tmp_buf[num_bytes] = '\0';

            p = &tmp_buf[VOLUME_IDX];
            APPL_TRACE_DEBUG2( "btapp_fm_set_host_volume(): num read: %d, current host volume: <%s>",
                               num_bytes, p );
            /* to get nicer rounding add HOST_MAX_VOLUME-1 and write it back to the read audio
             * settings pointed by p. sprintf() would be more portable ;-)*/
            vol = ((HOST_MAX_VOLUME*volume)+(HOST_MAX_VOLUME-1))/BTA_FM_VOLUME_MAX;
            *p = (vol<10)?(vol+'0'):(vol/10+'0');
            *(p+1) = (vol<10)?'\0':(vol%10+'0');
            *(p+2) = '\0';
            num_bytes = strlen(tmp_buf)+1;  /* +1 for the 0 */
            APPL_TRACE_DEBUG2( "btapp_fm_set_host_volume(): NEW audio settings: %s, lenght: %d",
                               tmp_buf, num_bytes );

            if ( (num_bytes = write( fd, tmp_buf, num_bytes )) >= 0 )
            {
                ret_val = TRUE;
            }
            else
            {
                err = errno;
                APPL_TRACE_WARNING2( "btapp_fm_set_host_volume(): write failed with error %d for % bytes",
                                     err, num_bytes );
            }
        }
        if ( close(fd) < 0 )
        {
            err = errno;
            APPL_TRACE_WARNING1( "btapp_fm_set_host_volume(): close failed with error %d", err );
        }
    }
    else
    {
        err = errno;
        APPL_TRACE_WARNING2( "btapp_fm_set_host_volume():open failed with error %d for path %s",
                             err, p_cb->fs_control_path);
    }
#endif
    return ret_val;
} /* btapp_fm_set_host_volume() */
#endif

/*******************************************************************************
**
** Function         btapp_fm_on_rx_ctrl
**
** Description      Decode and execute incoming FM commands from application.
**
** Returns          void
**
*******************************************************************************/
void btapp_fm_on_rx_ctrl(tCTRL_HANDLE fd, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params)
{
    /* Update handle to app using returned handle. */
    btl_if_fm_handle = fd;
    tBTUI_FM_RDS_EVT *p_event_msg;

    switch (id)
    {
    case BTLIF_FM_ENABLE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmEnable() */
            tBTA_FM simulated_fm_event_data;

            GKI_delay(1000);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmEnable");

            simulated_fm_event_data.status = BTA_FM_OK;

            btui_fm_cback(BTA_FM_ENABLE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmEnable");

#if (BTAPP_FM_USE_HW_POKE_VOLUME==TRUE)
            FMVolumeData = 0;
            SetVolume_CB_Needed = 0;
            //Removing the volume poke command before BTA_FmEnable()
            /*BTM_VendorSpecificCommand( HCI_BRCM_SUPER_PEEK_POKE,
                                        HCI_BRCM_SUPER_PEEK_POKE_LENGTH,(UINT8 *)hci_SetVolumePeekPokedata,
                                        (tBTM_CMPL_CB*) hci_SuperPeekPoke_cback);
            GKI_delay(1000);*/
#endif
            /* Enable FM and register callback */

            BTA_FmEnable((UINT16)(params->fm_I_param.i1), btui_fm_cback, BTUI_RDS_APP_ID);
        }
#endif
        break;

    case BTLIF_FM_DISABLE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmDisable() */
            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmDisable");

            simulated_fm_event_data.status = BTA_FM_OK;

            btui_fm_cback(BTA_FM_DISABLE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmDisable");
			BTA_FmConfigAudioPath(BTA_FM_AUDIO_NONE);

            /* Disable FM */
            memset(&btui_fm_cb, 0, sizeof(tBTUI_FM_CB));
            memset(&btui_rds_cb, 0, sizeof(tBTUI_RDS_CB));

            BTA_FmDisable();
        }
#endif
        break;

    case BTLIF_FM_TUNE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmTuneFreq() */
            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmTuneFreq");

            simulated_fm_event_data.chnl_info.status = BTA_FM_OK;
            simulated_fm_event_data.chnl_info.rssi = 85;
            simulated_fm_event_data.chnl_info.freq = (UINT16)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_TUNE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmTuneFreq");

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               search_in_porgress = TRUE;
               /* Tune Frequency */
               btui_rds_cb.ps_on = btui_rds_cb.pty_on = btui_rds_cb.ptyn_on = btui_rds_cb.rt_on = FALSE;

               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_TUNE_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->tune_cmd.freq = (UINT16)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
#endif
        break;

    case BTLIF_FM_MUTE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmMute() */
            tBTA_FM simulated_fm_event_data;

            GKI_delay(500);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmMute");

            simulated_fm_event_data.mute_stat.status = BTA_FM_OK;
            simulated_fm_event_data.mute_stat.is_mute = (BOOLEAN)(params->fm_Z_param.z1);

            btui_fm_cback(BTA_FM_MUTE_AUD_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmMute");

            /* Mute Audio */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_MUTE_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->mute_cmd.mute = (BOOLEAN)(params->fm_Z_param.z1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
#endif
        break;

    case BTLIF_FM_SEARCH:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSearchFreq() */
            tBTA_FM simulated_fm_event_data_1;
            tBTA_FM simulated_fm_event_data_2;

            GKI_delay(1000);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSearchFreq");

            simulated_fm_event_data_1.scan_data.rssi = (UINT8) 85;
            simulated_fm_event_data_1.scan_data.freq = (UINT16) 10000;

            btui_fm_cback(BTA_FM_SEARCH_EVT, (tBTA_FM *)&simulated_fm_event_data_1);

            GKI_delay(500);

            simulated_fm_event_data_2.chnl_info.status = BTA_FM_OK;
            simulated_fm_event_data_2.chnl_info.rssi = (UINT8) 100;
            simulated_fm_event_data_2.chnl_info.freq = (UINT16) 10800;

            btui_fm_cback(BTA_FM_SEARCH_CMPL_EVT, (tBTA_FM *)&simulated_fm_event_data_2);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSearchFreq");

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               search_in_porgress = TRUE;
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SEARCH_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->search_cmd.pending_search_scan_mode = (tBTA_FM_SCAN_MODE)(params->fm_IIII_param.i1);
               p_event_msg->search_cmd.pending_search_rssi = (UINT8)(params->fm_IIII_param.i2);
               p_event_msg->search_cmd.pending_search_cond_type = (tBTA_FM_SCH_RDS_TYPE)(params->fm_IIII_param.i3);
               p_event_msg->search_cmd.pending_search_cond_val = (UINT8)(params->fm_IIII_param.i4);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
#endif
        break;

    case BTLIF_FM_COMBO_SEARCH:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmComboSearch() */
            tBTA_FM simulated_fm_event_data_1;
            tBTA_FM simulated_fm_event_data_2;

            GKI_delay(1000);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmComboSearch");

            simulated_fm_event_data_1.scan_data.rssi = (UINT8) 85;
            simulated_fm_event_data_1.scan_data.freq = (UINT16) 10000;

            btui_fm_cback(BTA_FM_SEARCH_EVT, (tBTA_FM *)&simulated_fm_event_data_1);

            GKI_delay(500);

            simulated_fm_event_data_2.chnl_info.status = BTA_FM_OK;
            simulated_fm_event_data_2.chnl_info.rssi = (UINT8) 100;
            simulated_fm_event_data_2.chnl_info.freq = (UINT16) 10800;

            btui_fm_cback(BTA_FM_SEARCH_CMPL_EVT, (tBTA_FM *)&simulated_fm_event_data_2);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmComboSearch");

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               search_in_porgress = TRUE;
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_COMBO_SEARCH_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->combo_search_cmd.pending_combo_search_start_freq = (UINT16)(params->fm_IIIIIZII_param.i1);
               p_event_msg->combo_search_cmd.pending_combo_search_end_freq = (UINT16)(params->fm_IIIIIZII_param.i2);
               p_event_msg->combo_search_cmd.pending_combo_search_rssi = (UINT8)(params->fm_IIIIIZII_param.i3);
               p_event_msg->combo_search_cmd.pending_combo_search_direction = (UINT8)(params->fm_IIIIIZII_param.i4);
               p_event_msg->combo_search_cmd.pending_combo_search_scan_method = (UINT8)(params->fm_IIIIIZII_param.i5);
               p_event_msg->combo_search_cmd.pending_combo_search_multi_channel = (BOOLEAN)(params->fm_IIIIIZII_param.z1);
               p_event_msg->combo_search_cmd.pending_combo_search_cond_type = (tBTA_FM_SCH_RDS_TYPE)(params->fm_IIIIIZII_param.i6);
               p_event_msg->combo_search_cmd.pending_combo_search_cond_val = (UINT8)(params->fm_IIIIIZII_param.i7);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_SEARCH_ABORT:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSearchAbort() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSearchAbort");

            simulated_fm_event_data.chnl_info.status = BTA_FM_SCAN_ABORT;
            simulated_fm_event_data.chnl_info.rssi = (UINT8) 20;
            simulated_fm_event_data.chnl_info.freq = (UINT16) 10000;

            btui_fm_cback(BTA_FM_SEARCH_CMPL_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSearchAbort");

            /* Ensure that no pending search is already started.
               This case will result in a spurious NFL event to application! */
            pending_search = TRUE;
            search_in_porgress = FALSE;

            /* Abort search for station. */
            BTA_FmSearchAbort();
        }
#endif
        break;

    case BTLIF_FM_SET_RDS_MODE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetRDSMode() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetRDSMode");

            simulated_fm_event_data.rds_mode.status = BTA_FM_OK;
            simulated_fm_event_data.rds_mode.rds_on = (BOOLEAN)(params->fm_ZZ_param.z1);
            simulated_fm_event_data.rds_mode.af_on = (BOOLEAN)(params->fm_ZZ_param.z2);

            btui_fm_cback(BTA_FM_RDS_MODE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSetRDSMode");

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {

               btui_fm_cb.rds_on = (BOOLEAN)(params->fm_ZZ_param.z1);
               btui_fm_cb.af_cb.af_on = (BOOLEAN)(params->fm_ZZ_param.z2);

               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_RDS_MODE_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->rds_mode_cmd.rds_on = (BOOLEAN)(params->fm_ZZ_param.z1);
               p_event_msg->rds_mode_cmd.af_on = (BOOLEAN)(params->fm_ZZ_param.z2);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }

#endif
        break;

    case BTLIF_FM_SET_RDS_RBDS:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetRdsRbds() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetRdsRbds");

            simulated_fm_event_data.rds_type.status = BTA_FM_OK;
            simulated_fm_event_data.rds_type.type = (tBTA_FM_RDS_B)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_RDS_TYPE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG1("BTAPP_FM: BTA_FmSetRdsRbds = %d",params->fm_I_param.i1);

            /* Set RDS or RBDS. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_RDS_RBDS_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->rds_rbds_cmd.rds_type = (tBTA_FM_RDS_B)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
#endif
        break;

    case BTLIF_FM_AUDIO_MODE:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetAudioMode() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetAudioMode");

            simulated_fm_event_data.mode_info.status = BTA_FM_OK;
            simulated_fm_event_data.mode_info.audio_mode = (tBTA_FM_AUDIO_MODE)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_AUD_MODE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSetAudioMode");

            /* Set Mono, Stereo, Blend mode etc. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_AUDIO_MODE_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->rds_rbds_cmd.rds_type = (tBTA_FM_AUDIO_MODE)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
#endif
        break;

    case BTLIF_FM_AUDIO_PATH:

#if ( BTAPP_FM_SUPPORT_AUDIOPATH==TRUE )
#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetAudioPath() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetAudioPath");

            simulated_fm_event_data.path_info.status = BTA_FM_OK;
            simulated_fm_event_data.path_info.audio_path = (tBTA_FM_AUDIO_PATH)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_AUD_PATH_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {


             APPL_TRACE_DEBUG1( "BTAPP_FM: BTLIF_FM_AUDIO_PATH::audio_path: %d",
                             (tBTA_FM_AUDIO_PATH)(params->fm_I_param.i1));

            /* Set Audio path to None, Speaker, Speaker, wired headset or I2S path. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_AUDIO_PATH_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->audio_path_cmd.audio_path = (tBTA_FM_AUDIO_PATH)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
               save_audio_path = params->fm_I_param.i1;

            }
        }
#endif
#endif /* #if ( BTAPP_FM_SUPPORT_AUDIOPATH==TRUE ) */
        break;

    case BTLIF_FM_SCAN_STEP:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetScanStep() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetScanStep");

            simulated_fm_event_data.scan_step = (tBTA_FM_STEP_TYPE)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_SCAN_STEP_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSetScanStep");

            /* Set ScanStep. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_SCAN_STEP_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->scan_step_cmd.scan_step = (tBTA_FM_STEP_TYPE)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_SET_REGION:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetRegion() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetRegion");

            simulated_fm_event_data.region_info.status = BTA_FM_OK;
            simulated_fm_event_data.region_info.region = (tBTA_FM_REGION_CODE)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_SET_REGION_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSetRegion");

            /* Set Region. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_REGION_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->region_cmd.region = (tBTA_FM_REGION_CODE)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_CONFIGURE_DEEMPHASIS:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmConfigDeemphasis() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmConfigDeemphasis");

            simulated_fm_event_data.deemphasis.status = BTA_FM_OK;
            simulated_fm_event_data.deemphasis.time_const = (tBTA_FM_DEEMPHA_TIME)(params->fm_I_param.i1);

            btui_fm_cback(BTA_FM_SET_DEEMPH_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmConfigDeemphasis");

            /* Set Deemphasis time. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_DEEMPH_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->deemph_cmd.deemphasis = (tBTA_FM_DEEMPHA_TIME)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_ESTIMATE_NFL:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmEstNoiseFloor() */

            tBTA_FM simulated_fm_event_data;

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmEstNoiseFloor");

            simulated_fm_event_data.nfloor = (UINT8)(85);

            btui_fm_cback(BTA_FM_NFL_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmEstNoiseFloor");

            /* Estimate NFL. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_NFL_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->nfloor_cmd.nfloor = (tBTA_FM_NFE_LEVL)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_GET_AUDIO_QUALITY:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmReadAudioQuality() */

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmReadAudioQuality");
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmReadAudioQuality");

            /* Enable live audio quality sampling. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_RD_AUDIO_QUALITY_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->audio_quality_cmd.turn_on = (BOOLEAN)(params->fm_Z_param.z1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION:

#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetSignalNotifSetting() */

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmSetSignalNotifSetting");
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: BTA_FmSetSignalNotifSetting");

            /* Set period interval of notifications. */
            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_SIGNAL_NOTIFICATION_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->signal_notif_cmd.notif = (INT32)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
        }
#endif
        break;

    case BTLIF_FM_SET_VOLUME:
#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmSetSignalNotifSetting() */

            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating btapp_fm_volume_control");
        }
#else
        APPL_TRACE_DEBUG1("BTAPP_FM: BTLIF_FM_SET_VOLUME: %d", params->fm_I_param.i1 );

        /* Send message for volume settings */
        if ((p_event_msg = (tFM_VOLUME_CMD *)GKI_getbuf(sizeof(tFM_VOLUME_CMD))) != NULL)
        {
           p_event_msg->hdr.event            = BTUI_FM_EVENT;
           p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
           p_event_msg->hdr.layer_specific   = BTUI_FM_VOLUME_CMD;
           p_event_msg->hdr.offset = 0;
           p_event_msg->volume_cmd.level = (UINT16)(params->fm_I_param.i1);
           GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
        }
#endif
        break;

        case BTLIF_FM_SET_SNR_THRES:
#ifdef BTAPP_FM_SIMULATION
            {
                /* For testing: simulate call to BTA_FmSetSchCriteria() */

                GKI_delay(100);
                APPL_TRACE_DEBUG0("BTAPP_FM: Simulating btapp_fm_set_search_criteria");
            }
#else
            APPL_TRACE_DEBUG1("BTAPP_FM: BTLIF_FM_SET_SNR_THRES snr: %d", params->fm_I_param.i1 );

            /* Send message for SNR Threshold  settings */
            if ((p_event_msg = (tFM_SET_SNR_THRES_CMD*)GKI_getbuf(sizeof(tFM_SET_SNR_THRES_CMD))) != NULL)
            {
               p_event_msg->hdr.event            = BTUI_FM_EVENT;
               p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
               p_event_msg->hdr.layer_specific   = BTUI_FM_SET_SNR_THRES_CMD;
               p_event_msg->hdr.offset = 0;
               p_event_msg->set_snr_cmd.snr= (UINT8)(params->fm_I_param.i1);
               GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);
            }
#endif
            break;

    case BTLIF_SUBSYSTEM_DETACHED:
#ifdef BTAPP_FM_SIMULATION
        {
            /* For testing: simulate call to BTA_FmDisable() */
            tBTA_FM simulated_fm_event_data;
            GKI_delay(100);
            APPL_TRACE_DEBUG0("BTAPP_FM: Simulating BTA_FmDisable for BTLIF_SUBSYSTEM_DETACHED");
            simulated_fm_event_data.status = BTA_FM_OK;
            btui_fm_cback(BTA_FM_DISABLE_EVT, (tBTA_FM *)&simulated_fm_event_data);
        }
#else
        {
            APPL_TRACE_DEBUG0("BTAPP_FM: Calling BTA_FmDisable for BTLIF_SUBSYSTEM_DETACHED case");
            /* Disable FM */
            memset(&btui_fm_cb, 0, sizeof(tBTUI_FM_CB));
            memset(&btui_rds_cb, 0, sizeof(tBTUI_RDS_CB));

            BTA_FmDisable();
        }
#endif
        break;

    default: APPL_TRACE_DEBUG1("BTAPP_FM: btapp_fm_on_rx_ctrl: UNKNOWN COMMAND (ID=%d) RECEIVED!",id);
        break;
    }
}

// Platform specific functions.
/* p_buf must be big enough to hold the init_audio_path. no check is done */
static int btapp_fm_write_audio_path( int fd, char * p_buf, tBTAPP_FM_PLAT_CB *p_cb, const int audio_path )
{
    return 0;
}


static BOOLEAN btapp_fm_set_host_audio_path( tBTAPP_FM_PLAT_CB *p_cb, const int audio_path )
{
    BOOLEAN ret_val = FALSE;

 return ret_val;
}

/*******************************************************************************
 **
 ** Function         BTAPP_FmHandleEvents
 **
 ** Description      Handles events comming from callback, call-outs and btl-if
 **
 ** Returns          BOOLEAN: true, free buffer, false
 **
 *******************************************************************************/
BOOLEAN BTAPP_FmHandleEvents( tBTUI_FM_RDS_EVT *p_msg )
{
    BOOLEAN free_buf = TRUE;

    switch (p_msg->hdr.layer_specific)
    {
    case BTUI_FM_TUNE_CMD:
        BTA_FmTuneFreq(p_msg->tune_cmd.freq);
        break;

    case BTUI_FM_MUTE_CMD:
        BTA_FmMute(p_msg->mute_cmd.mute);
        break;

    case BTUI_FM_SEARCH_CMD:
    {
        tBTA_FM_SCH_RDS_COND temp_cond;

        temp_cond.cond_type = p_msg->search_cmd.pending_search_cond_type;
        temp_cond.cond_val = p_msg->search_cmd.pending_search_cond_val;
        if ((0 == temp_cond.cond_type) && (0 == temp_cond.cond_val))
        {
            BTA_FmSearchFreq(p_msg->search_cmd.pending_search_scan_mode,
                    p_msg->search_cmd.pending_search_rssi, NULL);
        }
        else
        {
            BTA_FmSearchFreq(p_msg->search_cmd.pending_search_scan_mode,
                    p_msg->search_cmd.pending_search_rssi, (tBTA_FM_SCH_RDS_COND*) (&temp_cond));
        }
    }
        break;

    case BTUI_FM_COMBO_SEARCH_CMD:
    {
        tBTA_FM_SCH_RDS_COND temp_cond;

        temp_cond.cond_type = p_msg->combo_search_cmd.pending_combo_search_cond_type;
        temp_cond.cond_val = p_msg->combo_search_cmd.pending_combo_search_cond_val;

        if ((0 == temp_cond.cond_type) && (0 == temp_cond.cond_val))
        {
             BTA_FmComboSearch (p_msg->combo_search_cmd.pending_combo_search_start_freq,
				p_msg->combo_search_cmd.pending_combo_search_end_freq,
				p_msg->combo_search_cmd.pending_combo_search_rssi,
				p_msg->combo_search_cmd.pending_combo_search_direction,
				p_msg->combo_search_cmd.pending_combo_search_scan_method,
				p_msg->combo_search_cmd.pending_combo_search_multi_channel,
				NULL);
        }
        else
        {
             BTA_FmComboSearch (p_msg->combo_search_cmd.pending_combo_search_start_freq,
				p_msg->combo_search_cmd.pending_combo_search_end_freq,
				p_msg->combo_search_cmd.pending_combo_search_rssi,
				p_msg->combo_search_cmd.pending_combo_search_direction,
				p_msg->combo_search_cmd.pending_combo_search_scan_method,
				p_msg->combo_search_cmd.pending_combo_search_multi_channel,
				(tBTA_FM_SCH_RDS_COND*) (&temp_cond));
        }
    }
        break;

    case BTUI_FM_SET_RDS_MODE_CMD:
    {
        extern void btapp_fm_set_rds_mode(BOOLEAN rds_on, BOOLEAN af_on);

        btapp_fm_set_rds_mode(p_msg->rds_mode_cmd.rds_on, p_msg->rds_mode_cmd.af_on);
    }
        break;

    case BTUI_FM_SET_RDS_RBDS_CMD:
        BTA_FmSetRdsRbds(p_msg->rds_rbds_cmd.rds_type);
        break;

    case BTUI_FM_SET_AUDIO_MODE_CMD:
        BTA_FmSetAudioMode(p_msg->audio_mode_cmd.audio_mode);
        break;

    case BTUI_FM_SET_AUDIO_PATH_CMD:
#if ( BTAPP_FM_SUPPORT_AUDIOPATH==TRUE )
    {
        int                 desired_bta_audio_path = -1;
        BOOLEAN             b_cback = TRUE;         /* call audio callback directly */
        int                 audio_path      = p_msg->audio_path_cmd.audio_path;
        tBTAPP_FM_PLAT_CB   *p_cb = &btapp_fm_plat_cb;

        switch (audio_path)
        {
        case BTAPP_FM_AUDIO_PATH_NONE:
            desired_bta_audio_path = BTA_FM_AUDIO_NONE;
            break;
        case BTAPP_FM_AUDIO_PATH_SPEAKER:
        case BTAPP_FM_AUDIO_PATH_WIRED_HEADSET:
#if (BTAPP_FM_AUDIO_PATH==BTA_FM_AUDIO_DAC) || (BTAPP_FM_SINGLE_PATH_ONLY==FALSE)
            desired_bta_audio_path = BTA_FM_AUDIO_DAC;
            break;
#endif
        case BTAPP_FM_AUDIO_PATH_DIGITAL:
#if (BTAPP_FM_AUDIO_PATH==BTA_FM_AUDIO_I2S) || (BTAPP_FM_SINGLE_PATH_ONLY==FALSE)
            desired_bta_audio_path = BTA_FM_AUDIO_I2S;
#else
            APPL_TRACE_WARNING0( "BTUI_FM_SET_AUDIO_PATH_CMD:: not supported: BTA_FM_AUDIO_I2S!" );
#endif
            break;

        default:
            /* TODO: Add BTAPP_FM_AUDIO_BT_MONO, BTAPP_FM_AUDIO_BT_STEREO */
            APPL_TRACE_WARNING1( "BTUI_FM_SET_AUDIO_PATH_CMD:: unsupported path: %d!",
                                 audio_path );
            break;
        }

        // Platform-specific changes
        if ( (-1 != desired_bta_audio_path) && (p_cb->current_bta_path!=desired_bta_audio_path) )
        {
            p_cb->current_bta_path = desired_bta_audio_path;
            b_cback = FALSE;            /* BTA will callcback */
            BTA_FmConfigAudioPath((tBTA_FM_AUDIO_PATH)desired_bta_audio_path);
        }
#if (FALSE == BTAPP_FM_AUDIO_PATH_POST_SET)
        /* Customer specific goes here */
        //btapp_fm_set_host_audio_path( p_cb, audio_path );
#endif
        if (b_cback)
        {
            tBTA_FM_PATH_INFO path_data;

            /* always report current_bta_path as if returned by FmConfigAudioPath(). The callback
             * function will return application requested audio_path! */
            path_data.status = BTA_FM_OK;
            path_data.audio_path = p_cb->current_bta_path;
            btui_fm_cback( BTA_FM_AUD_PATH_EVT, (tBTA_FM *)&path_data );
        }
    }
#endif
        break;

    case BTUI_FM_SET_SCAN_STEP_CMD:
        BTA_FmSetScanStep(p_msg->scan_step_cmd.scan_step);
        break;

    case BTUI_FM_SET_REGION_CMD:
        BTA_FmSetRegion(p_msg->region_cmd.region);
        break;

    case BTUI_FM_SET_DEEMPH_CMD:
        BTA_FmConfigDeemphasis(p_msg->deemph_cmd.deemphasis);
        break;

    case BTUI_FM_SET_NFL_CMD:
        BTA_FmEstNoiseFloor(p_msg->nfloor_cmd.nfloor);
        break;

    case BTUI_FM_RD_AUDIO_QUALITY_CMD:
        BTA_FmReadAudioQuality(p_msg->audio_quality_cmd.turn_on);
        break;

    case BTUI_FM_SET_SIGNAL_NOTIFICATION_CMD:
        BTA_FmSetSignalNotifSetting(p_msg->signal_notif_cmd.notif);
        break;

    case BTUI_FM_VOLUME_CMD:
        btapp_fm_volume_control( p_msg->volume_cmd.level );
        break;

    case BTUI_FM_SET_SNR_THRES_CMD:
        {
            tBTA_FM_CVALUE      criteria;
            criteria.snr = p_msg->set_snr_cmd.snr;
            BTA_FmSetSchCriteria(BTA_FM_CTYPE_SNR, &criteria);
        }
        break;

    case BTUI_FM_ENABLE_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_I_param.i1 = (int) p_msg->st.status;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ENABLE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_I_PARAM));
    }
        break;

    case BTUI_FM_DISABLE_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_I_param.i1 = (int) p_msg->st.status;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_DISABLE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_I_PARAM));
    }
        break;

    case BTUI_FM_AF_JMP_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_III_param.i1 = (int) p_msg->af_jmp.status;
        btl_if_params.fm_III_param.i2 = (int) p_msg->af_jmp.freq;
        btl_if_params.fm_III_param.i3 = (int) p_msg->af_jmp.rssi;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AF_JMP_EVT, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_III_PARAM));
    }
        break;

    case BTUI_FM_TUNE_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IIII_param.i1 = (int) p_msg->tune.status;
        btl_if_params.fm_IIII_param.i2 = (int) p_msg->tune.rssi;
        btl_if_params.fm_IIII_param.i3 = (int) p_msg->tune.freq;
        btl_if_params.fm_IIII_param.i4 = (int) p_msg->tune.snr;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_TUNE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IIII_PARAM));
    }
        break;

    case BTUI_FM_SEARCH_CMPL_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IIII_param.i1 = (int) p_msg->search_cmpl.status;
        btl_if_params.fm_IIII_param.i2 = (int) p_msg->search_cmpl.rssi;
        btl_if_params.fm_IIII_param.i3 = (int) p_msg->search_cmpl.freq;
        btl_if_params.fm_IIII_param.i4 = (int) p_msg->search_cmpl.snr;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH_CMPL_EVT, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IIII_PARAM));
    }
        break;

    case BTUI_FM_SEARCH_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_III_param.i1 = (int) p_msg->search.rssi;
        btl_if_params.fm_III_param.i2 = (int) p_msg->search.freq;
        btl_if_params.fm_III_param.i3 = (int) p_msg->search.snr;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_III_PARAM));
    }
        break;

    case BTUI_FM_AUDIO_MODE_EVT:
    {
        tBTL_PARAMS btl_if_params;
        tBTUI_FM_RDS_EVT *p = (tBTUI_FM_RDS_EVT *) p_msg;

        btl_if_params.fm_II_param.i1 = (int) p_msg->audio_mode.status;
        btl_if_params.fm_II_param.i2 = (int) p_msg->audio_mode.audio_mode;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_MODE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    case BTUI_FM_RDS_UPD_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IIIS_param.i1 = (int) p_msg->rds_upd.status;
        btl_if_params.fm_IIIS_param.i2 = (int) p_msg->rds_upd.data;
        btl_if_params.fm_IIIS_param.i3 = (int) p_msg->rds_upd.index;
        memcpy(btl_if_params.fm_IIIS_param.s1, p_msg->rds_upd.s1, 65);
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_RDS_UPDATE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IIIS_PARAM));
    }
        break;

    case BTUI_FM_AUDIO_DATA_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IIII_param.i1 = (int) p_msg->audio_data.status;
        btl_if_params.fm_IIII_param.i2 = (int) p_msg->audio_data.rssi;
        btl_if_params.fm_IIII_param.i3 = (int) p_msg->audio_data.audio_mode;
        btl_if_params.fm_IIII_param.i4 = (int) p_msg->audio_data.snr;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_GET_AUDIO_QUALITY, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IIII_PARAM));
    }
        break;

    case BTUI_FM_AUDIO_PATH_EVT:
    {
        tBTL_PARAMS btl_if_params;
#if (TRUE == BTAPP_FM_AUDIO_PATH_POST_SET)
        /* Customer specific goes here. The audio_path should contain the app request path and
         * not the BTA version! */
        // Adapt to customer platform here.
        if (BTA_FM_OK==p_msg->audio_path.status)
            btapp_fm_set_host_audio_path( &btapp_fm_plat_cb, p_msg->audio_path.audio_path );
#endif
        btl_if_params.fm_II_param.i1 = (int) p_msg->audio_path.status;
        btl_if_params.fm_II_param.i2 = p_msg->audio_path.audio_path;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_PATH, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    case BTUI_FM_RDS_MODE_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IZZ_param.i1 = (int) p_msg->rds_af_mode.status;
        btl_if_params.fm_IZZ_param.z1 = (BOOLEAN) p_msg->rds_af_mode.rds_on;
        btl_if_params.fm_IZZ_param.z2 = (BOOLEAN) p_msg->rds_af_mode.af_on;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_MODE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IZZ_PARAM));
    }
        break;

    case BTUI_FM_SET_DEEMPH_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_II_param.i1 = (int) p_msg->deemphasis.status;
        btl_if_params.fm_II_param.i2 = (int) p_msg->deemphasis.time_const;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_CONFIGURE_DEEMPHASIS, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    case BTUI_FM_MUTE_AUDIO_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_IZ_param.i1 = (int) p_msg->mute_stat.status;
        btl_if_params.fm_IZ_param.z1 = (BOOLEAN) p_msg->mute_stat.is_mute;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_MUTE, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_IZ_PARAM));
    }
        break;

    case BTUI_FM_SCAN_STEP_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_I_param.i1 = (int) p_msg->scan_step.scan_step;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SCAN_STEP, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_I_PARAM));
    }
        break;

    case BTUI_FM_SET_REGION_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_II_param.i1 = (int) p_msg->region_info.status;
        btl_if_params.fm_II_param.i2 = (int) p_msg->region_info.region;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_REGION, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    case BTUI_FM_SET_NFL_EVT:
    {
        tBTL_PARAMS btl_if_params;

        LOGI("BTAPP_FM: BTUI_FM_SET_NFL_EVT > rssi = %d", p_msg->nfloor.rssi);
        btl_if_params.fm_I_param.i1 = (int) p_msg->nfloor.rssi;
        BTL_IF_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ESTIMATE_NFL, &btl_if_params,
                sizeof(tBTLIF_FM_REQ_I_PARAM));
    }
        break;

    case BTUI_FM_SET_RDS_TYPE_EVT:
    {
        tBTL_PARAMS btl_if_params;

        btl_if_params.fm_II_param.i1 = (int) p_msg->rds_type.status;
        btl_if_params.fm_II_param.i2 = (int) p_msg->rds_type.type;
        BTL_IF_CtrlSend( btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_RBDS, &btl_if_params,
                         sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    case BTUI_FM_RESET_PARSER_TASK_EVT:
        APPL_TRACE_DEBUG0("0000000000000000- call BTA_FmRDSResetDecoder...");
        BTA_FmRDSResetDecoder(BTUI_RDS_APP_ID);
        GKI_freebuf(p_msg);
        free_buf = FALSE; /* we free ourself */
        break;

    case BTUI_FM_PARSE_RDS_TASK_EVT:
    {
        //                               LOGI("1111111111111111111- call BTA_FmRDSDecode (%d bytes)...", p_msg->parser.len);
        BTA_FmRDSDecode(BTUI_RDS_APP_ID, p_msg->parser.data, p_msg->parser.len);
        GKI_freebuf(p_msg);
        free_buf = FALSE; /* we free ourself */
    }
        break;

     case BTUI_FM_VOLUME_EVT:
    {
        tBTLIF_FM_REQ_II_PARAM                 fm_II_param;

#if (BTAPP_FM_AUDIO_PATH==BTA_FM_AUDIO_I2S) || (BTAPP_FM_SINGLE_PATH_ONLY==FALSE)
        /* set host volume in i2s. UNSPT error status indicates request i2s volume control handling.
         * in dac case status should be ok or err */
        if (BTA_FM_UNSPT_ERR == p_msg->volume_evt.vol.status)
        {
            btapp_fm_set_host_volume( &btapp_fm_plat_cb, p_msg->volume_evt.vol.volume );
            p_msg->volume_evt.vol.status = BTA_FM_OK;
        }
#endif
        fm_II_param.i1 = (int) p_msg->volume_evt.vol.status;
        fm_II_param.i2 = (int) p_msg->volume_evt.vol.volume;
        BTL_IF_CtrlSend( btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_VOLUME_EVT, (tBTL_PARAMS *)&fm_II_param,
                         sizeof(tBTLIF_FM_REQ_II_PARAM));
    }
        break;

    default:
        APPL_TRACE_WARNING1( "BTAPP_FmHandleEvents( layer_specific: 0x%x ) NOT handled!",
                             p_msg->hdr.layer_specific );
        break;
    }
    return free_buf;
} /* BTAPP_FmHandleEvents() */

#endif
