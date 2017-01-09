/*****************************************************************************
**
**  Name:           bta_av_co.c
**
**  Description:    This is the advanced audio call-out function
**                  implementation for Insight.
**
**  Copyright (c) 2009, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"
#include <string.h>

#if defined(BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE)

#include "a2d_api.h"
#include "a2d_sbc.h"
#include "a2d_m12.h"
#include "a2d_m24.h"
#include "bta_sys.h"
#include "bta_av_co.h"
#include "bta_av_ci.h"
#include "btl_av_codec.h"

#include "bte_appl.h"

#define LOG_TAG "BTLD"

#if (!defined(LINUX_NATIVE))
#include <cutils/log.h>
#else
#include <stdio.h>
# define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif



#ifndef BTUI_AA_M12_SUPPORT
#define BTUI_AA_M12_SUPPORT          FALSE
#endif

#ifndef BTUI_AA_M24_SUPPORT
#define BTUI_AA_M24_SUPPORT          FALSE
#endif

#ifndef BTUI_AV_ATRAC_SUPPORT
#define BTUI_AV_ATRAC_SUPPORT      FALSE
#endif

#define BTUI_AV_SBC_INDEX       0
#define BTUI_AV_M12_INDEX       1

#if (BTUI_AV_M12_SUPPORT == TRUE)
#define BTUI_AV_M24_INDEX       2
#else
#define BTUI_AV_M24_INDEX       1
#endif

#ifndef BTUI_AV_PCM_DUMP
#define BTUI_AV_PCM_DUMP        0   //Dump SBC output to a file
#endif

/*****************************************************************************
**  Constants
*****************************************************************************/

/* maximum number of packets for btui codec to queue before dropping data
** this is in units of waveIn buffer periods, 20.3 ms
*/
#define BTA_AV_CO_CODEC_TYPE_IDX       2
//HLONG: #define BTA_AV_SBC_MAX_BITPOOL      0x59
#define BTA_AV_SBC_MAX_BITPOOL      53
#define BTA_AV_CO_MAX_SNKS      5   /* max number of SNK devices */
#define BTA_AV_CO_MAX_SEPS      BTA_AV_MAX_SEPS /* number of codec suported locally */

/* SBC codec capabilities */
const tA2D_SBC_CIE bta_av_co_sbc_cap = {
    A2D_SBC_IE_SAMP_FREQ_44,                                /* samp_freq */
    A2D_SBC_IE_CH_MD_JOINT,                                 /* ch_mode */
    A2D_SBC_IE_BLOCKS_16,                                   /* block_len */
    A2D_SBC_IE_SUBBAND_8,                                   /* num_subbands */
    A2D_SBC_IE_ALLOC_MD_L,                                  /* alloc_mthd */
    BTA_AV_SBC_MAX_BITPOOL,                                 /* max_bitpool */
    A2D_SBC_IE_MIN_BITPOOL                                  /* min_bitpool */
};

/* SBC codec preferences */
static tA2D_SBC_CIE bta_av_co_sbc_pref = {
    A2D_SBC_IE_SAMP_FREQ_44,                                /* samp_freq */
    A2D_SBC_IE_CH_MD_JOINT,                                 /* ch_mode */
    A2D_SBC_IE_BLOCKS_16,                                   /* block_len */
    A2D_SBC_IE_SUBBAND_8,                                   /* num_subbands */
    A2D_SBC_IE_ALLOC_MD_L,                                  /* alloc_mthd */
    BTA_AV_SBC_MAX_BITPOOL,                                                      /* max_bitpool */
    A2D_SBC_IE_MIN_BITPOOL                                                       /* min_bitpool */
};

#if (BTUI_AV_M12_SUPPORT == TRUE)
/* MPEG-1, 2 Audio (MP3) codec capabilities */
const tA2D_M12_CIE bta_av_co_m12_cap = {
    (A2D_M12_IE_LAYER3),                                /* layers - MP3 only */
    TRUE,                                               /* Support of CRC protection or not */
    (A2D_M12_IE_CH_MD_MONO  | A2D_M12_IE_CH_MD_DUAL |   /* Channel mode */
     A2D_M12_IE_CH_MD_STEREO| A2D_M12_IE_CH_MD_JOINT),
    1,                                                  /* 1, if MPF-2 is supported. 0, otherwise */
    (A2D_M12_IE_SAMP_FREQ_16 | A2D_M12_IE_SAMP_FREQ_22 |/* Sampling frequency */
     A2D_M12_IE_SAMP_FREQ_24 | A2D_M12_IE_SAMP_FREQ_32 |
     A2D_M12_IE_SAMP_FREQ_44 | A2D_M12_IE_SAMP_FREQ_48),
    TRUE,                                               /* Variable Bit Rate */
    (A2D_M12_IE_BITRATE_7 | A2D_M12_IE_BITRATE_8  |     /* Bit rate index */
     A2D_M12_IE_BITRATE_9 | A2D_M12_IE_BITRATE_10 |     /* for MP3, bit rate index 1,2,3,4,6 are for single channel */
     A2D_M12_IE_BITRATE_11| A2D_M12_IE_BITRATE_12 |     /* bit rate index 13, 14 may be too much in data size */
     A2D_M12_IE_BITRATE_0 | A2D_M12_IE_BITRATE_5 )
};
/* MPEG-1, 2 Audio (MP3) codec preferences */
const tA2D_M12_CIE bta_av_co_m12_pref = {
    (A2D_M12_IE_LAYER3),                                /* layers */
    TRUE,                                               /* Support of CRC protection or not */
    (A2D_M12_IE_CH_MD_JOINT),                           /* Channel mode */
    1,                                                  /* 1, if MPF-2 is supported. 0, otherwise */
    (A2D_M12_IE_SAMP_FREQ_44),                          /* Sampling frequency */
    TRUE,                                               /* Variable Bit Rate */
    A2D_M12_IE_BITRATE_9                                /* Bit rate index */
};
#endif

