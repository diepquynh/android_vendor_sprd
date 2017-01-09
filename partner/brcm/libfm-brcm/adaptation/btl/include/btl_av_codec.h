/*****************************************************************************
**
**  Name:           btl_av_codec.h
**
**  Description:    Interface file to the example codec implementation used
**                  by BTA advanced audio.  The implementation uses the
**                  wave file to get PCM samples as input to the
**                  SBC encoder.
**
**  Copyright (c) 2004-2009, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BTUI_AV_CODEC_H
#define BTUI_AV_CODEC_H

#include "bt_target.h"
#include "a2d_api.h"
#include "a2d_sbc.h"
#include "sbc_encoder.h"
#include "a2d_m12.h"
#include "bta_av_api.h"
#include "bta_av_sbc.h"
#include "bta_av_m12.h"

/*****************************************************************************
**  Constants
*****************************************************************************/

/* callback event */
#define BTUI_CODEC_RX_READY         1

#define MAX_PATH_SIZE 32

#define BTA_AV_TIMER_EVT EVENT_MASK(APPL_EVT_3)
#define BTA_AV_CONNECT_EVT EVENT_MASK(APPL_EVT_4)
#define BTA_AV_SRC_UNDERRUN_EVT EVENT_MASK(APPL_EVT_5)

/* Enable when you want to send MP3 file directly */
#define BTUI_AV_M12_MP3_SUPORT    TRUE
//#undef BTUI_AV_M12_MP3_SUPORT

/* maximum number of packets for btui codec to queue before dropping data
** this is approximately 230 ms of audio at 44.1 kHz sample rate
*/
#define BTUI_AV_PKT_Q_MAX        10

#define BTUI_AV_SBC_BIT_RATE (229)
#define BTUI_AV_CHANNEL_MODE 0x01

/* max number of SBC frames per packet */
#define BTUI_CODEC_FR_MAX           15

/* tuneable number of SBC frames per waveIn data buffer		*/
/* NOTE: If you change this value then you need to reset	*/
/*       the asrc_wav_period_tbl with correct info.			*/
/*       BTUI_CODEC_FR_PER_BUF = 7 means we will read 896	*/
/*       samples per buffer.								*/

#define USE_40MS_WAV_TIMER
#ifdef USE_40MS_WAV_TIMER
/* Default time tick for AA - 40 miliseconds */
#define BTUI_AV_TIME_TICK                                      40000
#define BTUI_CODEC_FR_PER_BUF_48_kHz		       20
#define BTUI_CODEC_FR_PER_BUF_44_1kHz		20
#define BTUI_CODEC_FR_PER_BUF_44_1kHz_13	       13
#define BTUI_CODEC_FR_PER_BUF_32kHz			10
#define BTUI_CODEC_FR_PER_BUF_16kHz			5
#else	//80ms interval
#define BTUI_AV_TIME_TICK                                      80000
#define BTUI_CODEC_FR_PER_BUF_48_kHz		       30
#define BTUI_CODEC_FR_PER_BUF_44_1kHz		28
#define BTUI_CODEC_FR_PER_BUF_44_1kHz_13	       26
#define BTUI_CODEC_FR_PER_BUF_32kHz			20
#define BTUI_CODEC_FR_PER_BUF_16kHz			10
#endif

// Added For Mono support
#define BT_AV_44P1_LINE_SPEED_KBPS	       229
#define BT_AV_48_LINE_SPEED_KBPS		       237
#define BT_AV_44P1_MONO_LINE_SPEED_KBPS	127
#define BT_AV_48_MONO_LINE_SPEED_KBPS	132

/* Input Data is PCM or SBC */
enum
{
    BTUI_INPUT_DATA_IS_PCM  = 0x01,
    BTUI_INPUT_DATA_IS_SBC   = 0x02
};

//#if defined( USE_ALREADY_SBC_ENCODED_DATA ) && ( USE_ALREADY_SBC_ENCODED_DATA == TRUE )

// SBC FRAME length  Stereo
#define SBC_FRAME_LENGTH_48_AT_237                        79
#define SBC_FRAME_LENGTH_441_AT_229                      83

// SBC FRAME length Mono
#define SBC_FRAME_LENGTH_48_MONO_AT_132             44
#define SBC_FRAME_LENGTH_441_MONO_AT_127           46

