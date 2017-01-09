/*****************************************************************************
**
**  Name:             btapp_fm.h
**
**  Description:
**
**  Copyright (c) 2000-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/


#ifndef BTAPP_FM_H
#define BTAPP_FM_H

#include "bta_fm_api.h"
#include "bta_rds_api.h"

#ifndef BTUI_FM_TEST
#define BTUI_FM_TEST            FALSE
#endif

#ifndef BTAPP_FM_SUPPORT_AUDIOPATH
#define BTAPP_FM_SUPPORT_AUDIOPATH TRUE
#endif

/* I2C_FM_SEARCH_METHOD */
#ifndef I2C_FM_SEARCH_NORMAL
#define I2C_FM_SEARCH_NORMAL            0x00
#endif
#ifndef I2C_FM_SEARCH_PRESET
#define I2C_FM_SEARCH_PRESET            0x01  /* default: preset scan with CO */
#endif
#ifndef I2C_FM_SEARCH_RSSI
#define I2C_FM_SEARCH_RSSI              0x02
#endif
#ifndef I2C_FM_SEARCH_PRESET_SNR
#define I2C_FM_SEARCH_PRESET_SNR        0x03  /* preset scan with SNR */
#endif


/* BTUI FM state */
#define BTUI_FM_NULL_ST             0x00
#define BTUI_FM_ACTIVE_ST           0x01

#define BTUI_FM_GET_FREQ(x)     (UINT16)((x + 0.001) *100)
#define BTUI_FM_CVT_FREQ(x)     (float) (x/100.0)
#define BTUI_FM_MAX_FREQ        BTUI_FM_GET_FREQ(108.0)
#define BTUI_FM_MIN_FREQ        BTUI_FM_GET_FREQ(87.5)
#define BTUI_FM_DEFAULT_STEP    10

#define BTUI_RDS_APP_ID         1
#define BTUI_FM_MAX_STAT_LST    15

/* FM play via option */
enum
{
    BTUI_FM_VIA_WIRED,   /* play via wired headset */
    BTUI_FM_VIA_BT_HFP,  /* play via BT mono headset using HFP */
    BTUI_FM_VIA_BT_A2DP  /* play via BT stereo headset over A2DP */
};
typedef UINT8 tBTUI_FM_PLAY_OPT;


enum
{
    HSET_UI_NON_FM,
    HSET_UI_MAIN_DISPLAY,
    HSET_UI_MAIN_MENU,
    HSET_UI_SUBMENU_1,
    HSET_UI_SUBMENU_2,
    HSET_UI_SUBMENU_SCH,
    HSET_UI_SUBMENU_COMBO_SCH,
    HSET_UI_SUBMENU,
    HSET_UI_SUBMENU_RECORD,
    /* TODO by mschoi : will remove SUBMENU_1 and SUBMENU_2 later. */
    /* the state will be all SUBMENU if it is not MAIN_MENU state */
    HSET_UI_MAX
};

enum
{
    BTUI_FM_SCAN_OFF,
    BTUI_FM_SCAN_ON,
    BTUI_FM_SCAN_FULL_ON
};
typedef UINT8  tBTUI_FM_SCAN_STAT;

enum
{
    BTUI_RDS_PS,
    BTUI_RDS_PTY,
    BTUI_RDS_PTYN,
    BTUI_RDS_RT
};
typedef UINT8 tBTUI_RDS_DATA_TYPE;

typedef struct
{
    BOOLEAN             af_avail;
    BOOLEAN             af_on;
    UINT16              pi_code;
    tBTA_FM_AF_LIST     af_list;
}tBTUI_FM_AF_CB;

typedef struct
{
    tBTA_FM_AUDIO_PATH      audio_path;
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    tAUDIO_CODEC_TYPE       codec_type;
    tCODEC_INFO             codec_info;
    tAUDIO_ROUTE_SF         i2s_sf;         /* I2S Output sampling freq */
#endif
} tBTUI_FM_Params;