#if (BTUI_AV_M24_SUPPORT == TRUE)
/* AAC codec capabilities */
const tA2D_M24_CIE bta_av_co_m24_cap = {
    (A2D_M24_IE_OBJ_2LC | A2D_M24_IE_OBJ_4LC |              /* Object type */
     A2D_M24_IE_OBJ_4LTP| A2D_M24_IE_OBJ_4S),
    (A2D_M24_IE_SAMP_FREQ_8  | A2D_M24_IE_SAMP_FREQ_11 |    /* Sampling frequency */
     A2D_M24_IE_SAMP_FREQ_12 | A2D_M24_IE_SAMP_FREQ_16 |
     A2D_M24_IE_SAMP_FREQ_22 | A2D_M24_IE_SAMP_FREQ_24 |
     A2D_M24_IE_SAMP_FREQ_32 | A2D_M24_IE_SAMP_FREQ_44 |
     A2D_M24_IE_SAMP_FREQ_48 | A2D_M24_IE_SAMP_FREQ_64 |
     A2D_M24_IE_SAMP_FREQ_88 | A2D_M24_IE_SAMP_FREQ_96),
    (A2D_M24_IE_CHNL_1 | A2D_M24_IE_CHNL_2),                /* Channel mode */
    TRUE,                                                   /* Variable Bit Rate */
    A2D_M24_IE_BITRATE_MSK                                  /* Bit rate index */
};
#endif

/*****************************************************************************
**  Local data
*****************************************************************************/
/* timestamp */
// static UINT32 bta_aa_co_timestamp;
tA2D_SBC_CIE bta_av_co_open_cie;
/* peer configuration (get config) */
// static UINT8  bta_aa_co_sep_info_idx;   /* sep_info_idx as reported/used in the bta_aa_co_getconfig */
/* for incoming connections */
static UINT8  bta_av_co_inc = 0;
// static UINT8  bta_aa_co_curr_codec_info[AVDT_CODEC_SIZE]; /* current configuration for incoming connection */

UINT8 default_bta_av_co_peer_m12_info[AVDT_CODEC_SIZE]={
    A2D_M12_INFO_LEN,         /* SBC Info length */
    AVDT_MEDIA_AUDIO,         /* Media Type */
    A2D_MEDIA_CT_M12,         /* Codec Type */
    0x34,0x02,0x88,0x00,      /* Codec Info */
    0,0,0};

/************************************************************
** Sampling Frequency - 16/32/44.1/48kHz,
** Channel Mode - Mono/Dual Channel/Stereo/Joint Stereo
** Block Length - 4/8/12/16
** Subbands - 4/8
** Allocation Method - SNR/Loudness
** Minimum Bitpool Value - 2
** Maximum Bitpool Value - 32
*************************************************************/
UINT8 default_bta_av_co_peer_sbc_info[AVDT_CODEC_SIZE]={
    A2D_SBC_INFO_LEN,  /* SBC Info length */
    AVDT_MEDIA_AUDIO,  /* Media Type */
    A2D_MEDIA_CT_SBC,  /* Codec Type */
    0xFF,0xFF,2,32,    /* Codec Info */
    0,0,0};
/************************************************************
** Sampling Frequency-44.1kHz
** Channel Mode-Joint Stereo
** Block Length-16
** Subbands-8
** Allocation Method-Loudness
** Minimum Bitpool Value - 2
** Maximum Bitpool Value - 32
*************************************************************/
UINT8 default_bta_av_co_current_codec_info[AVDT_CODEC_SIZE]={
    A2D_SBC_INFO_LEN,  /* SBC Info length */
    AVDT_MEDIA_AUDIO,  /* Media Type */
    A2D_MEDIA_CT_SBC,  /* Codec Type */
    0x21,0x15,2,32,    /* Codec Info */
    0,0,0};

typedef struct
{
    UINT8           codec_info[AVDT_CODEC_SIZE]; /* peer SEP configuration */
    UINT8           seid;
    UINT8           sep_idx;
} tBTA_AV_CO_SEP;

typedef struct
{
    BD_ADDR         addr; /* address of audio SNK */
    tBTA_AV_CO_SEP  sep[BTA_AV_CO_MAX_SEPS];
    UINT8           num_seps;   /* total number of seps at peer */
    UINT8           num_snk;    /* total number of snk at peer */
    UINT8           num_good;   /* total number of snk that we are interested */
    UINT8           idx;    /* the index of sep[] to store info next */
    UINT8           use;    /* the index of sep[] used for streaming */
} tBTA_AV_CO_SNK;

tBTA_AV_CO_SNK bta_av_co_snk[BTA_AV_CO_MAX_SNKS];
UINT8 bta_av_co_snk_idx = 0;
UINT8 bta_av_co_num_snk = 0;
UINT8 bta_av_co_sep_info_idx_temp = 0;

/*****************************************************************************
**  External Functions
*****************************************************************************/
extern tBTUI_AV_CB btui_av_cb;

extern BT_HDR *btui_codec_readbuf(tBTA_AV_CODEC codec_type);
extern void bta_av_co_dump_pcm(char *p_path, UINT8 *p_buf, UINT16 len);

extern int bdcmp(const BD_ADDR a, const BD_ADDR b);
extern void bdcpy(BD_ADDR a, const BD_ADDR b);
extern void btui_av_sbc_codec_init(UINT8 *p_codec_info, UINT16 mtu);
extern void btui_av_m12_codec_init(UINT8 *p_codec_info, UINT16 mtu);
extern UINT8 bta_av_m12_cfg_in_cap(UINT8 *p_cfg, tA2D_M12_CIE *p_cap);


/*******************************************************************************
**
** Function         bta_av_media_type
**
** Description
**
**
** Returns          char *
**
*******************************************************************************/
static char *bta_av_media_type(UINT8 media_type)
{
    switch(media_type)
    {
    case AVDT_MEDIA_AUDIO:  return "Audio SEP";
    case AVDT_MEDIA_VIDEO:  return "Video SEP";
    case AVDT_MEDIA_MULTI:  return "Multimedia SEP";
    default: return"Unknown Media Type";
    }
}

/*******************************************************************************
**
** Function         bta_av_codec_type
**
** Description
**
**
** Returns          char *
**
*******************************************************************************/
static char *bta_av_codec_type(tBTA_AV_CODEC codec_type)
{
    switch(codec_type)
    {
    case BTA_AV_CODEC_SBC:  return "SBC";
    case BTA_AV_CODEC_M12:  return "MPEG-1,2";
    case BTA_AV_CODEC_M24:  return "MPEG-2,4";
    case BTA_AV_CODEC_ATRAC:  return "ATRAC Family";
    default: return"Unknown Codec Type";
    }
}