//#define BTUI_CODEC_BUF_MAX_48kHz         (BTUI_CODEC_FR_PER_BUF_48_kHz        *  SBC_FRAME_LENGTH_48_AT_237)
//#define BTUI_CODEC_BUF_MAX_44_1kHz     (BTUI_CODEC_FR_PER_BUF_44_1kHz      *  SBC_FRAME_LENGTH_441_AT_229)
//#define BTUI_CODEC_BUF_MAX_44_1kHz13 (BTUI_CODEC_FR_PER_BUF_44_1kHz_13 *  SBC_FRAME_LENGTH_441_AT_229)

// CTT to be calculated later - Not Accurate.
//#define BTUI_CODEC_BUF_MAX_32kHz           (BTUI_CODEC_FR_PER_BUF_32kHz      * SBC_FRAME_LENGTH_441_AT_229)
//#define BTUI_CODEC_BUF_MAX_16kHz           (BTUI_CODEC_FR_PER_BUF_16kHz      * SBC_FRAME_LENGTH_441_AT_229)


// SBC Frame Length for Stereo
#define BTUI_CODEC_BUF_MAX_48kHz_SBC           (BTUI_CODEC_FR_PER_BUF_48_kHz       * SBC_FRAME_LENGTH_48_AT_237)
#define BTUI_CODEC_BUF_MAX_44_1kHz_SBC       (BTUI_CODEC_FR_PER_BUF_44_1kHz      * SBC_FRAME_LENGTH_441_AT_229)
#define BTUI_CODEC_BUF_MAX_44_1kHz13_SBC   (BTUI_CODEC_FR_PER_BUF_44_1kHz_13 * SBC_FRAME_LENGTH_441_AT_229)

// SBC Frame Length for Mono
#define BTUI_CODEC_BUF_MAX_48kHz_SBC_MONO           (BTUI_CODEC_FR_PER_BUF_48_kHz          * SBC_FRAME_LENGTH_48_MONO_AT_132)
#define BTUI_CODEC_BUF_MAX_44_1kHz_SBC_MONO       (BTUI_CODEC_FR_PER_BUF_44_1kHz        * SBC_FRAME_LENGTH_441_MONO_AT_127)
#define BTUI_CODEC_BUF_MAX_44_1kHz13_SBC_MONO   (BTUI_CODEC_FR_PER_BUF_44_1kHz_13  * SBC_FRAME_LENGTH_441_MONO_AT_127)

//#else
/* Number of samples per waveIn pcm data buffer period, calculated as follows:
**
** Max SBC frames per AVDTP packet is 15.
** Samples per SBC frame for SBC is num_blocks * num_subbands.
** Typically music uses num_blocks=16, num_subbands=8 --> 128 samples/frame.
*/
#define BTUI_CODEC_SAMPLES_PER_BUF_48kHz           (BTUI_CODEC_FR_PER_BUF_48_kHz          * 128)
#define BTUI_CODEC_SAMPLES_PER_BUF_44_1kHz       (BTUI_CODEC_FR_PER_BUF_44_1kHz        * 128)
#define BTUI_CODEC_SAMPLES_PER_BUF_44_1kHz13   (BTUI_CODEC_FR_PER_BUF_44_1kHz_13   * 128)
#define BTUI_CODEC_SAMPLES_PER_BUF_32kHz           (BTUI_CODEC_FR_PER_BUF_32kHz            * 128)
#define BTUI_CODEC_SAMPLES_PER_BUF_16kHz           (BTUI_CODEC_FR_PER_BUF_16kHz            * 128)

/* maximum number of bytes in each waveIn pcm data buffer
** samples per frame * num channels * bytes per sample
*/
#define BTUI_CODEC_BUF_MAX_48kHz             (BTUI_CODEC_SAMPLES_PER_BUF_48kHz           * 2 * 2)
#define BTUI_CODEC_BUF_MAX_44_1kHz         (BTUI_CODEC_SAMPLES_PER_BUF_44_1kHz       *  2 * 2)
#define BTUI_CODEC_BUF_MAX_44_1kHz13     (BTUI_CODEC_SAMPLES_PER_BUF_44_1kHz13    * 2 * 2)
#define BTUI_CODEC_BUF_MAX_32kHz             (BTUI_CODEC_SAMPLES_PER_BUF_32kHz           * 2 * 2)
#define BTUI_CODEC_BUF_MAX_16kHz             (BTUI_CODEC_SAMPLES_PER_BUF_16kHz           * 2 * 2)