typedef struct
{
    tBTA_RDS_RTP            rt_plus;
    UINT32                  fm_main_menu;
    BOOLEAN                 enabled;
    BOOLEAN                 rds_on;
    BOOLEAN                 audio_mute;
    BOOLEAN                 fm_live_update;
    UINT8                   act_from_btui;
    UINT8                   state;
    UINT8                   hset_ui_state;
    tBTA_FM_AUDIO_MODE      mode;
    tBTA_FM_AUDIO_PATH      path;       /* this relies on bta_fm_api.h (and bta_fm_act.c) channges */
    tBTA_FM_STEP_TYPE       scan_step;
    UINT16                  service_mask;
    UINT16                  cur_freq;
    tBTUI_FM_AF_CB          af_cb;
    UINT16                  volume;

    UINT16                  stat_lst[BTUI_FM_MAX_STAT_LST];
    UINT8                   stat_num;
    tBTUI_FM_SCAN_STAT      scan_stat;
    UINT32                  scan_proc_handle;
    BOOLEAN                 init_cfg;
    BOOLEAN                 nfe_thresh;
    UINT8                   closing;        /* Number of closing BT devices */
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE)
    BOOLEAN                 av_started;
    UINT32                  rec_handle;
    BOOLEAN                 rec_started;
    FILE                    *p_recfile;
#endif

    /* The fields to be retained after FM On/Off will be added to parms. */
    tBTUI_FM_Params         parms;

} tBTUI_FM_CB;

typedef struct
{
    UINT8                   prg_service[9];
    UINT8                   prg_type[9];
    UINT8                   prg_type_name[9];
    UINT8                   radio_txt[65];
    BOOLEAN                 ps_on;
    BOOLEAN                 pty_on;
    BOOLEAN                 ptyn_on;
    BOOLEAN                 rt_on;
} tBTUI_RDS_CB;

extern tBTUI_FM_CB btui_fm_cb;
extern tBTUI_RDS_CB btui_rds_cb;

extern const UINT8* audio_mode_name[4];


extern void btui_fm_cback(tBTA_FM_EVT event, tBTA_FM *p_data);
extern void btapp_fm_disable(void);
extern void btapp_fm_init(void);
extern void btapp_fm_save_station (float freq);
extern void btapp_fm_del_station(float freq);
extern void btapp_fm_abort(void);
extern void btapp_fm_set_rds_mode(BOOLEAN rds_on, BOOLEAN af_on);
extern void btapp_fm_set_audio_mode(tBTA_FM_AUDIO_MODE mode);
extern void btapp_fm_search_freq(tBTA_FM_SCAN_MODE search_up, BOOLEAN combo );
extern void btapp_fm_rds_search(tBTA_FM_SCAN_MODE direction, tBTA_FM_SCH_RDS_COND rds_cond);
extern void btapp_fm_tune_freq(UINT16 freq);
extern void btapp_fm_mute_audio(BOOLEAN mute);
extern void btapp_fm_rssi_notification(BOOLEAN dyn_rssi);
extern void btapp_fm_read_rds(void);
extern void btapp_fm_route_audio(tBTA_FM_AUDIO_PATH path, BOOLEAN recording);
extern void btapp_fm_set_deemphasis(tBTA_FM_DEEMPHA_TIME interval);
extern void btapp_fm_rdsp_cback(tBTA_RDS_EVT event, tBTA_FM_RDS *p_data, UINT8 app_id);
extern void btapp_fm_init( void );
extern void btui_fm_display_search_msg(void);
extern void btapp_fm_set_scan_step(tBTA_FM_STEP_TYPE step);
extern void btapp_fm_set_region(tBTA_FM_REGION_CODE region);
extern void btapp_fm_set_rds(tBTA_FM_RDS_B rds_type);
extern void btapp_fm_get_nfe (tBTA_FM_NFE_LEVL level);
extern void btapp_fm_cfg_blend_mute(UINT8 start_snr, UINT8 stop_snr, INT8 start_rssi,
                             INT8 stop_rssi, UINT8 start_mute, INT8 stop_tten,
                             UINT8 mute_rate, INT8 snr_40);
extern void btapp_fm_volume_control(UINT16 volume);

extern BOOLEAN BTAPP_FmHandleEvents( tBTUI_FM_RDS_EVT *p_msg );

#endif
