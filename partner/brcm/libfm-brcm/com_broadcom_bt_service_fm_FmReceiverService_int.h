/* Copyright 2009 - 2011 Broadcom Corporation
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
#ifndef _FM_FMRECEIVERSERVICE_INT_H_
#define _FM_FMRECEIVERSERVICE_INT_H_
#include "fm_cfg.h"


#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define INT32 int32_t

#define BOOLEAN bool


/* Max AF number */
#ifndef BTA_FM_AF_MAX_NUM
#define BTA_FM_AF_MAX_NUM   25
#endif

/* FM band region, bit 0, 1,2 of func_mask */
#ifndef BTA_MAX_REGION_SETTING
#define BTA_MAX_REGION_SETTING  3   /* max region code defined */
#endif

/* RDS decoder event */
enum
{
    BTA_RDS_REG_EVT,        /* RDS decoder user register event*/
    BTA_RDS_PI_EVT,         /* PI code */
    BTA_RDS_PTY_EVT,        /* program type */
    BTA_RDS_TP_EVT,         /* traffic program identification */
    BTA_RDS_DI_EVT,         /* Decoder Identification */
    BTA_RDS_MS_EVT,         /* music speech content */
    BTA_RDS_AF_EVT,         /* Alternative Frequency */
    BTA_RDS_PS_EVT,         /* Program service name */
    BTA_RDS_PTYN_EVT,       /* program type name */
    BTA_RDS_RT_EVT,         /* Radio Text */
    BTA_RDS_PIN_EVT,        /* program Item number */
    BTA_RDS_CT_EVT,         /* clock time */
    BTA_RDS_ODA_EVT,        /* open data application */
    BTA_RDS_TDC_EVT,        /* transparent data channel */
    BTA_RDS_EWS_EVT,        /* emergency warning system */
    BTA_RDS_SLC_EVT,         /* slow labelling code */
    BTA_RDS_RPTOR_EVT,      /* charcter repertoire set */
    BTA_RDS_EON_EVT,        /* EON information, should be evalute with sub code */
    BTA_RDS_TMC_EVT,        /* Traffic Message Channel */
    BTA_RDS_RTP_EVT,        /* RT plus event */
    BTA_RDS_STATS_EVT
};
typedef int tBTA_RDS_EVT;


enum
{
    BTA_FM_ENABLE_EVT,          /* BTA FM is enabled */
    BTA_FM_DISABLE_EVT,         /* BTA FM is disabled */
    BTA_FM_TUNE_EVT,            /* FM tuning is completed or failed */
    BTA_FM_SEARCH_EVT,          /* FM scanning, clear channel update event */
    BTA_FM_SEARCH_CMPL_EVT,     /* FM scanning is completed event */
    BTA_FM_AF_JMP_EVT,          /* AF jump event */
    BTA_FM_AUD_MODE_EVT,        /* Set audio mode completed  */
    BTA_FM_AUD_DATA_EVT,        /* audio quality live updating call back event */
    BTA_FM_AUD_PATH_EVT,        /* set audio path completed */
    BTA_FM_RDS_UPD_EVT,         /* read RDS data event */
    BTA_FM_RDS_MODE_EVT,        /* set RDS mode event */
    BTA_FM_SET_DEEMPH_EVT,      /* configure deempahsis parameter event */
    BTA_FM_MUTE_AUD_EVT,         /* audio mute/unmute event */
    BTA_FM_SCAN_STEP_EVT,       /* config scan step */
    BTA_FM_SET_REGION_EVT,       /* set band region */
    BTA_FM_NFL_EVT,             /* noise floor estimation event */
    BTA_FM_VOLUME_EVT,          /* volume event */
    BTA_FM_SET_SNR_THRES_EVT, /* Set SNR Threshold event */
    BTA_FM_RDS_TYPE_EVT
};
typedef int tBTA_FM_EVT;
enum
{
    BTA_FM_OK,
    BTA_FM_SCAN_RSSI_LOW,
    BTA_FM_SCAN_FAIL,
    BTA_FM_SCAN_ABORT,
    BTA_FM_SCAN_NO_RES,
    BTA_FM_ERR,
    BTA_FM_UNSPT_ERR,
    BTA_FM_FLAG_TOUT_ERR,
    BTA_FM_FREQ_ERR,
    BTA_FM_VCMD_ERR
};
typedef UINT8   tBTA_FM_STATUS;
/* FM function mask */
#define     BTA_FM_REGION_NA    0x00        /* bit0/bit1/bit2: north america */
#define     BTA_FM_REGION_EUR   0x01        /* bit0/bit1/bit2: Europe           */
#define     BTA_FM_REGION_JP    0x02        /* bit0/bit1/bit2: Japan            */
#define     BTA_FM_REGION_JP_II   0x03        /* bit0/bit1/bit2: Japan II         */
#define     BTA_FM_RDS_BIT      1<<4        /* bit4: RDS functionality */
#define     BTA_FM_RBDS_BIT     1<<5        /* bit5: RBDS functionality, exclusive with RDS bit */
#define     BTA_FM_AF_BIT       1<<6        /* bit6: AF functionality */
#define     BTA_FM_SOFTMUTE_BIT       1<<8        /* bit8: SOFTMUTE functionality */
typedef UINT16   tBTA_FM_FUNC_MASK;