/*******************************************************************************
**
** Function         bta_av_co_get_sep_info_idx
**
** Description      find the sep_info_idx for the stream with the given codec type.
**                  need this for reconfig
**
** Returns          sep_info_idx
**
*******************************************************************************/
UINT8 bta_av_co_get_sep_info_idx(UINT8 codec_type)
{
    UINT8 sep_info_idx = 0;
    int     xx;
    tBTA_AV_CO_SNK *p_snk = &bta_av_co_snk[bta_av_co_snk_idx]; /* need to find an unused one */
    for(xx=0; xx<p_snk->num_good; xx++)
    {
        if(p_snk->sep[xx].codec_info[BTA_AV_CO_CODEC_TYPE_IDX] == codec_type)
        {
            APPL_TRACE_DEBUG2("bta_av_co_get_sep_info_idx [%d] sep_info_idx: %d", xx, p_snk->sep[xx].sep_idx);
            sep_info_idx = p_snk->sep[xx].sep_idx;
            break;
        }
    }
    return sep_info_idx;
}

/*******************************************************************************
**
** Function         bta_av_co_audio_drop
**
** Description      An Audio packet is dropped. .
**                  It's very likely that the connected headset with this handle
**                  is moved far away. The implementation may want to reduce
**                  the encoder bit rate setting to reduce the packet size.
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_drop(tBTA_AV_HNDL hndl)
{
    APPL_TRACE_ERROR1("bta_av_co_audio_drop dropped: x%x", hndl);
}

/*******************************************************************************
**
** Function         bta_av_co_audio_delay
**
** Description      This function is called by AV when the audio stream connection
**                  needs to send the initial delay report to the connected SRC.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_delay(tBTA_AV_HNDL hndl, UINT16 delay)
{
    APPL_TRACE_ERROR2("bta_av_co_audio_delay handle: x%x, delay:0x%x", hndl, delay);
}

/*******************************************************************************
**
** Function         bta_av_audio_codec_callback
**
** Description      Callback for btui codec.
**                  Notify all channels that audio data is ready
**
** Returns          void
**
*******************************************************************************/
void bta_av_audio_codec_callback(UINT8 event)
{
    if(event == BTUI_CODEC_RX_READY)
    {
        bta_av_ci_src_data_ready(BTA_AV_CHNL_AUDIO);
    }
}

/*******************************************************************************
**
** Function         bta_av_co_audio_init
**
** Description      This callout function is executed by AA when it is
**                  started by calling BTA_AvRegister().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.
**
**
** Returns          Stream codec and content protection capabilities info.
**
*******************************************************************************/
BOOLEAN bta_av_co_audio_init(UINT8 *p_codec_type, UINT8 *p_codec_info,
                                   UINT8 *p_num_protect, UINT8 *p_protect_info, UINT8 index)
{
    int i;
    BOOLEAN ret = FALSE;
    tA2D_STATUS status;

    APPL_TRACE_DEBUG1("bta_av_co_audio_init: %d", index);

    /* set up for SBC codec */
    switch(index/*btui_av_cb.pref_codec*/)
    {
    case BTUI_AV_SBC_INDEX: /* SBC */
        {
        for(i=0; i<BTA_AV_CO_MAX_SNKS; i++)
            bta_av_co_snk[i].num_good = 0;

        /* initialize SBC codec preferences */
        bta_av_co_sbc_pref.ch_mode = BTUI_AV_CHANNEL_MODE; /* initialize SBC codec preferences */

        /* set up for SBC codec */
        *p_codec_type = BTA_AV_CODEC_SBC/*btui_av_cb.pref_codec*/;
        status = A2D_BldSbcInfo(AVDT_MEDIA_AUDIO, (tA2D_SBC_CIE *) & bta_av_co_sbc_cap, p_codec_info);
        if(status != A2D_SUCCESS)
        {
            APPL_TRACE_ERROR1("bta_av_co_audio_init ERROR -> A2D_BldSbcInfo failed %d", status);
        }
        else
        {

            memcpy(btui_av_cb.peer_sbc_codec_info, p_codec_info, AVDT_CODEC_SIZE);

            APPL_TRACE_DEBUG6("  bta_av_co_audio_init Codec Info: %0x:%0x:%0x:%0x:%0x:%0x",
            p_codec_info[1],p_codec_info[2],p_codec_info[3],
            p_codec_info[4],p_codec_info[5],p_codec_info[6]);

            btui_codec_init();

            ret = TRUE;
        }
        }break;

    case BTA_AV_CODEC_M12:
	{
#if (BTUI_AV_M12_SUPPORT == TRUE)
      // APPL_TRACE_DEBUG0("Support MPEG-1,2 Audio");

        *p_codec_type = BTA_AV_CODEC_M12/*btui_av_cb.pref_codec*/;

        status = A2D_BldM12Info(AVDT_MEDIA_AUDIO, (tA2D_M12_CIE *)&bta_av_co_m12_cap, p_codec_info);
#if 1
             APPL_TRACE_DEBUG6("bta_av_co_audio_init m12 [%x;%x;%x;%x;%x]", \
             p_codec_info[1], p_codec_info[2], p_codec_info[3],  p_codec_info[4],  p_codec_info[5], p_codec_info[6]);
#endif

      if(status != A2D_SUCCESS)
        {
           ; // APPL_TRACE_ERROR1("ERROR bta_aa_co_init->A2D_BldM12Info failed %d", status);
        }
        else
        {

            memcpy(p_codec_info, default_bta_av_co_peer_m12_info, AVDT_CODEC_SIZE);
            memcpy(btui_av_cb.cur_codec_info, default_bta_av_co_peer_m12_info, AVDT_CODEC_SIZE);
            memcpy(btui_av_cb.peer_m12_codec_info, default_bta_av_co_peer_m12_info, AVDT_CODEC_SIZE);

            APPL_TRACE_DEBUG6("  bta_av_co_audio_init Codec Info: %0x:%0x:%0x:%0x:%0x:%0x",
            p_codec_info[1],p_codec_info[2],p_codec_info[3],
            p_codec_info[4],p_codec_info[5],p_codec_info[6]);
            ret = TRUE;
        }
#endif
	}break;
#if (BTUI_AV_M24_SUPPORT == TRUE)
    case BTA_AV_CODEC_M24:
    {
        /* set up for MP3 codec */
        *p_codec_type = BTA_AV_CODEC_M24;
        A2D_BldM24Info(AVDT_MEDIA_AUDIO, (tA2D_M24_CIE *) &bta_av_co_m24_cap, p_codec_info);
#if 1
        APPL_TRACE_DEBUG6("bta_av_co_audio_init m24 [%x;%x;%x;%x;%x]", \
        p_codec_info[1], p_codec_info[2], p_codec_info[3],  p_codec_info[4],  p_codec_info[5], p_codec_info[6]);
#endif
        ret = TRUE;
    }break;
#endif
    case BTA_AV_CODEC_ATRAC:
    {
        // APPL_TRACE_DEBUG0("Now we don't support ATRAC family");
    }break;
    default:
        break;
    }

    /* content protection info for test purposes */
    *p_num_protect = 0;
    *p_protect_info++ = 0; /* length */
    //memcpy(p_protect_info, "123456", 6);



    return ret;
}