// Added for Mono support.  Only 44.1 and 48kHz
// PCM frame sizes for Mono
#define BTUI_CODEC_BUF_MAX_48kHz_Mono              (BTUI_CODEC_BUF_MAX_48kHz         / 2)
#define BTUI_CODEC_BUF_MAX_44_1kHz_Mono          (BTUI_CODEC_BUF_MAX_44_1kHz      / 2)
#define BTUI_CODEC_BUF_MAX_44_1kHz13_Mono      (BTUI_CODEC_BUF_MAX_44_1kHz13  / 2)
#define BTUI_CODEC_BUF_MAX_32kHz_Mono              (BTUI_CODEC_BUF_MAX_32kHz         / 2)
#define BTUI_CODEC_BUF_MAX_16kHz_Mono              (BTUI_CODEC_BUF_MAX_16kHz         / 2)
//#endif

/* number of waveIn pcm data buffers */
#define BTUI_CODEC_NUM_BUF          6

/* number of bits per pcm sample */
#define BTUI_CODEC_BITS_PER_SAMPLE  16

/* maximum out queue buffer limit */
#define BTUI_CODEC_OUT_Q_MAX        24

/* Upsample everything to 44.1 and 48kHz */
#define SUPPORT_ONLY_48_AND_44_KHZ    TRUE

/*****************************************************************************
** MPEG1,2 related define macros
******************************************************************************/
#define BTUI_AV_M12_OFFSET (AVDT_MEDIA_OFFSET + 4)

/* pm3 frame header */
#define BTUI_AV_M12_FRM_HDR_SIZE (4)

/* StreamingConfig */
#define BTUI_AV_STR_CFG_NONE      (0)
#define BTUI_AV_STR_CFG_WAV_2_SBC (1)
#define BTUI_AV_STR_CFG_MP3_2_SBC (2)
#define BTUI_AV_STR_CFG_MP3_2_MP3 (3)


/* Global AA state machine */
#define BTUI_AV_STATE_CLOSED            (0x00)
#define BTUI_AV_STATE_OPEN              (0x01)
#define BTUI_AV_STATE_OPENING           (0x02)
#define BTUI_AV_STATE_STARTING          (0x03)
#define BTUI_AV_STATE_STARTED           (0x04)
#define BTUI_AV_STATE_STOPPED           (0x05)
#define BTUI_AV_STATE_SUSPENDED         (0x06)
#define BTUI_AV_STATE_CHANGING_TRACK    (0x07)
#define BTUI_AV_STATE_DISCONNECTING     (0x08)
#define BTUI_AV_STATE_CONNECTED_AS_ACP  (0x09)


/* Maximum MP3 data size to read */
#define BTUI_AV_MP3_DATA_MAX (1500)

/*****************************************************************************
**  Data types
*****************************************************************************/

/* application callback */
typedef void (tBTUI_CODEC_CBACK)(UINT8 event);

/* configuration structure passed to btui_codec_open */
typedef struct
{
    tA2D_SBC_CIE        sbc_cie;        /* SBC encoder configuration */
    tA2D_SBC_CIE        sbc_cfg;        /* SBC stream configuration */
    tA2D_SBC_CIE        sbc_current;        /* SBC current configuration */
    tBTUI_CODEC_CBACK   *p_cback;       /* application callback */

// #if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
    UINT16              Channels;       /* mono or stereo */
    UINT32              dwSamplesPerSec;
    UINT32              dwInputDatatype;
// #endif

    UINT16              bit_rate;       /* SBC encoder bit rate in kbps */
    UINT16              bit_rate_busy;  /* SBC encoder bit rate in kbps */
    UINT16              bit_rate_swampd;/* SBC encoder bit rate in kbps */
    UINT8               pool_id;        /* GKI pool id for output packet buffers */
    UINT16              offset;         /* GKI buffer offset */
    UINT8               pkt_q_max;      /* output packet queue max */
    UINT16              mtu;            /* output packet mtu in bytes */
    UINT8               num_snk;        /* future use */
    UINT8               num_seps;       /* number of seps returned by stream discovery */
    UINT8               sep_info_idx;   /* sep_info_idx as reported/used in the bta_aa_co_getconfig */
} tBTUI_CODEC_CFG;