#define     BTA_FM_REGION_MASK  0x07             /* low 3 bits (bit0, 1, 2)of FUNC mask is region code */
typedef UINT8  tBTA_FM_REGION_CODE;


#define     BTA_FM_DEEMPHA_50U      0       /* 6th bit in FM_AUDIO_CTRL0 set to 0, Europe default */
#define     BTA_FM_DEEMPHA_75U      1<<6    /* 6th bit in FM_AUDIO_CTRL0 set to 1, US  default */
typedef UINT8 tBTA_FM_DEEMPHA_TIME;
enum
{
    BTA_FM_SCH_RDS_NONE,
    BTA_FM_SCH_RDS_PTY,
    BTA_FM_SCH_RDS_TP
};
typedef UINT8 tBTA_FM_SCH_RDS_TYPE;

typedef struct
{
    tBTA_FM_SCH_RDS_TYPE    cond_type;
    UINT8                   cond_val;
} tBTA_FM_SCH_RDS_COND;

/* FM audio output mode */
enum
{
    BTA_FM_AUTO_MODE,       /* auto blend by default */
    BTA_FM_STEREO_MODE,     /* manual stereo switch */
    BTA_FM_MONO_MODE,       /* manual mono switch */
    BTA_FM_SWITCH_MODE      /* auto stereo, and switch activated */
};
typedef UINT8  tBTA_FM_AUDIO_MODE;

/* FM audio output mode */
enum
{
    BTA_FM_AUDIO_PATH_NONE,             /* None */
    BTA_FM_AUDIO_PATH_SPEAKER,          /* speaker */
    BTA_FM_AUDIO_PATH_WIRE_HEADSET,     /* wire headset */
    BTA_FM_AUDIO_PATH_I2S               /* digital */
};
typedef UINT8  tBTA_FM_AUDIO_PATH;

/* FM audio output quality */
#define BTA_FM_STEREO_ACTIVE    0x01     /* audio stereo detected */
#define BTA_FM_MONO_ACTIVE      0x02     /* audio mono */
#define BTA_FM_BLEND_ACTIVE     0x04     /* stereo blend active */

typedef UINT8  tBTA_FM_AUDIO_QUALITY;

/* FM audio routing configuration */
#define BTA_FM_SCO_ON       0x01        /* routing FM over SCO off, voice on SCO on */
#define BTA_FM_SCO_OFF      0x02        /* routing FM over SCO on, voice on SCO off */
#define BTA_FM_A2D_ON       0x10        /* FM analog output active */
#define BTA_FM_A2D_OFF      0x40        /* FM analog outout off */
#define BTA_FM_I2S_ON       0x20        /* FM digital (I2S) output on */
#define BTA_FM_I2S_OFF      0x80        /* FM digital output off */

typedef UINT8 tBTA_FM_AUDIO_PATH;

