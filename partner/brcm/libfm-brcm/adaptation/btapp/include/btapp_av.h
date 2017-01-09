/*****************************************************************************
**
**  Name:             btapp_av.h
**
**  Description:     This file contains  internal interface
**				     definition
**
**  Copyright (c) 2000-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_AV_H
#define BTAPP_AV_H

#include "a2d_sbc.h"
#include "bta_av_co.h"

#include "btui.h"

#define BTUI_AV_SBC_MAX_BITPOOL  0x59

#define BTUI_AV_MIN_SAMPLE_RATE 4000
#define BTUI_AV_MAX_SAMPLE_RATE 48000

#ifndef BTUI_AV_M12_SUPORT
#define BTUI_AV_M12_SUPORT      TRUE
#endif

#ifndef BTUI_AV_M24_SUPORT
#define BTUI_AV_M24_SUPORT      TRUE
#endif

#ifndef BTUI_AV_MPEG4_SUPORT
#define BTUI_AV_MPEG4_SUPORT    TRUE
#endif

#ifndef BTUI_AV_H263P3_SUPORT
#define BTUI_AV_H263P3_SUPORT   TRUE
#endif

#ifndef BTUI_AV_H263P8_SUPORT
#define BTUI_AV_H263P8_SUPORT   TRUE
#endif

#ifndef BTUI_AV_VEND1_SUPORT
#define BTUI_AV_VEND1_SUPORT    TRUE
#endif

#ifndef BTUI_AV_VEND2_SUPORT
#define BTUI_AV_VEND2_SUPORT    TRUE
#endif

#define BTUI_AV_SBC_INDEX       0
#define BTUI_AV_M12_INDEX       1

#if (BTUI_AV_M12_SUPORT == TRUE)
#include "a2d_m12.h"
#include "bta_av_m12.h"
#define BTUI_AV_M24_INDEX       2
#else
#define BTUI_AV_M24_INDEX       1
#endif

#if (BTUI_AV_M24_SUPORT == TRUE)
#include "a2d_m24.h"
#endif

#define BTUI_AV_H263P0_INDEX    0
#if (BTUI_AV_MPEG4_SUPORT == TRUE)
#define BTUI_AV_MPEG4_INDEX     (BTUI_AV_H263P0_INDEX + 1)
#else
#define BTUI_AV_MPEG4_INDEX     (BTUI_AV_H263P0_INDEX)
#endif

#if (BTUI_AV_H263P3_SUPORT == TRUE)
#define BTUI_AV_H263P3_INDEX    (BTUI_AV_MPEG4_INDEX  + 1)
#else
#define BTUI_AV_H263P3_INDEX    (BTUI_AV_MPEG4_INDEX)
#endif

#if (BTUI_AV_H263P8_SUPORT == TRUE)
#define BTUI_AV_H263P8_INDEX    (BTUI_AV_H263P3_INDEX + 1)
#else
#define BTUI_AV_H263P8_INDEX    (BTUI_AV_H263P3_INDEX)
#endif

#if (BTUI_AV_VEND1_SUPORT == TRUE)
#define BTUI_AV_VENDOR1_INDEX   (BTUI_AV_H263P8_INDEX + 1)
#else
#define BTUI_AV_VENDOR1_INDEX   (BTUI_AV_H263P8_INDEX)
#endif

#define BTUI_AV_VENDOR2_INDEX   (BTUI_AV_VENDOR1_INDEX+ 1)

#ifndef BTUI_AV_H263_EXT
#define BTUI_AV_H263_EXT        "263"   /* pre-encodec H.263 video file */
#endif

#ifndef BTUI_AV_MP4_EXT
#define BTUI_AV_MP4_EXT         "mp4"   /* MP4 file format. assuming both Audio and video */
#endif

#ifndef BTUI_AV_M4A_EXT
#define BTUI_AV_M4A_EXT         "m4a"   /* MP4 file format. assuming audio only. AAC format */
#endif

#ifndef BTUI_AV_MP3_EXT
#define BTUI_AV_MP3_EXT         "mp3"   /* MP3 file format. audio only. */
#endif