/*******************************************************************************
**
** Function         bta_av_co_audio_disc_res
**
** Description      This callout function is executed by AV to report the
**                  number of stream end points (SEP) were found during the
**                  AVDT stream discovery process.
**
**
** Returns          void.
**
*******************************************************************************/
void bta_av_co_audio_disc_res(tBTA_AV_HNDL hndl, UINT8 num_seps,
                                             UINT8 num_snk, BD_ADDR addr)
{
    int i;
    tBTA_AV_CO_SNK *p_snk;

    APPL_TRACE_DEBUG2("bta_av_co_audio_disc_res [num_seps=%d, num_snk=%d]", num_seps, num_snk);

//    btui_av_cb.num_seps = num_seps;
//    btui_av_cb.num_snk = num_snk;

    /* do nothing here, if only SBC is supported */
    /* otherwise, keep num_seps and num_snk in the application control block */
    if(bta_av_co_inc)
    {
        /* incoming - bta_aa_co_setconfig() made sure bta_av_co_snk_idx is right */
        p_snk = &bta_av_co_snk[bta_av_co_snk_idx];
        APPL_TRACE_DEBUG2("bta_av_co_audio_disc_res in :%d good: %d",  bta_av_co_snk_idx, p_snk->num_good);
        /* we gave the real num_good to AA.
         * need to reset this to 0 to increase in getconfig */
        p_snk->num_good = 0;

    }
    else
    {
        /* outgoing */
        for(i=0; i<BTA_AV_CO_MAX_SNKS; i++)
        {
            p_snk = &bta_av_co_snk[i];
            if( (p_snk->num_good && bdcmp(p_snk->addr, addr) == 0) ||
                (p_snk->num_good == 0) )
            {
                APPL_TRACE_DEBUG2("bta_av_co_audio_disc_res out :%d good: %d",  i, p_snk->num_good);
                bta_av_co_snk_idx = i;
                p_snk->num_seps = num_seps;
                p_snk->num_snk  = num_snk;
                p_snk->num_good = 0;
                bdcpy(p_snk->addr, addr);
                p_snk->idx = 0;
                p_snk->use = 0;
                break;
            }
        }
    }

}