/* scan mode */
//#define BTA_FM_PRESET_SCAN      I2C_FM_SEARCH_PRESET        /* preset scan : bit0 = 1 */
//#define BTA_FM_NORMAL_SCAN      I2C_FM_SEARCH_NORMAL        /* normal scan : bit0 = 0 */
typedef UINT8 tBTA_FM_SCAN_METHOD;

/* frequency scanning direction */
#define BTA_FM_SCAN_DOWN        0x00        /* bit7 = 0 scanning toward lower frequency */
#define BTA_FM_SCAN_UP          0x80        /* bit7 = 1 scanning toward higher frequency */
typedef UINT8 tBTA_FM_SCAN_DIR;

#define BTA_FM_SCAN_FULL        (BTA_FM_SCAN_UP | BTA_FM_NORMAL_SCAN|0x02)       /* full band scan */
#define BTA_FM_FAST_SCAN        (BTA_FM_SCAN_UP | BTA_FM_PRESET_SCAN)       /* use preset scan */
#define BTA_FM_SCAN_NONE        0xff

typedef UINT8 tBTA_FM_SCAN_MODE;

#define  BTA_FM_STEP_100KHZ     0x00
#define  BTA_FM_STEP_50KHZ      0x10
typedef UINT8   tBTA_FM_STEP_TYPE;

/* RDS mode */
enum
{
    BTA_FM_RDS,
    BTA_FM_RBDS
};
typedef UINT8 tBTA_FM_RDS_B;

enum
{
    BTA_FM_AF_FM = 1,
    BTA_FM_AF_LF,
    BTA_FM_AF_MF
};
typedef UINT8 tBTA_FM_AF_TYPE;

/* AF structure */
typedef struct
{
    UINT16              num_af;     /* number of AF in list */
    tBTA_FM_AF_TYPE     af_type[BTA_FM_AF_MAX_NUM];
    UINT16              af_list[BTA_FM_AF_MAX_NUM];
}tBTA_FM_AF_LIST;

typedef struct
{
    UINT16              pi_code;          /* currently tuned frequency PI code */
    tBTA_FM_AF_LIST     af_list;          /* AF frequency list */
    UINT8               af_thresh;        /* AF jump RSSI threshold*/
} tBTA_FM_AF_PARAM;

enum
{
    BTA_FM_NFL_LOW,
    BTA_FM_NFL_MED,
    BTA_FM_NFL_FINE
};
typedef UINT8 tBTA_FM_NFE_LEVL;

/* channel tunning/scanning call back data */
typedef struct
{
    tBTA_FM_STATUS      status;         /* operation status */
    UINT8               rssi;
    UINT16              freq;           /* tuned frequency */
    INT8               snr;           /* current channel SNR value */
}tBTA_FM_CHNL_DATA;


/* set FM audio mode callback data */
typedef struct
{
    tBTA_FM_STATUS      status;     /* operation status */
    tBTA_FM_AUDIO_MODE  audio_mode; /* audio mode */
}tBTA_FM_MODE_INFO;

/* set FM audio path callback data */
typedef struct
{
    tBTA_FM_STATUS      status;     /* operation status */
    tBTA_FM_AUDIO_PATH  audio_path; /* audio path */
}tBTA_FM_PATH_INFO;

/* audio quality live updating call back data */
typedef struct
{
    tBTA_FM_STATUS          status;     /* operation status */
    UINT8                   rssi;       /* rssi strength    */
    INT8                    snr;        /*current channel SNR value */
    tBTA_FM_AUDIO_QUALITY   audio_mode; /* audio mode       */
}tBTA_FM_AUD_DATA;


typedef struct
{
    tBTA_FM_STATUS          status;     /* operation status         */
    tBTA_FM_DEEMPHA_TIME    time_const; /* deemphasis parameter     */
}tBTA_FM_DEEMPH_DATA;

/* set FM audio mode callback data */
typedef struct
{
    tBTA_FM_STATUS      status;     /* operation status */
    BOOLEAN             rds_on;
    BOOLEAN             af_on;  /* audio mode */
}tBTA_FM_RDS_MODE_INFO;