#ifndef BTUI_AV_WAV_EXT
#define BTUI_AV_WAV_EXT         "wav"
#endif

#ifndef BTUI_AV_SBC_EXT
#define BTUI_AV_SBC_EXT         "sbc"
#endif

#define BTUI_AV_ACTIVE_NONE     0xFF
#define BTUI_AV_PATH              "test_files\\av"
#define BTUI_AV_LIST_SIZE       50     /* the maximum number of AV files for display */

#ifndef BTUI_AV_MAX_PLAY_COUNT
#define BTUI_AV_MAX_PLAY_COUNT  20      /* the maximum number of AV files allowed in the now playing list */
#endif

#define AVRCP_NOW_PLAYING_FOLDER_ID     0x00000000
#define BTUI_AV_PATH_NOW_PLAY           "00now_playing"
#define BTUI_AV_ITEM_UNKNOWN            (-1)

#ifndef BTUI_AV_DEF_1FILE_NAME
#define BTUI_AV_DEF_1FILE_NAME  "Goodbye.wav"
#endif

#ifndef BTUI_AV_DEF_1MD_NAME
#define BTUI_AV_DEF_1MD_NAME    "Goodbye.md"
#endif

#ifndef BTUI_AV_DEF_2FILE_NAME
#define BTUI_AV_DEF_2FILE_NAME  "angel_fire_w44.wav"
#endif

#ifndef BTUI_AV_DEF_2MD_NAME
#define BTUI_AV_DEF_2MD_NAME    "angel_fire.md"
#endif

enum
{
    BTUI_RCFG_FALSE,
    BTUI_RCFG_NEEDED,
    BTUI_RCFG_PROG,
    BTUI_RCFG_PEND
};

/* use index 0 for video handle */
#define BTUI_AV_HNDL_TO_MASK(h) (1<<(h&BTA_AV_HNDL_MSK))

/* Initialize the codec service. */
typedef void (*tBTUI_AV_INIT)(void);

/* open current file,
 * parse file for codec_info, fr_period, fr_per_tick, tick_period.
 * close file.
 * return codec_info */
typedef tBTA_AV_STATUS (*tBTUI_AV_READ_CFG)(char *p_name, UINT8 *p_codec_info);

/* set up control block. create thread/event, if needed */
typedef tBTA_AV_STATUS (*tBTUI_AV_AOPEN)(void *p_cb, UINT8 *p_file_info);

/* set up control block. create thread/event, if needed */
typedef tBTA_AV_STATUS (*tBTUI_AV_VOPEN)(void *p_cb, UINT8 *p_codec_info);

/* open current file for reading
 * parse file for codec_info, fr_period, fr_per_tick, tick_period.
 */
typedef tBTA_AV_STATUS (*tBTUI_AV_OPEN_FILE)(char *p_name);

enum
{
    BTUI_AV_UPDATE_NONE = 0,
    BTUI_AV_UPDATE_BAD,
    BTUI_AV_UPDATE_TIME
};
/* update the mtu
 * return: 0: no change necessary
 *         1: the new MTU is not acceptable (too small)
 *         2: the new MTU is acceptable, need to read the new tick_period and fr_per_tick */
typedef UINT8 (*tBTUI_AV_UPDATE_CFG)(void *p_cfg);

/* TODO what makes sense?? what happens if update */
typedef void (*tBTUI_AV_TIME)(UINT32 *p_tick_period);

/* free out_q
 * kill timer */
typedef void (*tBTUI_AV_STOP)(void);

/* close the file
 * kill task/event if applicable
 * kill timer */
typedef void (*tBTUI_AV_CLOSE)(void);

/* timer expires, read frames and queue to out_q */
typedef void (*tBTUI_AV_TMR_CBACK)(void);

/* callback event */
enum
{
    BTUI_AV_CODEC_RX_READY = 1,
    BTUI_AV_CODEC_EOF,
    BTUI_AV_CODEC_ERR
};

/* events indicating audio hdr ready */
#define BTUI_AV_HDR_NOT_READY			0
#define BTUI_AV_HDR_READY				1