typedef struct fmt_header {
    unsigned int fmt_id;        /* 'fmt' */
    unsigned int fmt_chunk_size;
    unsigned short wFormatTag;
    unsigned short wChannels;
    unsigned int dwSamplesPerSec;
    unsigned int dwAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short wBitsPerSample;

// #if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
    UINT32       dwInputDatatype;
// #endif

} FormatChunk;

typedef struct data_header {
    unsigned int data_id;       /* 'data' */
    unsigned int data_chunk_size;       /* size of wave data left in file */
} DataChunk;

typedef struct wav_hdr {
    unsigned int riff_id;       /* 'RIFF'  */
    unsigned int file_len;      /* file len - 8 bytes */
    unsigned int wav_id;        /* 'WAVE'  */
    FormatChunk fmt;
    DataChunk data;
} WaveChunk;


typedef struct file_cb {
    int  fd;
    UINT32 tick_period;         /* in us */
    char filename[MAX_PATH_SIZE];
    FormatChunk fmt;
    UINT8 ResamplingNeeded;
    UINT32 DstSps;
    UINT16 samp_freq;
    UINT32 ResamplingNeededFrPerBuf;
    UINT32 ResamplingNeededBufMax;
} tFILEHDR;

typedef struct {
    UINT32 index;
    UINT32 dwBufferLength;
    UINT32 dwFlags;
    UINT8 free;
    UINT32 p_next;
    unsigned char *p_data;
} tWAVHDR;

typedef struct {
    tWAVHDR hdr;                                                           /* wave header */

#if defined( USE_ALREADY_SBC_ENCODED_DATA ) && ( USE_ALREADY_SBC_ENCODED_DATA == TRUE )
    unsigned char data[BTUI_CODEC_BUF_MAX_48kHz * 2 ];   /* data             */
#else
    unsigned char data[BTUI_CODEC_BUF_MAX_48kHz];        /* data             */
#endif
} tBTUI_CODEC_BUFFER;

typedef struct {
    UINT8 lmp_ver;
    UINT16 manufacturer;
    UINT8 lmp_subver;
}tBTUI_RMT_VERSION;

typedef struct {
    tBTUI_CODEC_BUFFER buf[BTUI_CODEC_NUM_BUF]; /* waveIn buffers */
    UINT32 buf_len;                              /* waveIn buffer length */
    UINT32 buf_idx;                              /* waveIn buffer index */

    tBTA_AV_CHNL    chnl;
    tBTA_AV_HNDL    hndl;

    char thread_done;                            /* thread done */

    /* for API and SBC encoder */
    UINT8 state;                                 /* State of AA */
    char started;                                /* true if codec started */
    tBTUI_CODEC_CBACK *p_cback;                  /* API callback */
    UINT16 fr_per_pkt;                           /* SBC frames per packet */
    UINT8 pool_id;                               /* GKI buffer pool for output packets */
    UINT16 offset;                               /* BT_HDR offset for output packets */
    UINT16 bytes_per_fr;                         /* pcm bytes per SBC frame encode */
    SBC_ENC_PARAMS encoder;                      /* SBC encoder control block */
    BUFFER_Q out_q;                              /* output packet GKI queue */
    UINT16 out_q_max;                            /* max number of buffers in out_q */
    tBTUI_CODEC_CFG codec_cfg;

    UINT8 num_snk;                               /* Total number of sink */
    UINT8 num_seps;                              /* Total number of stream end points (SEP) */
    UINT8 sep_info_idx;                          /* sep_info_idx as reported/used in the bta_aa_co_getconfig */
    UINT8 sbc_sep_idx;                           /* SEP index for SBC */
    UINT8 m12_sep_idx;                           /* SEP index for MPEG-1,2 Audio */
    UINT8 m24_sep_idx;                           /* SEP index for MPEG-2,4 Audio */
    UINT8 atrac_sep_idx;                         /* SEP index for ATRAC family */
    UINT8 pref_codec;                            /* Pregered Codec */
    UINT8 str_cfg;                               /* Kind of the streaming what you want */
	UINT8 cur_codec_info[AVDT_CODEC_SIZE];
    UINT8 peer_sbc_codec_info[AVDT_CODEC_SIZE];  /* SBC info of peer device */
    UINT8 peer_m12_codec_info[AVDT_CODEC_SIZE];  /* MPEG-1,2 Audio of peer device */
    UINT8 peer_m24_codec_info[AVDT_CODEC_SIZE];  /* MPEG-2,4 AAC of peer device */
    UINT8 peer_atrac_codec_info[AVDT_CODEC_SIZE];/* ATRAC family of peer device */

    tFILEHDR file_cb;
    int   serv_fd;

    tA2D_M12_CIE cie;                            /* MPEG-1, 2 Audio Codec Information Element */
    UINT32 mp3_file_size;                        /* MP3 file size. Used for MP3 streaming only */
    UINT32 timer_counter;                        /* Timer counter to send MP3 data */
    UINT32 timestamp;                            /* Time stamp */
    UINT8 avdt_hdl;                              /* AVDT Handle */
    UINT8 timer_started;                          /* TRUE, if timer is started, otherwise FALSE */

    tBTUI_RMT_VERSION rmt_version;
    BOOLEAN pcm_dump;
	 tBTUI_CODEC_CFG open_cfg;
} tBTUI_AV_CB;