/*******************************************************************************
**
** Function         bta_av_co_audio_getconfig
**
** Description      This callout function is executed by AV to retrieve the
**                  desired codec and content protection configuration for the
**                  audio stream.
**
**
** Returns          Stream codec and content protection configuration info.
**
*******************************************************************************/
UINT8 bta_av_co_audio_getconfig(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                                         UINT8 *p_codec_info, UINT8 *p_sep_info_idx, UINT8 seid,
                                         UINT8 *p_num_protect, UINT8 *p_protect_info)
{
    UINT8 status = A2D_FAIL;
    tBTA_AV_CO_SNK *p_snk = &bta_av_co_snk[bta_av_co_snk_idx]; /* need to find an unused one */
    tBTA_AV_CO_SEP *p_sep = &p_snk->sep[p_snk->idx];
#if 1
    APPL_TRACE_DEBUG1("bta_av_co_audio_getconfig : %s", bta_av_media_type(p_codec_info[1]));
    APPL_TRACE_DEBUG2("bta_av_co_audio_getconfig  Codec Type : [%d]%s", codec_type, bta_av_codec_type(codec_type));
    APPL_TRACE_DEBUG6("    %0x:%0x:%0x:%0x:%0x:%0x", p_codec_info[1],p_codec_info[2],p_codec_info[3],\
                                                    p_codec_info[4],p_codec_info[5],p_codec_info[6]);
#endif
    /* ignore content protection for now */
    *p_num_protect = 0;

    /* if more than one codec type is supported, the application needs to keep
     * candidate *p_sep_info_idx and p_codec_info[] in its control block.
     * at the last report (num_snk times) of configuration
     * or when the preferred configuration
     * is found, copy the decided *p_sep_info_idx and p_codec_info[] back to AA
     * and return 0.
     * otherwise return 1 (non-zero) to tell AA to keep getting the configuration
     * from headset.
     */

    /* remember the peer codec info - capabilities */
    memcpy(p_sep->codec_info, p_codec_info, AVDT_CODEC_SIZE);

    /* get codec configuration */
    switch(codec_type)
    {
    case BTA_AV_CODEC_SBC:
    {
        /* get codec configuration; this also verifies that codec is SBC */
        status = bta_av_sbc_cfg_for_cap(p_codec_info, (tA2D_SBC_CIE *) & bta_av_co_sbc_cap, (tA2D_SBC_CIE *) & bta_av_co_sbc_pref);

        /* the preferred is SBC in this case */
        p_snk->use = p_snk->idx;
        bta_av_co_sep_info_idx_temp = p_sep->sep_idx + 1;

        /* check preferred codec and keep it */
        if(btui_av_cb.pref_codec == BTA_AV_CODEC_SBC)
            btui_av_cb.sep_info_idx = *p_sep_info_idx;

        /* remember the peer sbc codec info */
        memcpy(btui_av_cb.peer_sbc_codec_info, p_codec_info, AVDT_CODEC_SIZE);
        APPL_TRACE_DEBUG5("bta_av_co_audio_getconfig  ***HLONG0: %d %d=%d=%d=%d", status, p_sep->sep_idx,p_snk->num_seps,*p_sep_info_idx, p_snk->num_snk );
    }break;
    case BTA_AV_CODEC_M12:
    {
#if 0 /* HLONG */
        status = A2D_SUCCESS;
        btui_av_cb.m12_sep_idx = *p_sep_info_idx;

        p_snk->use = p_snk->idx;
        bta_av_co_sep_info_idx_temp = p_sep->sep_idx + 1;

        /* check preferred codec and keep it */
        if(btui_av_cb.pref_codec == BTA_AV_CODEC_M12)
            btui_av_cb.sep_info_idx = *p_sep_info_idx;

        /* remember the peer sbc codec info */
        memcpy(btui_av_cb.peer_m12_codec_info, p_codec_info, AVDT_CODEC_SIZE);

        APPL_TRACE_DEBUG4("bta_av_co_audio_getconfig  HLONG0: %d=%d=%d=%d", p_sep->sep_idx,p_snk->num_seps,*p_sep_info_idx, p_snk->num_snk );
#else
        APPL_TRACE_DEBUG1("bta_av_co_audio_getconfig  unsuported codec_type %d", codec_type);
        status = A2D_FAIL;
#endif
    }break;
    case BTA_AV_CODEC_M24:
    {
        APPL_TRACE_DEBUG1("bta_av_co_audio_getconfig  unsuported codec_type %d", codec_type);
        APPL_TRACE_DEBUG4("bta_av_co_audio_getconfig  HLONG0: %d=%d=%d=%d", p_sep->sep_idx,p_snk->num_seps,*p_sep_info_idx, p_snk->num_snk );
        status = A2D_FAIL;
    }break;
    case BTA_AV_CODEC_ATRAC:
    {
        APPL_TRACE_DEBUG1("bta_aa_co_getconfig unsuported codec_type %d", codec_type);
        status = A2D_FAIL;
    }break;
    default:
    {
        status = A2D_FAIL;
    }break;
    }


    /* If codec is supported, then store info */
    if(status == A2D_SUCCESS)
    {
        APPL_TRACE_DEBUG3("bta_av_co_audio_getconfig [%d] codec_type: %d sep_info_idx: %d", p_snk->idx, codec_type, *p_sep_info_idx);
        p_snk->num_good++;
        p_snk->idx++;
        /* we like this one. store info */
        p_sep->sep_idx  = *p_sep_info_idx;
        p_sep->seid     = seid;

        /*
         * Made this change because a SNK can support more than one tsep
         * some SNK devices have many tseps - so if this happens then this
         * check will fail - changed it to compare against num_seps instead
         * of num_snks.
         */
#if 0
        if(p_sep->sep_idx < (p_snk->num_snk - 1))
#else
        if(p_sep->sep_idx < (p_snk->num_seps - 1))
#endif

        {
            /* make it continue */
//            status = A2D_FAIL;
            p_snk->num_seps = p_sep->sep_idx + 1;
        }
    }

    /* If this is the last codec that the sink supports, and we found one that we support... */
    // changed by KBNAM : code chagne from BTEM 10.2.2.0
    // if(bta_av_co_sep_info_idx_temp && p_sep->sep_idx == (p_snk->num_snk - 1))
    if(bta_av_co_sep_info_idx_temp && (p_sep->sep_idx == (p_snk->num_seps - 1)))
    //if(bta_av_co_sep_info_idx_temp && (*p_sep_info_idx == 2))
    {
        /* this is the last one */
        status = A2D_SUCCESS;
        *p_sep_info_idx = btui_av_cb.sep_info_idx;
        bta_av_co_sep_info_idx_temp = 0;

        APPL_TRACE_DEBUG1("bta_av_co_audio_getconfig : Setting codec_type as %s", bta_av_codec_type(codec_type));

        switch(btui_av_cb.pref_codec)
        {
        case BTA_AV_CODEC_SBC:
        {
            btui_av_cb.str_cfg = BTUI_AV_STR_CFG_WAV_2_SBC;

            memcpy(btui_av_cb.cur_codec_info, btui_av_cb.peer_sbc_codec_info, AVDT_CODEC_SIZE);
            memcpy(p_codec_info, btui_av_cb.peer_sbc_codec_info, AVDT_CODEC_SIZE);
        }break;
        case BTA_AV_CODEC_M12:
        {
            btui_av_cb.str_cfg = BTUI_AV_STR_CFG_MP3_2_MP3;

            memcpy(p_codec_info, default_bta_av_co_peer_m12_info, AVDT_CODEC_SIZE);
            memcpy(btui_av_cb.cur_codec_info, default_bta_av_co_peer_m12_info, AVDT_CODEC_SIZE);
        }break;
        case BTA_AV_CODEC_M24:
        case BTA_AV_CODEC_ATRAC:
        default:
        {
            APPL_TRACE_DEBUG0("bta_av_co_audio_getconfig : Unsupported codec_type!!!");
            status = A2D_WRONG_CODEC;
            return status;
        }break;
        }

        status = A2D_SUCCESS;
    }
    else
    {
        /* To get next SEP information */
        status = A2D_FAIL;
    }

    if(bta_av_co_inc)
        bta_av_co_inc++;

    return status;
}

/*******************************************************************************
**
** Function         bta_av_co_audio_setconfig
**
** Description      This callout function is executed by AV to set the codec and
**                  content protection configuration of the audio stream.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_setconfig(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                               UINT8 *p_codec_info, UINT8 seid, BD_ADDR addr,
                               UINT8 num_protect, UINT8 *p_protect_info)
{
    UINT8 status = A2D_FAIL;
    UINT8 category = 0;
    UINT8 num = 0, count = 0;
    UINT8 seids[BTA_AV_CO_MAX_SEPS]={0,};
    int i, j;

#if 1
    APPL_TRACE_DEBUG1("bta_av_co_audio_setconfig: %s", bta_av_media_type(p_codec_info[1]));
    APPL_TRACE_DEBUG2("  bta_av_co_audio_setconfig Codec Type : [%d]%s", codec_type, bta_av_codec_type(codec_type));
    APPL_TRACE_DEBUG6("    %0x:%0x:%0x:%0x:%0x:%0x", p_codec_info[1],p_codec_info[2],p_codec_info[3],\
                                                    p_codec_info[4],p_codec_info[5],p_codec_info[6]);
#endif
    /* verify configuration is acceptable */
    switch(codec_type)
    {
    case BTA_AV_CODEC_SBC:
        if ((status = bta_av_sbc_cfg_in_cap(p_codec_info, (tA2D_SBC_CIE *) &bta_av_co_sbc_cap)) != 0)
        {
            category = AVDT_ASC_CODEC;
        }

    /* do not allow content protection for now */
        if (num_protect != 0)
        {
            status = A2D_BAD_CP_TYPE;
            category = AVDT_ASC_PROTECT;
        }
        break;

#if (BTUI_AV_M12_SUPPORT == TRUE)
    case BTA_AV_CODEC_M12:
        /* verify MP3 configuration */
        if ((status = bta_av_m12_cfg_in_cap(p_codec_info,(tA2D_M12_CIE *) &bta_av_co_m12_cap)) != 0)
        {
            category = AVDT_ASC_CODEC;
        }
        break;
#endif

#if (BTUI_AV_M24_SUPPORT == TRUE)
    case BTA_AV_CODEC_M24:
        /* verify AAC configuration */
        status = A2D_SUCCESS;
        break;
#endif

    default:
        break;
    }

    memcpy(btui_av_cb.cur_codec_info, p_codec_info, AVDT_CODEC_SIZE);
    bta_av_co_inc = 1;
    btui_av_cb.sbc_sep_idx = 0;

    /* find the associated bta_av_co_snk[] by addr */
    for(i=0; i<BTA_AV_CO_MAX_SNKS; i++)
    {
        if(bdcmp(bta_av_co_snk[i].addr, addr) == 0)
        {
            APPL_TRACE_DEBUG1("bta_av_co_audio_getconfig idx: %d", i);
            bta_av_co_snk_idx = i;
            num = bta_av_co_snk[i].num_good;
            bta_av_co_snk[i].idx = 0;
            /* find the seid list */
            count = 0;
            for(j=0; j<num; j++)
            {
                if(seid != bta_av_co_snk[i].sep[j].seid)
                {
                    APPL_TRACE_DEBUG2("bta_av_co_audio_getconfig seid[%d]: %d", count, bta_av_co_snk[i].sep[j].seid);
                    seids[count++] = bta_av_co_snk[i].sep[j].seid;
                }
            }
            break;
        }
    }

    /* call call-in */

    bta_av_ci_setconfig(btui_av_cb.hndl, status, category, count, seids, TRUE);
}