/* application callback */
typedef void (tBTUI_AV_CODEC_CBACK)(UINT8 event);

/* audio interface object */
typedef struct {
    tBTUI_AV_INIT           init_fn;
    tBTUI_AV_READ_CFG       cfg_fn;
    tBTUI_AV_AOPEN          open_fn;
    tBTUI_AV_UPDATE_CFG     update_fn;
    tBTUI_AV_TIME           time_fn;
    tBTUI_AV_CLOSE          close_fn;
    tBTUI_AV_TMR_CBACK      tmr_cback;
    char                    ext[4];         /* file name extension */
} tBTUI_AV_AIF;

#define BTUI_AV_ERR_SIZE    0xFFFF
/* read an video frame into p_buf,
 * return data len, (0 if EOF, BTUI_AV_ERR_SIZE if error)
 *        delta time (p_delta_t) since last frame read */
typedef UINT32 (*tBTUI_AV_READ)(UINT8 *p_buf, UINT32 *p_delta_t);

/* the number of frames till next I-frame. return the number of frames skipped */
typedef UINT8 (*tBTUI_AV_SKIP)(UINT8 qcount);

/* video interface object */
typedef struct {
    tBTUI_AV_INIT           init_fn;
    tBTUI_AV_READ_CFG       cfg_fn;
    tBTUI_AV_VOPEN          open_fn;
    tBTUI_AV_TIME           time_fn;
    tBTUI_AV_CLOSE          close_fn;
    tBTUI_AV_READ           read_fn;
    tBTUI_AV_SKIP           skip_fn;
    char                    ext[4];         /* file name extension */
} tBTUI_AV_VIF;

#define BTUI_AV_AI_MASK         0x07
#define BTUI_AV_AUDIO_FORMAT    0x08
#define BTUI_AV_VI_MASK         0x70
#define BTUI_AV_VIDEO_FORMAT    0x80

#define BTUI_AV_AUDIO_INDEX(x) ((x) & BTUI_AV_AI_MASK)
#define BTUI_AV_VIDEO_INDEX(x) (((x) & BTUI_AV_VI_MASK) >> 4)

#define BTUI_AV_AFMT_WAV        (BTUI_AV_AUDIO_FORMAT)
#define BTUI_AV_AFMT_SBC        (BTUI_AV_AUDIO_FORMAT | 1)
#define BTUI_AV_AFMT_MP3        (BTUI_AV_AUDIO_FORMAT | 2)
#define BTUI_AV_AFMT_M4A        (BTUI_AV_AUDIO_FORMAT | 3)
#define BTUI_AV_AFMT_MP4        (BTUI_AV_AUDIO_FORMAT | 4)

#define BTUI_AV_VFMT_H263       (BTUI_AV_VIDEO_FORMAT)
#define BTUI_AV_VFMT_MP4        (BTUI_AV_VIDEO_FORMAT | 0x10)

#define BTUI_AV_META_MAX_NUM                    255
/* The max size of string in any character set is set to 255 bytes. */
#define BTUI_AV_META_MAX_STR_LEN                255
#define BTUI_AV_META_SONG_DATA_LEN              BTUI_AV_META_MAX_STR_LEN*4+30
/* The maximum length of track info is set to 3 digits  */
#define BTUI_AV_META_MAX_STR_LEN_TRACK          3
/* The maximum length of playing time is set to 1 hour.  */
#define BTUI_AV_META_MAX_STR_LEN_PLAYING_TIME   7
/* The maximum folder depth for browsing */
#define BTUI_AV_MAX_FOLDER_DEPTH            3
/* The maximum folder/file name length */
#define BTUI_AV_MAX_NAME_LEN                30

/* btui_app can not support dynamically switching players, so we can only mark it as one player supported */
#ifndef BTUI_AV_NUM_MPLAYERS
#define BTUI_AV_NUM_MPLAYERS                2
#endif
#define BTUI_AV_PLAYER_ID_INVALID           0
#define BTUI_AV_PLAYER_ID_FILES             1
#define BTUI_AV_PLAYER_ID_MPLAYER           2
#define BTUI_AV_PLAYER_ID_FM                3