typedef struct tBTUI_AV_M12_FR_HDR_TAG
{
    tA2D_M12_CIE    cie;
    UINT8           id;
    UINT8           bitrate_index;
    UINT8           samp_freq_index;
    UINT8           padding_bit;
} tBTUI_AV_M12_FR_HDR;


#ifdef KBNAM
/****************************************************************************
** for receive dat form MP3 decodef : KBNAM
*****************************************************************************/

// Initial value of cWriteIndex = 0;
// Initial value of cReadIndex = 0;

/* Data Writing
	cTmpIndex = cWriteIndex + 1;
	if( cTmpIndex == MAX_PCM_DATA_BUFFER_COUNT )
		cTmpIndex = 0;

	if( cTmpIndex == cReadIndex )
	{
		buffer full
	}
	else
	{
		write data on mp3_buffer[cTmpIndex] : usFrameLength

		// Schedule lock
		cWriteIndex = cTmpIndex;
		// Schedule unlock
	}
*/

/* Data Reading

	if( cReadIndex == cWriteIndex )
	{
		buffer empty
	}
	else
	{
		read data from mp3_buffer[cReadIndex] : usFrameLength

		// Schedule lock
		cReadIndex++;
		// Schedule unlock
	}
*/

typedef struct {
	char mp3_buffer[MAX_PCM_DATA_BUFFER_COUNT][BTUI_CODEC_SAMPLES_PER_BUF_48kHz];
	char cWriteIndex;
	char cReadIndex;
	unsigned short usFrameLength;
} tBTA_MP3_Buffer;

extern tBTA_MP3_Buffer gBta_mp3_buffer;

// 1. BT stack set usFrameLength;
// 2. call for start MP3 decoding : event or function call

#endif // KBNAM

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
/***************************************************************************
 *  Function        btapp_av_enable
 *
 *  Descripition    Enable/disable av profile
 *
 *  Params          av_en: TRUE: enable AV, FALSE: disable
 *  - Description:
 *
 ***************************************************************************/
extern void btapp_av_enable( UINT8 av_en );

/*******************************************************************************
**
** Function         btui_wav_timer_cback
**
** Description      WAV timer callback function
**
** Returns          void
**
*******************************************************************************/
extern void btui_wav_timer_cback(signed long id);

/*******************************************************************************
**
** Function         btui_wav_start_timer
**
** Description      Start WAV timer
**
** Returns          void
**
*******************************************************************************/
extern UINT32 btui_wav_start_timer(UINT32 tick_period);

/*******************************************************************************
**
** Function         btui_wav_start_timer
**
** Description      Stop WAV timer
**
** Returns          void
**
*******************************************************************************/
extern void btui_wav_stop_timer(void);

/*******************************************************************************
**
** Function         btui_wav_start_timer
**
** Description      Close WAV file
**
** Returns          void
**
*******************************************************************************/
extern UINT32 btui_wav_close(tFILEHDR * f);

/*******************************************************************************
**
** Function         btui_wav_read_next
**
** Description      Read next WAV file in the juke_box_wav
**
** Returns          void
**
*******************************************************************************/
extern void btui_wav_read_next(void);

/*******************************************************************************
**
** Function         btui_wav_read_prev
**
** Description      Read previous WAV file in the juke_box_wav
**
** Returns          void
**
*******************************************************************************/
extern void btui_wav_read_prev(void);