/*******************************************************************************
**
** Function         bta_av_co_audio_open
**
** Description      This function is called by AV when the audio stream connection
**                  is opened.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_open(tBTA_AV_HNDL hndl,
                                         tBTA_AV_CODEC codec_type, UINT8 *p_codec_info,
                                         UINT16 mtu)
{
    // tBTUI_CODEC_CFG cfg;

#if 1
    APPL_TRACE_DEBUG1("bta_av_co_audio_open mtu:%d", mtu);
    APPL_TRACE_DEBUG2("bta_av_co_audio_open codec_type: [%d]%s", codec_type, bta_av_codec_type(codec_type));
    APPL_TRACE_DEBUG6("    %0x:%0x:%0x:%0x:%0x:%0x", p_codec_info[1],p_codec_info[2],p_codec_info[3], \
                                                    p_codec_info[4],p_codec_info[5],p_codec_info[6]);
#endif

    memcpy(btui_av_cb.cur_codec_info, p_codec_info, AVDT_CODEC_SIZE);

    if(bta_av_co_inc)
        bta_av_co_inc++;

    btui_av_cb.timestamp = 0;

    switch(codec_type)
    {
    case BTA_AV_CODEC_SBC:
    {
        btui_av_sbc_codec_init(p_codec_info, mtu);
    }break;
#if (BTUI_AV_M12_SUPPORT == TRUE)
    case BTA_AV_CODEC_M12:
    {
        btui_av_m12_codec_init(p_codec_info, mtu);
    }break;
#endif
#if (BTUI_AV_M24_SUPPORT == TRUE)
    case BTA_AV_CODEC_M24:
    {
    }break;
#endif
#if (BTUI_AV_ATRAC_SUPPORT == TRUE)
    case BTA_AV_CODEC_ATRAC:
    {
    }break;
#endif
    default:
    {
    }break;
    }
}

/*******************************************************************************
**
** Function         bta_av_co_audio_close
**
** Description      This function is called by AV when the audio stream connection
**                  is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_close(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type, UINT16 mtu)
{
    APPL_TRACE_DEBUG2("bta_av_co_audio_close: codec_type [%d]%s", codec_type, bta_av_codec_type(codec_type));

    bta_av_co_inc = 0;

    btui_codec_close(codec_type);
}

/*******************************************************************************
**
** Function         bta_av_co_audio_start
**
** Description      This function is called by AV when the audio streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_start(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type)
{
    //UINT8           pref_codec_info[AVDT_CODEC_SIZE];
    // BT_HDR *p_msg;

    APPL_TRACE_DEBUG2("bta_av_co_audio_start: codec_type [%d]%s", codec_type, bta_av_codec_type(codec_type));

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
    /* Disable LPM */
    if(bte_appl_cfg.lpm_enabled == 1)
    {
        LOGE("Disabling LPM");
        HCILP_WakeupBTDevice(NULL);         /* Wake sure BT_WAKE is asserted */
        btapp_HCILP_Enable(FALSE, NULL);	/* Disable LPM */
    }
#endif

    switch(codec_type)
    {
    case BTA_AV_CODEC_SBC:
#if 0
        if(bta_av_co_inc >= 3)
        {
            bta_av_co_open_cie.samp_freq = bta_av_co_sbc_pref.samp_freq;
            A2D_BldSbcInfo(AVDT_MEDIA_AUDIO, (tA2D_SBC_CIE *) &bta_av_co_open_cie, pref_codec_info);
            if(memcmp(pref_codec_info, btui_av_cb.cur_codec_info, A2D_SBC_INFO_LEN))
            {
                memcpy(btui_av_cb.cur_codec_info, pref_codec_info, A2D_SBC_INFO_LEN);
                //BTA_AvReconfig(FALSE, btui_av_cb.sbc_sep_idx, pref_codec_info, 0, NULL);
                btui_av_send_evt_4_aa(BTUI_AV_DO_RECONFIG_EVT);
                return;
            }
        }
#endif
        btui_codec_start(codec_type);
        break;

#if (BTUI_AV_M12_SUPPORT == TRUE)
    case BTA_AV_CODEC_M12:
        btui_codec_start(codec_type);
        break;
#endif

#if (BTUI_AV_M24_SUPPORT == TRUE)
    case BTA_AV_CODEC_M24:
        break;
#endif

    default:
        break;
    }
}

/*******************************************************************************
**
** Function         bta_av_co_audio_stop
**
** Description      This function is called by AV when the audio streaming data
**                  transfer is stopped.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_audio_stop(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type)
{
    APPL_TRACE_DEBUG2("bta_av_co_audio_stop : codec_type [%d]%s", codec_type, bta_av_codec_type(codec_type));

    /* stop codec */
    btui_codec_stop(codec_type);

#if (defined(HCILP_INCLUDED) && HCILP_INCLUDED == TRUE)
   /* Re-enable LPM if needed */
    if(bte_appl_cfg.lpm_enabled == 1)
    {
        LOGE("Re-enabling LPM");
        btapp_HCILP_Enable(TRUE, NULL);	/* Enable LPM */
    }
#endif
  }