#define BTUI_AV_DEFAULT_ADDR_PLAYER         BTUI_AV_PLAYER_ID_FILES
#define BTUI_AV_DEFAULT_BR_PLAYER           BTUI_AV_PLAYER_ID_FILES

#define BTUI_UTF8_CHARACTER_SET                 0x6A
#define BTUI_META_DEFAULT_CHARACTER_SET         BTUI_UTF8_CHARACTER_SET


#define BTUI_META_PLAYER_SETTING_MENU_EXT_1     (AVRC_PLAYER_SETTING_LOW_MENU_EXT)
#define BTUI_META_PLAYER_SETTING_MENU_EXT_2     (AVRC_PLAYER_SETTING_LOW_MENU_EXT+1)
#define BTUI_META_PLAYER_SETTING_MENU_EXT_3     (AVRC_PLAYER_SETTING_LOW_MENU_EXT+2)
#define BTUI_META_PLAYER_SETTING_MENU_EXT_4     (AVRC_PLAYER_SETTING_LOW_MENU_EXT+3)
#define BTUI_META_PLAYER_SETTING_MENU_EXT_MAX   0xFF
#define BTUI_META_PLAYER_SETTING_NUM            0x08

#define BTUI_AV_COMPANY_ID_SIZE                 3
#define BTUI_AV_MAX_COMPANY_LIST_SIZE           BTUI_AV_COMPANY_ID_SIZE*255
#define BTUI_AV_MAX_ATTRIB_SETTING_SIZE         BTUI_AV_META_MAX_STR_LEN*5
#define BTUI_MAX_META_APPSETTING_SIZE           sizeof(tBTA_META_APPSETTING)
#define BTUI_AV_MAX_NUM_MEDIA_ATTR              7

/* bit mask of AVRC_EVT_* */
#define BTUI_AV_RC_EVT_MSK_PLAY_STATUS_CHANGE             0x0001 /* AVRC_EVT_PLAY_STATUS_CHANGE     */
#define BTUI_AV_RC_EVT_MSK_TRACK_CHANGE                   0x0002 /* AVRC_EVT_TRACK_CHANGE           */
#define BTUI_AV_RC_EVT_MSK_TRACK_REACHED_END              0x0004 /* AVRC_EVT_TRACK_REACHED_END      */
#define BTUI_AV_RC_EVT_MSK_TRACK_REACHED_START            0x0008 /* AVRC_EVT_TRACK_REACHED_START    */
#define BTUI_AV_RC_EVT_MSK_PLAY_POS_CHANGED               0x0010 /* AVRC_EVT_PLAY_POS_CHANGED       */
#define BTUI_AV_RC_EVT_MSK_BATTERY_STATUS_CHANGE          0x0020 /* AVRC_EVT_BATTERY_STATUS_CHANGE  */
#define BTUI_AV_RC_EVT_MSK_SYSTEM_STATUS_CHANGE           0x0040 /* AVRC_EVT_SYSTEM_STATUS_CHANGE   */
#define BTUI_AV_RC_EVT_MSK_APP_SETTING_CHANGE             0x0080 /* AVRC_EVT_APP_SETTING_CHANGE     */
/* added in AVRCP 1.4 */
#define BTUI_AV_RC_EVT_MSK_NOW_PLAYING_CHANGE             0x0100 /* AVRC_EVT_NOW_PLAYING_CHANGE     */
#define BTUI_AV_RC_EVT_MSK_AVAL_PLAYERS_CHANGE            0x0200 /* AVRC_EVT_AVAL_PLAYERS_CHANGE    */
#define BTUI_AV_RC_EVT_MSK_ADDR_PLAYER_CHANGE             0x0400 /* AVRC_EVT_ADDR_PLAYER_CHANGE     */
#define BTUI_AV_RC_EVT_MSK_UIDS_CHANGE                    0x0800 /* AVRC_EVT_UIDS_CHANGE            */
#define BTUI_AV_RC_EVT_MSK_VOLUME_CHANGE                  0x1000 /* AVRC_EVT_VOLUME_CHANGE          */