typedef struct
{
    UINT8               rssi;
    UINT16              freq;
    INT8               snr;        /*current channel SNR value */
}tBTA_FM_SCAN_DAT;

typedef struct
{
    tBTA_FM_STATUS      status;
    BOOLEAN             is_mute;
}tBTA_FM_MUTE_STAT;

typedef struct
{
    tBTA_FM_STATUS          status;
    tBTA_FM_REGION_CODE     region;
}tBTA_FM_REGION_INFO;

typedef struct
{
    tBTA_FM_STATUS          status;
    tBTA_FM_RDS_B           type;
}tBTA_FM_RDS_TYPE;

typedef struct
{
    tBTA_FM_STATUS          status;
    UINT8                   data;
    UINT8                   index;
    char                    text[65];
}tBTA_FM_RDS_UPDATE;

typedef struct
{
    tBTA_FM_STATUS      status;
    UINT16              volume;
}tBTA_FM_VOLUME;

/* Union of all FM callback structures */
typedef union
{
    tBTA_FM_STATUS          status;             /* BTA_FM_DISABLE_EVT/
                                                   BTA_FM_ENABLE_EVT/
                                                   BTA_FM_AUD_PATH_EVT/
                                                   BTA_FM_RDS_MODE_EVT call back data
                                                   BTA_FM_AUD_PATH_EVTcall back data*/
    tBTA_FM_CHNL_DATA       chnl_info;          /* BTA_FM_TUNE_EVT/BTA_FM_SEARCH_EVT */
    tBTA_FM_MODE_INFO       mode_info;          /* BTA_FM_AUD_MODE_EVT */
    tBTA_FM_PATH_INFO       path_info;          /* BTA_FM_AUD_PATH_EVT */
    tBTA_FM_AUD_DATA        audio_data;         /* BTA_FM_AUD_DATA_EVT call back data */
    tBTA_FM_DEEMPH_DATA     deemphasis;         /* BTA_FM_SET_DEEMPH_EVT call back data */
    tBTA_FM_RDS_MODE_INFO   rds_mode;           /* BTA_FM_RDS_MODE_EVT call back data */
    tBTA_FM_SCAN_DAT        scan_data;          /* BTA_FM_SEARCH_EVT call back data */
    tBTA_FM_MUTE_STAT       mute_stat;          /* BTA_FM_MUTE_AUD_EVT call back data */
    tBTA_FM_STEP_TYPE       scan_step;          /* BTA_FM_SCAN_STEP_EVT callback data */
    tBTA_FM_REGION_INFO     region_info;        /* BTA_FM_SET_REGION_EVT callback data */
    tBTA_FM_RDS_TYPE        rds_type;
    tBTA_FM_RDS_UPDATE      rds_update;
    UINT8                   nfloor;
    tBTA_FM_VOLUME          volume;             /* BTA_FM_VOLUME_EVT */
} tBTA_FM;


/* run-time configuration struct */
typedef struct
{
    UINT16                  low_bound;      /* lowest frequency boundary */
    UINT16                  high_bound;     /* highest frequency boundary */
    tBTA_FM_DEEMPHA_TIME    deemphasis;     /* FM de-emphasis time constant */
    UINT8                   scan_step;      /* scanning step */
} tBTA_FM_CFG_ENTY;

typedef struct
{
    tBTA_FM_CFG_ENTY        reg_cfg_tbl[BTA_MAX_REGION_SETTING];
                                            /* region related setting table */
    INT32                   aud_timer_value;/* audio quality live updating timer
                                               value, must be a non-0 positive value */
    INT32                   rds_timer_value;/* RDS data update timer */
    INT32                   enable_delay_val;/* FM enable delay time value, platform dependant */
    UINT8                   rds_read_size;  /* how many RDS tuples per read */
    UINT8                   max_station;    /* maximum number of FM stations for a full band scan
                                                can not exceed BTA_FM_MAX_PRESET_STA */
} tBTA_FM_CFG;

typedef struct
{
    UINT16   cur_freq;
}tBTA_FM_CTX;

#endif