/*******************************************************************************
**
** Function         bta_av_co_audio_src_data_path
**
** Description      This function is called to manage data transfer from
**                  the audio codec to AVDTP.
**
** Returns          TRUE if AVDT_WriteReq called, false otherwise.
**
*******************************************************************************/
void * bta_av_co_audio_src_data_path(tBTA_AV_CODEC codec_type,
                                     UINT32 *p_len, UINT32 *p_timestamp)
{

    BT_HDR  *p_buf;

/*    APPL_TRACE_DEBUG0("bta_av_co_audio_src_data_path"); */

    p_buf = btui_codec_readbuf(codec_type);
    if (p_buf != NULL)
    {
         bta_av_sbc_bld_hdr(p_buf, p_buf->layer_specific);         /* set up packet header */

        /* increment timestamp; increment value is in BT_HDR event field */
        *p_timestamp = btui_av_cb.timestamp;
        *p_len       = p_buf->len;

        /* BTA calls AVDT_WriteReq to send the packet */
        btui_av_cb.timestamp += p_buf->event;
    }

    return p_buf;
}

/*******************************************************************************

**
** Function         bta_av_co_video_report_conn
**
** Description      This function is called by AV when the reporting channel is
**                  opened (open=TRUE) or closed (open=FALSE).
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_report_conn (BOOLEAN open, UINT8 avdt_handle)
{
}

#if defined(BTUI_AV_PCM_DUMP) && (BTUI_AV_PCM_DUMP == 1)

/*******************************************************************************
**
** Function         bta_av_co_dump_pcm
**
** Description      This function is called to dump pcm data before call AVDT_WriteReq
**                  It must be used test purpose.
**
** Returns
**
*******************************************************************************/
void bta_av_co_dump_pcm(char *p_path, UINT8 *p_buf, UINT16 len)
{

#if defined(BTUI_AV_PCM_DUMP) && (BTUI_AV_PCM_DUMP == 1)
    extern btapp_flash_aa_dump_sbc(char *p_path, UINT8 *p_buf, UINT16 len);

    btapp_flash_aa_dump_sbc(p_path, p_buf, len);
#else
    fs_open_rsp_type     open_rsp;
    fs_rsp_msg_type      seek_resp;
    fs_write_rsp_type    write_rsp;
    fs_close_rsp_type   close_rsp;
    int fd;

    fs_open(p_path, FS_OA_CREATE, NULL,NULL, (fs_rsp_msg_type *) &open_rsp);

    if (open_rsp.status == FS_FAIL_S) return;

    /* if file already exists try read/write */
    if (open_rsp.status == FS_FILE_ALREADY_EXISTS_S)
    {
        fs_open(p_path, FS_OA_READWRITE, NULL, NULL, (fs_rsp_msg_type *) &open_rsp);
    }

    fd = (int) open_rsp.handle;

    fs_seek((word) fd, FS_SEEK_EOF, 0, NULL, &seek_resp);


    fs_write((word) fd, (void *) p_buf,(dword) len, NULL, (fs_rsp_msg_type *) &write_rsp);

    fs_close((word) fd, NULL, (fs_rsp_msg_type *) &close_rsp);
#endif


}
#endif


/*******************************************************************************
**
** Function         bta_av_co_video_init
**
** Description      This callout function is executed by AV when it is
**                  started by calling BTA_AvRegister().  This function can be
**                  used by the phone to initialize video paths or for other
**                  initialization purposes.
**
**
** Returns          Stream codec and content protection capabilities info.
**
*******************************************************************************/
BOOLEAN bta_av_co_video_init(UINT8 *p_codec_type, UINT8 *p_codec_info,
                    UINT8 *p_num_protect, UINT8 *p_protect_info, UINT8 index)
{
   return 0;
}

/*******************************************************************************
**
** Function         bta_av_co_video_disc_res
**
** Description      This callout function is executed by AV to report the
**                  number of stream end points (SEP) were found during the
**                  AVDT stream discovery process.
**
**
** Returns          void.
**
*******************************************************************************/
void bta_av_co_video_disc_res(tBTA_AV_HNDL hndl, UINT8 num_seps,
                              UINT8 num_snk, BD_ADDR addr)
{
    /* do nothing here, if only H.263 is supported */
    /* otherwise, keep num_seps in the application control block */
}

/*******************************************************************************
**
** Function         bta_av_co_video_getconfig
**
** Description      This callout function is executed by AV to retrieve the
**                  desired codec and content protection configuration for the
**                  video stream.
**
**
** Returns          Stream codec and content protection configuration info.
**
*******************************************************************************/
UINT8 bta_av_co_video_getconfig(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                         UINT8 *p_codec_info, UINT8 *p_sep_info_idx, UINT8 seid,
                         UINT8 *p_num_protect, UINT8 *p_protect_info)
{
#if 0
    UINT8 status = 0; /* success */
    UINT8 levels_mask;

    APPL_TRACE_DEBUG0("bta_av_co_video_getconfig");
    /* if more than one codec type is supported, the application needs to keep
     * candidate *p_sep_info_idx and p_codec_info[] in its control block.
     * at the last report of configuration or when the preferred configuration
     * is found, copy the decided *p_sep_info_idx and p_codec_info[] back to AV
     * and return 0.
     * otherwise return 1 (non-zero) to tell AV to keep getting the configuration
     * from headset.
     */

    /* remember the peer codec info */
    memcpy(bta_av_co_video_cb.peer_codec_info, p_codec_info, AVDT_CODEC_SIZE);
    if (codec_type == bta_av_co_h263_pref.codec_type)
    {
        if (VDP_GetCodecInfo(p_codec_info, NULL, NULL, &levels_mask) == VDP_SUCCESS)
        {
            levels_mask &= bta_av_co_h263_cap.levels;
            if (levels_mask)
            {
                if (levels_mask & bta_av_co_h263_pref.levels)
                    levels_mask = bta_av_co_h263_pref.levels;
                else
                {
                    /* the preferred level is not supported by peer.
                     * pick one from the capabilities */
                    if (A2D_BitsSet(levels_mask) != A2D_SET_ONE_BIT)
                    {
                        levels_mask = VDP_H263_IE_LEVEL20;
                    }
                }
            }
            else
                status = VDP_NS_LEVEL;
        }
        else
            status = VDP_INVALID_PARAMS;
    }
    else
        status = VDP_NS_CODEC_TYPE;

    if (status == 0)
    {
        VDP_SetCodecInfo(p_codec_info, AVDT_MEDIA_VIDEO,
                    bta_av_co_h263_pref.codec_type, levels_mask);
        bta_av_co_video_cb.sep_info_idx = *p_sep_info_idx;
    }

    /* ignore content protection for now */
    *p_num_protect = 0;

    return status;
#endif
    return 0;
}