#define BTUI_AV_FS_UID_NOT_FOUND    0
#define BTUI_AV_FS_UID_FOUND        1
#define BTUI_AV_FS_UID_IS_FOLDER    2

/* AV */
typedef struct
{
    char            name[BTUI_NAME_LENGTH + 1]; /* does not include file ext */
    UINT8           av_fmt;     /*  */
} tBTUI_AV_NAME_ENT;

typedef struct
{
    char            name[BTUI_NAME_LENGTH + 1]; /* include file ext */
    char            md[BTUI_NAME_LENGTH + 1];
    UINT8           av_fmt;     /*  */
} tBTUI_AV_PLAY_ENT;
#define BTUI_AV_RC_SUPT_UNKNOWN     0
#define BTUI_AV_RC_SUPT_YES         1
#define BTUI_AV_RC_SUPT_NO          2

typedef struct
{
    BD_ADDR         peer_addr;
    char            short_name[BTUI_DEV_NAME_LENGTH+1];      /* short name of remote device */
    tBTA_AV_FEAT    peer_features;
    tBTA_AV_HNDL    av_handle;
    UINT8           rc_handle;
    BOOLEAN         rc_open;
    UINT8           rc_supt; /* 0: unknown, 1: supported, 2: not supported */
    BOOLEAN         rc_only; /* AVRC only device indicator */
    UINT8           index;
    UINT8           label;
    UINT8           label_volume;   /* the label for register notification event */
    UINT8           pend_label;     /* a command is sent, waiting for response */
    UINT8           cur_volume;
    INT8            queue_volume;   /* non-0 if absolute volume commands are queued */
    INT8            queue_notif;    /* non-0 if reg notif can not be sent due to waiting for response */
    BOOLEAN         rc_play; /* AVRC Play command is received to connect A2DP */
#if (BTU_DUAL_STACK_INCLUDED == TRUE)
    BOOLEAN         started;
#endif
#if (BTA_FM_INCLUDED == TRUE)
    BOOLEAN         fm_connected;   /* Connected through FM menu */
#endif
} tBTUI_AV_AUDIO_ENT;

typedef struct {
    UINT8   *p_song_title;    /* Title */
    UINT8   *p_artist_name;   /* Artist */
    UINT8   *p_album_name;    /* Album  */
    UINT8   *p_track_num;
    UINT32  track_num;        /* Current track number */
    UINT8   *p_total_track;
    UINT32  total_track;      /* Total number of track */
    UINT8   *p_genre_name;    /* Genre */
    UINT8   *p_playing_time;
    UINT32  playing_time;     /* Total playing time in miliseconds */
} tBTUI_AV_META_SONG_INFO;

typedef struct {
    UINT8   attrib_id;
    UINT8   curr_value;
    BOOLEAN value_updated;
    UINT8   *p_attrib_str;
    UINT8   *p_setting_str[AVRC_MAX_APP_SETTINGS];
} tBTUI_AV_META_ATTRIB;

typedef struct {
    tBTUI_AV_META_ATTRIB     equalizer;      /* Equalizer Status */
    tBTUI_AV_META_ATTRIB     repeat;         /* Repeat Mode Status */
    tBTUI_AV_META_ATTRIB     shuffle;        /* Shuffle Status */
    tBTUI_AV_META_ATTRIB     scan;           /* Scan Status */
    tBTUI_AV_META_ATTRIB     menu_ext1;      /* Menu extension 1 Status */
    tBTUI_AV_META_ATTRIB     menu_ext2;      /* Menu extension 2 Status */
    tBTUI_AV_META_ATTRIB     menu_ext3;      /* Menu extension 3 Status */
    tBTUI_AV_META_ATTRIB     menu_ext4;      /* Menu extension 4 Status */
    BOOLEAN                  chk_pas_setting;
} tBTUI_AV_PAS_INFO;

typedef struct {
    UINT32  song_length;
    UINT32  song_pos;
    UINT8   play_status;
} tBTUI_AV_META_PLAYSTAT;