/*******************************************************************************
**
** Function         btui_wav_get_clk_tick
**
** Description      get the clock tick
**
**
** Returns          void
*******************************************************************************/
extern UINT32 btui_wav_get_clk_tick(void);

/*******************************************************************************
**
** Function         btui_wav_set_clk_tick
**
** Description      Set the clock tick
**
**
** Returns          void
*******************************************************************************/
extern void btui_wav_set_clk_tick(UINT32 tick);

/*******************************************************************************
**
** Function         btui_wav_track_info
**
** Description      Give channel/SamplesPerSec information to SBC encoder
**                  It should be call before start streaming with some PCM data
**
** Returns          void
*******************************************************************************/
extern void btui_wav_track_info(UINT32 channels, UINT32 SamplesPerSec);
//extern void btui_wav_track_info(UINT32 channels, UINT32 SamplesPerSec, UINT32 InputDatatype);


/*******************************************************************************
**
** Function         btui_wav_buffer_read
**
** Description      Read pcm data from file
**
** Returns          void
**
*******************************************************************************/
extern UINT32 btui_wav_buffer_read(tFILEHDR *f, UINT8 *buf, UINT32 datasize);

/*******************************************************************************
**
** Function         btui_codec_init
**
** Description      Initialize btui codec service.  This function just prints
**                  out debug messages displaying the available audio devices
**                  available on the PC.
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_init(void);

/*******************************************************************************
**
** Function         btui_codec_open
**
** Description      Open the btui codec service.  Initialize control block
**                  variables and SBC encoder.  Open the waveIn interface and
**                  start the Windows thread that handles data.
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_open(tBTUI_CODEC_CFG * p_cfg);

/*******************************************************************************
**
** Function         btui_codec_notify
**
** Description
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_notify(void);

/*******************************************************************************
**
** Function         btui_codec_start
**
** Description      Start the btui codec service.  This initializes the waveIn
**                  pcm data buffers and starts reading pcm samples from the
**                  audio device.
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_start(tBTA_AV_CODEC codec_type);

/*******************************************************************************
**
** Function         btui_codec_stop
**
** Description      Stop the btui codec service.  This resets the waveIn
**                  pcm data buffers and stops reading pcm samples from the
**                  audio device.
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_stop(tBTA_AV_CODEC codec_type);

/*******************************************************************************
**
** Function         btui_codec_close
**
** Description      Close the btui codec service.  Stop the codec, then close
**                  the waveIn interface, then stop the Windows thread.
**
** Returns          void
**
*******************************************************************************/
extern void btui_codec_close(tBTA_AV_CODEC codec_type);

/*******************************************************************************
**
** Function         btui_codec_readbuf
**
** Description      Read the next packet buffer.  The buffer contains one or
**                  more SBC frames.  The number of frames is in the
**                  layer_specific element of the BT_HDR.
**
** Returns          Pointer to buffer or NULL if no buffer available.
**
*******************************************************************************/
extern BT_HDR *btui_codec_readbuf(tBTA_AV_CODEC codec_type);

/*******************************************************************************
**
** Function         btui_codec_get_cur_idx
**
** Description      get the current SEP info index
**
**
** Returns          void
*******************************************************************************/
extern UINT8 btui_codec_get_cur_idx(void);

/*******************************************************************************
**
** Function         BTUI_AV_sbc_codec_init
**
** Description      inittialise the current codec
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_sbc_codec_init(UINT8 *p_codec_info, UINT16 mtu);

/*******************************************************************************
**
** Function         sbc_encode_task
**
** Description      Task for SBC encoder.  This task receives an
**                  event when the waveIn interface has a pcm data buffer
**                  ready.  On receiving the event, handle all ready pcm
**                  data buffers.  If stream is started, run the SBC encoder
**                  on each chunk of pcm samples and build an output packet
**                  consisting of one or more encoded SBC frames.
**
** Returns          void
**
*******************************************************************************/
extern void sbc_encode_task(void *p);

extern void av_src_read_task(void *p);

void btl_av_sco_status(BOOLEAN sco_up);


#if USE_BCMBTUI  == TRUE
extern UINT32 BTA_MP3_Play( char *filename );
#else
extern UINT32 BTA_MP3_Play( void );
#endif

#endif /* BTUI_AV_CODEC_H */