/*******************************************************************************
**
** Function         bta_av_co_video_setconfig
**
** Description      This callout function is executed by AV to set the codec and
**                  content protection configuration of the video stream.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_setconfig(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type,
                               UINT8 *p_codec_info, UINT8 seid, BD_ADDR addr,
                         UINT8 num_protect, UINT8 *p_protect_info)
{
#if 0
    UINT8 status = 0;
    UINT8 category = 0;
    UINT8 levels_mask;
    UINT8 num = 0, count = 0;
    UINT8 seids[BTA_AV_CO_MAX_SEPS];
    tBTA_AV_CO_SNK *p_snk = bta_av_co_get_snk(addr, bta_av_co_video_cb.snk);
    int j;
    BOOLEAN good = FALSE;

    APPL_TRACE_DEBUG0("bta_av_co_video_setconfig");
#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    good = btui_video_read_cfg();
#endif

    /* verify configuration is acceptable */
    if (codec_type == bta_av_co_h263_pref.codec_type)
    {
        status = VDP_GetCodecInfo(p_codec_info, NULL, NULL, &levels_mask);
        if (status == VDP_SUCCESS)
        {
            levels_mask &= bta_av_co_h263_cap.levels;
            if (levels_mask == 0)
                status = VDP_INVALID_LEVEL;
        }
    }
    else
        status = VDP_BAD_CODEC_TYPE;

    if (status != 0)
        category = AVDT_ASC_CODEC;

     /* do not allow content protection for now */
    if (num_protect != 0)
    {
        status = VDP_BAD_CP_TYPE;
        category = AVDT_ASC_PROTECT;
    }

    if (status == 0)
    {
        //TODO more for video??
        bta_av_co_video_cb.sep_info_idx = 0;
        num = p_snk->num_good;
        p_snk->idx = 0;

        /* find the seid list */
        count = 0;
        for (j=0; j<num; j++)
        {
            if (seid != p_snk->sep[j].seid)
            {
                APPL_TRACE_DEBUG2("seid[%d]: %d", count, p_snk->sep[j].seid);
                seids[count++] = p_snk->sep[j].seid;
            }
        }
    }

    /* call call-in */
    bta_av_ci_setconfig(hndl, status, category, count, seids, FALSE);
#endif
}

/*******************************************************************************
**
** Function         bta_av_co_video_open
**
** Description      This function is called by AV when the video stream connection
**                  is opened.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_open(tBTA_AV_HNDL hndl,
                          tBTA_AV_CODEC codec_type, UINT8 *p_codec_info, UINT16 mtu)
{
#if 0
    tBTUI_AV_VIDEO_CFG cfg;

    APPL_TRACE_DEBUG1("bta_av_co_video_open mtu:%d", mtu);

    bta_av_co_video_cb.timestamp = 0;
    bta_av_co_video_cb.rpt_count = 0;
    bta_av_co_video_cb.pkt_count = 0;
    bta_av_co_video_cb.octet_count = 0;

    cfg.p_cback = bta_av_video_codec_callback;
    cfg.sep_info_idx = bta_av_co_video_cb.sep_info_idx;
    memcpy(cfg.codec_info, p_codec_info, AVDT_CODEC_SIZE);
#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    btui_video_open(&cfg);
#endif
#endif
}

/*******************************************************************************
**
** Function         bta_av_co_video_close
**
** Description      This function is called by AV when the video stream connection
**                  is closed.
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_close(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type, UINT16 mtu)
{
#if 0
    APPL_TRACE_DEBUG0("bta_av_co_video_close");
    bta_av_co_video_cb.report = FALSE;
#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    btui_video_close();
#endif
#endif
}

/*******************************************************************************
**
** Function         bta_av_co_video_delay
**
** Description      This function is called by AV when the audio stream connection
**                  needs to send the initial delay report to the connected SRC.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_delay(tBTA_AV_HNDL hndl, UINT16 delay)
{
    APPL_TRACE_ERROR2("bta_av_co_video_delay handle: x%x, delay:0x%x", hndl, delay);
}

/*******************************************************************************
**
** Function         bta_av_co_video_start
**
** Description      This function is called by AV when the video streaming data
**                  transfer is started.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_start(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type)
{
#if 0

    APPL_TRACE_DEBUG0("bta_av_co_video_start");
#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    btui_video_start();
    _ftime (&bta_av_co_video_cb.lsr);
#endif
#endif
}

/*******************************************************************************
**
** Function         bta_av_co_video_stop
**
** Description      This function is called by AV when the video streaming data
**                  transfer is stopped.
**
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_stop(tBTA_AV_HNDL hndl, tBTA_AV_CODEC codec_type)
{
#if 0
    APPL_TRACE_DEBUG0("bta_av_co_video_stop");
#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    btui_video_stop();
#endif
#endif
}

/*******************************************************************************
**
** Function         bta_av_co_video_src_data_path
**
** Description      This function is called to get the next data buffer from
**                  the video codec
**
** Returns          NULL if data is not ready.
**                  Otherwise, a video data buffer (UINT8*).
**
*******************************************************************************/
void * bta_av_co_video_src_data_path(tBTA_AV_CODEC codec_type,
                                     UINT32 *p_len, UINT32 *p_timestamp)
{
#if 0
    UINT8   *p_buf = NULL;
    UINT32  delta;

#if (BTA_AV_CO_INCLUDE_BTUI == TRUE)
    if ((*p_len = btui_video_readbuf(&p_buf, &delta)) != 0)
    {
        *p_timestamp = bta_av_co_video_cb.timestamp + delta;
        bta_av_co_video_cb.timestamp = *p_timestamp;

        bta_av_co_video_send_sr(*p_timestamp);
        bta_av_co_video_cb.pkt_count++;
        bta_av_co_video_cb.octet_count += *p_len;
    }
#endif
    return p_buf;
#endif
    return NULL;
}


/*******************************************************************************
**
** Function         bta_av_co_video_report_rr
**
** Description      This function is called by AV when a Receiver Report is
**                  received
**
** Returns          void
**
*******************************************************************************/
void bta_av_co_video_report_rr (UINT32 packet_lost)
{
    /* platform dependent:
     * tell the codec to send a SYNC frame if packet_lost is bigger than last RR */
    APPL_TRACE_DEBUG1("bta_av_co_video_report_rr: %d", packet_lost);
}

#endif  // BTA_AV_INCLUDED