typedef struct {
    UINT8   event_id;
    UINT32  playback_interval;
    UINT32  playback_pos;
    UINT8   bat_stat;
    UINT8   sys_stat;
} tBTUI_AV_STAT_INFO;

typedef UINT16          tBTUI_AV_EVT_MASK;
typedef struct {
    tBTUI_AV_EVT_MASK   evt_mask;
    UINT8               label[AVRC_NUM_NOTIF_EVENTS];
} tBTUI_AV_REG_EVT;

#define BTUI_AV_ABS_VOL_SENT_MASK       0x80

typedef struct {
    UINT8                   rx_cmd;
    tBTUI_AV_REG_EVT        registered_events[BTA_AV_NUM_STRS+1];
    UINT16                  charset_id[BTA_AV_NUM_STRS+1];
    UINT8                   max_attrib_num;     /* app dependent value */
    tBTUI_AV_PAS_INFO       pas_info;
    tBTUI_AV_META_SONG_INFO media_info;/* with btui_song_data*/
    tBTUI_AV_META_SONG_INFO file_info; /* use with btui_file_data*/
    tBTUI_AV_META_PLAYSTAT  play_status;
    tBTUI_AV_STAT_INFO      notif_info;
    UINT16                  addr_player_id;
    UINT16                  br_player_id;
    UINT16                  cur_uid_counter;    /* always 0 -> for database unaware players */
} tBTUI_AV_METADATA;

typedef struct {
    tAVRC_NAME          folder_depth[BTUI_AV_MAX_FOLDER_DEPTH]; /* related to cur_depth */
    tAVRC_UID           uid;        /* the UID of the current folder */
    UINT8               temp[BTUI_AV_MAX_NAME_LEN * BTUI_AV_MAX_FOLDER_DEPTH];
    UINT8               cur_depth;  /* durrent folder depth */
} tBTUI_AV_BROWSE;

#if 0 /* is re-defined in btl_av_codec.h. not very good we should on btl_av_codec.h for codec (bit
    rate, codec type etc) */
typedef struct
{
    tBTUI_STATE         prev_state;                     /* previous UI state. */
    char                play_list[BTUI_AV_LIST_SIZE];   /* this data member can be removed because of the new data member now_list. However, the lite stack is using it, so I do not want to touch it. SS */
    char                temp_path[BTUI_MAX_PATH_LENGTH+1];
    char                path[BTUI_MAX_PATH_LENGTH+1];
    tBTUI_AV_NAME_ENT   file_list[BTUI_AV_LIST_SIZE];   /* max is name length 32, list size 15 05/03/2005 */
    tBTUI_AV_PLAY_ENT   now_list[BTUI_AV_LIST_SIZE];   /* max is name length 32, list size 15 05/03/2005 */
    UINT8               audio_open;         /* the channel mask of open audio channels */
    UINT8               audio_2edr;         /* the channel mask of audio channels that support 2Mbps EDR */
    UINT8               audio_3edr;         /* the channel mask of audio channels that support 3Mbps EDR */
    UINT8               audio_open_count;   /* number of audio channels open */
    UINT8               audio_2edr_count;   /* number of audio channels open that supports only 2Mbps, but not 3Mbps */
    UINT8               video_open; /* the channel mask of open video channel */
    tBTA_AV_CHNL        active_ch;  /* the active menu */
    tBTA_AV_HNDL        active_hndl;/* the active handle */
    tBTA_AV_HNDL        video_hndl; /* the video channel registered */
    tBTUI_AV_AUDIO_ENT  audio[BTA_AV_NUM_STRS+1];
    UINT8               audio_cfg[AVDT_CODEC_SIZE]; /* the codec info of the current file */
    UINT8               video_cfg[AVDT_CODEC_SIZE]; /* the codec info of the current file */
    UINT8               active_audio_idx;   /* the index to audio[] */
    UINT8               num_count;  /* number of AV Audio channels registered */
    UINT8               label;
    UINT8               file_count;
    UINT8               play_count;
    UINT8               cur_play;
    tBTUI_AV_METADATA   meta_info;
    tBTUI_AV_BROWSE     br_info;
    UINT8               prev_trace;
    UINT8               rcfg_chk_mask;  /* the channel mask of the channels that needs to check reconfig on suspend_evt */
#if (defined BTA_FM_INCLUDED && BTA_FM_INCLUDED == TRUE)
    BOOLEAN             fm_disconnect;
    BOOLEAN             play_fm;
#endif
    UINT8               num_rcfg_evt; /* num stream rcfg events received by app. Useful when dual streams */
    UINT8               num_rcfg_ok;  /* num streams rcfged successfully. Useful when dual streams */
    UINT8               num_rcfg_req; /* num_streams that were requested to be reconfigured */
#if (BTU_DUAL_STACK_INCLUDED == TRUE)
    BOOLEAN             is_prestart;
    tBTA_AV_HNDL        prestart_hndl;/* handle for the pre-start processing */
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) && (BTU_DUAL_STACK_NETWORK_MUSIC_INCLUDED == TRUE)
    tBTA_DM_AUX_PATH    aux_path;     /* config audio source of embedded light stack (FM/I2S/PCM/ADC) */
#endif
#endif /* BTU_DUAL_STACK_INCLUDED */
} tBTUI_AV_CB;
#endif

/* data associated with BTA_AV_OPEN_EVT */
typedef struct
{
    BT_HDR          hdr;
    tBTA_AV_CHNL    chnl;
    tBTA_AV_HNDL    hndl;
    BD_ADDR         bd_addr;
    tBTA_AV_STATUS  status;
    BOOLEAN         starting;
    tBTA_AV_EDR     edr;        /* 0, if peer device does not support EDR */
} tBTAPP_AV_OPEN;

/* data from BTA associated with BTA_AV_START_EVT */
typedef struct
{
    BT_HDR          hdr;
    tBTA_AV_CHNL    chnl;
    tBTA_AV_HNDL    hndl;
    tBTA_AV_STATUS  status;
    BOOLEAN         suspending;
} tBTAPP_AV_START;

/* data associated with BTA_AV_SUSPEND_EVT or BTA_AV_STOP_EVT */
typedef struct
{
    tBTA_AV_CHNL    chnl;
    tBTA_AV_HNDL    hndl;
    BOOLEAN         initiator; /* TRUE, if local device initiates the SUSPEND */
    tBTA_AV_STATUS  status;
} tBTAPP_AV_SUSPEND;

typedef union
{
    BT_HDR                hdr;
#if 0
    tBTAPP_AV_CHNL        chnl;
    tBTAPP_AV_ENABLE      enable;
    tBTAPP_AV_REGISTER    registr;
#endif
    tBTAPP_AV_OPEN        open;
#if 0
    tBTAPP_AV_CLOSE       close;
#endif
    tBTAPP_AV_START       start;
#if 0
    tBTAPP_AV_PROTECT_REQ protect_req;
    tBTAPP_AV_PROTECT_RSP protect_rsp;
    tBTAPP_AV_RC_OPEN     rc_open;
    tBTAPP_AV_RC_CLOSE    rc_close;
    tBTAPP_AV_REMOTE_CMD  remote_cmd;
    tBTAPP_AV_REMOTE_RSP  remote_rsp;
    tBTAPP_AV_VENDOR      vendor_cmd;
    tBTAPP_AV_VENDOR      vendor_rsp;
    tBTAPP_AV_RECONFIG    reconfig;
#endif
    tBTAPP_AV_SUSPEND     suspend;
#if 0
    tBTAPP_AV_PEND        pend;
    tBTAPP_AV_META_MSG    meta_msg;
    tBTAPP_AV_REJECT      reject;
    tBTAPP_AV_RC_FEAT     rc_feat;
#endif
} tBTAPP_AV_MSG;

extern const char *btui_av_audio_ext[];
extern tBTUI_AV_CB btui_av_cb;
extern tA2D_SBC_CIE bta_av_co_sbc_pref;
extern tBTA_AV_VIDEO_CFG bta_av_co_h263_pref;
extern void bta_av_co_reconfig(tBTA_AV_HNDL hndl);
extern BOOLEAN bta_av_co_get_rcfg_on_hndl(tBTA_AV_HNDL hndl);

#if (BTU_STACK_LITE_ENABLED == TRUE)
extern void btui_av_sync_mmi_event(UINT16 event);
#else
extern void btui_av_send_mmi_event(UINT16 event);
#endif /* BTU_STACK_LITE_ENABLED */

extern void btapp_av_reset_co(void);
extern void btapp_av_init(void);
extern void btapp_av_increase_vol(void);
extern void btapp_av_decrease_vol(void);
extern void btapp_av_open_rc(void);
extern void btapp_av_close_rc(void);
extern void btapp_av_close_rc_only(void);
extern void btapp_av_start_stream(void);
extern void btapp_av_stop_stream(void);
extern void btapp_av_close(tBTA_AV_HNDL handle);
extern void btapp_av_set_device_authorized (tBTUI_REM_DEVICE * p_device_rec);
extern void btapp_av_open(BD_ADDR bd_addr, tBTA_AV_HNDL handle);
extern int  btapp_av_get_open_count(tBTA_AV_CHNL chnl, tBTA_AV_HNDL *p_hndl);
extern tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent(int index);
extern tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent_by_hndl(tBTA_AV_HNDL handle);
extern tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent_by_rc_hndl(UINT8 handle);
extern BOOLEAN btui_audio_read_cfg(UINT8 *p_codec_info);
extern void btui_audio_open_file(UINT8 *p_codec_info);
extern void btui_audio_chk_reconfig( void );
extern UINT8 btui_av_audio_get_rcfg(tBTA_AV_HNDL hndl);
extern void btui_audio_reconfig_done(void);

extern BOOLEAN btapp_av_connetced_dev(BD_ADDR bda);
#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
extern void btui_av_close_fm_device(void);
#endif /* BTA_FM_INCLUDED */
extern void btui_menu_av_devices(void);
extern void btapp_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data);
extern void btapp_rc_conn(tBTUI_AV_AUDIO_ENT *p_ent);
extern void btapp_rc_check_reset(void);
extern void btapp_rc_send_notification(UINT8 event_id);
extern void btapp_rc_reject_notification(UINT8 event_id);
extern void btapp_rc_change_play_status(UINT8 new_play_status);
extern void btapp_rc_next_label(tBTUI_AV_AUDIO_ENT *p_ent);

extern tAVRC_STS btui_av_add_now_play_item(int item_data, char *p_file_name, char *p_md_name);
extern void btui_av_remove_now_playing_items(void);
extern BOOLEAN btui_read_meta_records(char *p_name, BOOLEAN cur_play);
extern void btapp_rc_reg_volume_notif (tBTUI_AV_AUDIO_ENT *p_ent);
extern void btapp_rc_send_abs_volume_cmd (tBTUI_AV_AUDIO_ENT *p_ent, INT8 delta_volume);
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
extern UINT8 btui_platform_rc_get_item_name_by_uid(tAVRC_UID uid, char *p_name);
extern void btapp_rc_pass_cmd(tBTA_AV_REMOTE_CMD *p_cmd);
#endif /* AVRC_ADV_CTRL_INCLUDED */
extern void btapp_rc_play_file(UINT8 rc_handle);
extern BOOLEAN bta_av_co_reconfig_4file(void);
extern BOOLEAN bta_av_co_chk_reconfig_4file_chg(void);
extern void btui_audio_reconfig_pend( void );
extern void btui_audio_play_item(void);
extern BOOLEAN btapp_av_chk_a2dp_open(tBTUI_AV_AUDIO_ENT  *p_ent);
extern void bta_av_co_chk_state(tBTA_AV_HNDL av_handle);
extern BOOLEAN btapp_av_rc_play(tBTUI_AV_AUDIO_ENT  *p_ent);
extern BOOLEAN btui_platform_rc_read_file_names_by_uidname(char *p_uidname, char *p_media, char *p_md);
extern BOOLEAN btui_av_app_handle_msg(tBTAPP_AV_MSG *p_msg);

#endif /* BTAPP_AV_H */

