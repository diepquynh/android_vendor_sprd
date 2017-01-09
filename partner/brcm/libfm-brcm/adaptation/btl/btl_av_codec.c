/*****************************************************************************
**
**  Name:           btui_av_codec.c
**
**  Description:    Interface file to the example codec implementation used
**                  by BTA advanced audio.  The implementation uses the
**                  wave file to get PCM samples as input to the
**                  SBC encoder.
**
**  Copyright (c) 2004, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include "bt_target.h"

#define LOG_TAG "BTL_AV_CODEC:"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

BOOLEAN started_sent = FALSE;
#define AV_PRELOAD_PCM_BUF_COUNT 2
int num_pcm_bufs_to_read = 0;

#if defined(BTA_AV_INCLUDED) && (BTA_AV_INCLUDED == TRUE)
#include "gki.h"
#include "gki_int.h"
#include "a2d_api.h"
#include "avdt_api.h"
#include "sbc_encoder.h"
#include "btl_av_codec.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/poll.h>

#ifndef BTL_AV_DBG
#define BTL_AV_DBG FALSE
#endif

BOOLEAN  AV_WAS_SUSPENDED_BY_AG = FALSE;
BOOLEAN  av_was_suspended_by_hs = FALSE;
#define DRAIN_UNTIL_SCO_DISC 0x1
#define DRAIN_UNTIL_UNCONGESTED 0x2
int drain_until = 0;
//BOOLEAN av_src_read_waiting_for_data = FALSE;

#ifdef STORE_WAV_DATA
FILE *recv_fp = NULL;
#endif


#define USE_BCMBTUI TRUE

//The number of SBC encoded buffers we want to keep filled from the source just in case it goes away for some time
#define BTL_AV_SRC_UNDERRUN_PROTECT_CNT 0

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

extern tA2D_SBC_CIE bta_av_co_open_cie;
extern UINT32 btui_wav_buffer_read(tFILEHDR *f, UINT8 *buf, UINT32 datasize);
extern UINT8 btui_codec_get_cur_idx(void);
extern void bta_av_audio_codec_callback(UINT8 event);
extern void btui_av_reconfig(void);

static UINT8 mp3_buffer [BTUI_CODEC_BUF_MAX_48kHz  * 2];


#define BTUI_MAX_EXP_WAIT         7   // 7 * 40ms , enough time MMP to produce SBC data (16 k buffer)

/*****************************************************************************
**  Constants
*****************************************************************************/
#define	MIN_AV_SAMPLE_RATE 4000
#define	MAX_AV_SAMPLE_RATE 48000
#define AUDIO_HDR_NOT_READY 0
#define AUDIO_HDR_READY 1
#define AUDIO_HDR_ENCODED 2

#define WAV_RIFF		0x46464952
#define WAV_WAVE		0x45564157
#define WAV_FMT			0x20746D66
#define WAV_DATA		0x61746164
#define WAV_DATA_1		0x74636166      /* Hack hack hack! */
#define WAV_PCM_CODE	1

#define FAKE_READ_SIZE 4608

// void btui_wav_track_info(UINT32 channels, UINT32 SamplesPerSec, UINT32 InputDatatype);
void btui_wav_track_info(UINT32 channels, UINT32 SamplesPerSec);

#ifdef USE_40MS_WAV_TIMER
/* frame period, in milisecond,  */
static const UINT32 asrc_wav_period_tbl[] = {
    40000,                      /* 44.1 or 22.05 kHz */
    40000,                      /* 48 or 24 kHz - 15 */
    40000,                      /* 32 or 16 kHz - 10 */
    40000                       /* 8 khz */
};
#else		//80ms
/* frame period, in milisecond,  */
static const UINT32 asrc_wav_period_tbl[] = {
    80000,                      /* 44.1 or 22.05 kHz */
    80000,                      /* 48 or 24 kHz - 30 */
    80000,                      /* 32 or 16 kHz - 20 */
    80000                       /* 8 khz */
};
#endif

/* sample frequency, millisec units, used for frame size calculation */
const UINT32 asrc_m12_freq_tbl[] = {
    44100,
    48000,
    32000,
    16000
};

/* lookup table for converting channel mode */
INT16 btui_codec_mode_tbl[] = { SBC_JOINT_STEREO, SBC_STEREO, SBC_DUAL, 0, SBC_MONO};

/* lookup table for converting number of blocks */
INT16 btui_codec_block_tbl[] = { 16, 12, 8, 0, 4};

static       int    magic_number = 5;
static const UINT32 magic_number_tbl[] = {
    5,        /* 44100 */
    5,        /* 48000 */
    5,        /* 32000 */
    5,        /* 16000 */
};

static UINT16 outq_max = 0;
static UINT32 save_length = 0;

/*****************************************************************************
**  Data types
*****************************************************************************/

/*****************************************************************************
**  Local data
*****************************************************************************/
int BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_44_1kHz;
int BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz;

tWAVHDR *p_wav_hdr;
WaveChunk wav_hdr;

struct wav_time_intv {
    UINT32 save;
} wav_time_intvs[7];

// static rex_timer_type wav_timer;
static UINT32 wav_timer_period;

static UINT16 SbcBitRates[] = { 16000, 32000, 44100, 48000};
static UINT16 AVBitRates[] = { A2D_SBC_IE_SAMP_FREQ_16, A2D_SBC_IE_SAMP_FREQ_32, \
    A2D_SBC_IE_SAMP_FREQ_44, A2D_SBC_IE_SAMP_FREQ_48};
extern  tBTUI_AV_CB btui_av_cb;
extern UINT8 default_bta_av_co_current_codec_info[AVDT_CODEC_SIZE];
extern UINT8 default_bta_av_co_peer_sbc_info[AVDT_CODEC_SIZE];

/*****************************************************************************
**  Local functions
*****************************************************************************/
static void btui_codec_cback(void);
static tWAVHDR *btui_codec_next_buffer(void);
static void btui_codec_setup_sbc_encoder(SBC_ENC_PARAMS * p_enc, tA2D_SBC_CIE * p_cfg, FormatChunk *fmt);
static INT16 btui_codec_fr_len(SBC_ENC_PARAMS * p_enc);
static UINT16 btui_codec_fr_per_pkt(SBC_ENC_PARAMS * p_enc, UINT16 mtu);

static pthread_t            wav_timer_thread_id;

static BOOLEAN wav_timer_thread_created = FALSE;


static int btui_codec_num_bufs(void)
{
    int i;
	int num = 0;


    for (i = 0; i < BTUI_CODEC_NUM_BUF; i++) {
        if ((btui_av_cb.buf[i].hdr.dwFlags != AUDIO_HDR_NOT_READY) ) {
            num++;
        }
    }

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("btui_codec_num_bufs num = %d", num);
#endif

    return num;
}


#if (BTL_AV_DBG==TRUE)
void log_tstamps_us(char *comment, BOOLEAN dumponly)
{
#define USEC_PER_SEC 1000000L
    static struct timespec prev = {0, 0};
    struct timespec now, diff;
    unsigned int diff_us, now_us;

    if (!dumponly)
    {
        clock_gettime(CLOCK_MONOTONIC, &now);
        now_us = now.tv_sec*USEC_PER_SEC + now.tv_nsec/1000;
        diff_us = (now.tv_sec - prev.tv_sec) * USEC_PER_SEC + (now.tv_nsec - prev.tv_nsec)/1000;
    }

    APPL_TRACE_EVENT6("[%s] ts %08d, diff : %08d, l2c_q:%02d, a2dp:%02d, pcm:%02d",
                    comment, now_us, diff_us, bta_av_get_queue_sz(0),
                    btui_av_cb.out_q.count, btui_codec_num_bufs());

    if (!dumponly)
        prev = now;
}
#endif

void btl_av_sco_status(BOOLEAN sco_up)
{
    APPL_TRACE_EVENT1("######### SCO LINK IS %s #########", sco_up?"UP":"DOWN");
    AV_WAS_SUSPENDED_BY_AG = sco_up;
}


/*******************************************************************************
**
** Function         btui_wav_timer_cback
**
** Description      WAV timer callback function
**
** Returns          void
**
*******************************************************************************/

static void interruptFn (int sig)
{
	pthread_exit(0);
}


void btui_wav_timer_cback(signed long id)
{
    struct timespec prev = {0, 0}, now;
    struct timespec sInterval = {0, 0};
    int delta;
    const int POLL_INTERVAL_NSEC = 58049846;
    //      const int POLL_INTERVAL_NSEC = 58049886;
    const int NSEC_PER_SEC = 1000000000;

    struct sockaddr_un remote;
    int fd, t, count = 0;

    int sig = SIGINT;
    sigset_t sigSet;
    sigemptyset (&sigSet);
    sigaddset (&sigSet, sig);

    pthread_sigmask (SIG_UNBLOCK, &sigSet, NULL);

    struct sigaction act;
    act.sa_handler = interruptFn;
    sigaction (sig, &act, NULL );

    while(  !btui_av_cb.thread_done )
    {
        if(   (!started_sent) && (drain_until == 0) )
        {
            APPL_TRACE_DEBUG0("SBC_ENCODE_TASK: waiting for incoming socket connection...");

            if ((fd = accept(btui_av_cb.serv_fd, (struct sockaddr *)&remote, &t)) == -1) {
                perror("accept");
                return;
            }

            APPL_TRACE_DEBUG0("SBC_ENCODE_TASK: incoming socket connection !");

            //Check if the thread was done while waiting for accept
            if( (btui_av_cb.state == BTUI_AV_STATE_DISCONNECTING) || (btui_av_cb.thread_done) )
            {
                //Close the socket and call it a day
                LOGE("Detected state BTUI_AV_STATE_DISCONNECTING, close codec");
                //btui_drain_socket(fd);
                close(fd);
                continue;
            }

            if (AV_WAS_SUSPENDED_BY_AG == FALSE)
            {
                APPL_TRACE_EVENT0("SCO not active, start stream");
                btui_av_cb.file_cb.fd = fd;
                started_sent = TRUE;
                btapp_av_start_play();
                if (av_was_suspended_by_hs == TRUE)
                {
                    av_was_suspended_by_hs = FALSE;
                }

#ifdef STORE_PCM_DATA
                if ((recv_fp = fopen("/sdcard/Music/pcm_file", "wb")) == NULL)
                {
                    return;;
                }
#endif
            }
            else
            {
                btui_av_cb.file_cb.fd = fd;
                drain_until |= DRAIN_UNTIL_SCO_DISC;
                btui_wav_start_timer(btui_wav_get_clk_tick());
                LOGE( "Timer-tick: Suspended" );
            }
        }
        else if( (btui_av_cb.file_cb.fd != -1))
        {
            GKI_send_event(AV_SRC_READ_TASK, BTA_AV_SRC_UNDERRUN_EVT);//Always read the data

            if(btui_av_cb.timer_started)
            {
               GKI_send_event(SBC_ENCODE_TASK, BTA_AV_TIMER_EVT);
            }

#if (BTL_AV_DBG==TRUE)
            log_tstamps_us("BTA_AV_TIMER_EVT");
#endif

                clock_gettime(CLOCK_MONOTONIC, &now);
                if (prev.tv_sec || prev.tv_nsec)
                {
                    /* last interval */
                    delta = (now.tv_sec - prev.tv_sec) * NSEC_PER_SEC + now.tv_nsec - prev.tv_nsec;
                    if (delta >=0 && delta <= 2 * POLL_INTERVAL_NSEC)
                    {
                        /* next interval */
                        delta = 2 * POLL_INTERVAL_NSEC - delta;

                        prev.tv_sec += (prev.tv_nsec + POLL_INTERVAL_NSEC) / NSEC_PER_SEC;
                        prev.tv_nsec = (prev.tv_nsec + POLL_INTERVAL_NSEC) % NSEC_PER_SEC;
                    }
                    else
                    {
                        /* reset now */
						num_pcm_bufs_to_read = (delta/POLL_INTERVAL_NSEC); //Number of ticks missed
						LOGE( "*******>>>>LATE-FOR-THE-PARTY<<<<******* = %d", delta );
                        delta = ((num_pcm_bufs_to_read+1) * POLL_INTERVAL_NSEC) - delta;

                        prev.tv_sec += (prev.tv_nsec + (num_pcm_bufs_to_read*POLL_INTERVAL_NSEC)) / NSEC_PER_SEC;
                        prev.tv_nsec = (prev.tv_nsec + (num_pcm_bufs_to_read*POLL_INTERVAL_NSEC)) % NSEC_PER_SEC;
						num_pcm_bufs_to_read++; //Read one more buffer than the number of ticks missed
						//Do not preload too many pcm buffers otherwise it will overwhelm the out_q
						if( num_pcm_bufs_to_read > AV_PRELOAD_PCM_BUF_COUNT )
							num_pcm_bufs_to_read = AV_PRELOAD_PCM_BUF_COUNT;
                    }
                }
                else
                {
                    delta = POLL_INTERVAL_NSEC;
                    prev.tv_sec = now.tv_sec;
                    prev.tv_nsec = now.tv_nsec;
                }

                sInterval.tv_sec = 0;
                sInterval.tv_nsec = delta;
                nanosleep(&sInterval, NULL);
	    }
	    else
	    {
			GKI_delay(2);
	    }
	}

    pthread_exit(0);

}

void btui_drain_socket( int fd )
{
    struct pollfd pfd;
    int count, ret;
    pfd.fd = fd;
    pfd.events = POLLIN;
    int i = 0;
    UINT8 tmpBuff[BTUI_CODEC_BUF_MAX_44_1kHz];

    APPL_TRACE_DEBUG0( "btui_drain_socket" );

    if( fd == -1 )
        return;

    while ( 1 )
    {
        ret = poll(&pfd, 1, 58);

        if ( (ret == - 1) || (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)))
        {
            LOGE( "Close detected from drain poll" );
            btui_codec_stop(BTA_AV_CODEC_SBC); // hardcode for now
            //close(fd);
            //btui_av_cb.file_cb.fd = -1;

            APPL_TRACE_DEBUG0("no data available on pcm socket");
            return;
        }

        count = recv(fd, tmpBuff, BTUI_CODEC_BUF_MAX, 0);

        if( count == 0 )
        {
		LOGE( "Close detected from drain recv" );
	        btui_codec_stop(BTA_AV_CODEC_SBC);
		return;
	}
	if( count == -1 )
            return;
        APPL_TRACE_DEBUG1("Draining count = %d", count );
        i += count;

	if (i >= BTUI_CODEC_BUF_MAX_44_1kHz)
	{
	    APPL_TRACE_DEBUG0("stop drain socket");
	    return;
	}
    }
}


UINT32 btui_wav_start_timer(UINT32 tick_period)
{

#if USE_BCMBTUI  == TRUE
  APPL_TRACE_DEBUG0("btui_wav_start_timer");
#endif

  if (btui_av_cb.started) {
    GKI_send_event(SBC_ENCODE_TASK, BTA_AV_TIMER_EVT);


	num_pcm_bufs_to_read = AV_PRELOAD_PCM_BUF_COUNT;

	GKI_send_event(AV_SRC_READ_TASK, BTA_AV_SRC_UNDERRUN_EVT);
  }

  if (!btui_av_cb.timer_started)
  {
    btui_av_cb.timer_started = TRUE;

    wav_timer_period = ((tick_period + 500) / 1000);

    if (!wav_timer_period)
    {
#if USE_BCMBTUI  == TRUE
      APPL_TRACE_DEBUG2("tick_period %d wav_timer_period %d\n", tick_period, wav_timer_period);
#endif
    }


  }
  else
  {
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_wav_start_timer: Timer is already started!");
#endif
  }

  return (1);
}

void btui_wav_stop_timer(void)
{
#if USE_BCMBTUI  == TRUE
  APPL_TRACE_DEBUG0("btui_wav_stop_timer");
#endif

  num_pcm_bufs_to_read = 0;

  if(btui_av_cb.timer_started)
  {
     btui_av_cb.timer_started = FALSE;
  }
  else
  {
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_wav_stop_timer: Timer is already stopped!");
#endif
  }
}

/*******************************************************************************
**
** Function         btui_wav_close
**
** Description      Close WAV file
**
** Returns          void
**
*******************************************************************************/
UINT32 btui_wav_close(tFILEHDR * f)
{
    APPL_TRACE_DEBUG1("btui_wav_close status: fd:%x", f->fd);
    if (f->fd)
        close (f->fd);
    f->fd = -1;

    return(1);
}

/*******************************************************************************
**
** Function         btui_wav_get_data
**
** Description      Get WAV data from somewhere(DSP or Buffer) to pbuffer
**
** Returns          void
**
*******************************************************************************/
UINT32 btui_wav_get_data(tFILEHDR * f, UINT8 * pbuffer, UINT32 datasize)
{
    static UINT8 *save_ptr;
    UINT32 total_len = 0;
    UINT32 copy_len;
    UINT32 in_used;
    UINT32 out_used;
    UINT32 eof = 0;
    UINT32 zero_length = 0;

    //APPL_TRACE_DEBUG2("btui_wav_get_data: pbuffer[%x] datasize[%d]", pbuffer, datasize);
    do {

        if (!save_length) {
            save_length = btui_wav_buffer_read(f, &mp3_buffer[0], datasize);
            save_ptr = mp3_buffer;
        }

        /* check for end of file */
        if (save_length <= zero_length) {
            eof = 1;
        }

        if (save_length) {
            if (btui_av_cb.file_cb.ResamplingNeeded) {
                out_used = bta_av_sbc_up_sample(save_ptr, pbuffer + total_len, save_length, (datasize - total_len), &in_used);
                total_len += out_used;
                save_ptr += in_used;
                save_length -= in_used;
            } else {
                copy_len = min(save_length, datasize - total_len);
                memcpy((pbuffer + total_len), save_ptr, copy_len);

                if (((*(pbuffer + total_len)) != 0x9C)  &&  ((*(pbuffer + total_len)) != 0x8C)) {
#if USE_BCMBTUI  == TRUE
                    APPL_TRACE_DEBUG2("btui_wav_get_data: pbuffer[%x] datasize[%d]", pbuffer, datasize);
#endif
                }

                total_len += copy_len;
                save_ptr += copy_len;
                save_length -= copy_len;
            }
        }

    } while (!eof && (total_len < datasize));

    return(total_len);
}

/*******************************************************************************
**
** Function         btui_wav_get_clk_tick
**
** Description      get the clock tick
**
**
** Returns          void
*******************************************************************************/
UINT32 btui_wav_get_clk_tick(void)
{
    return btui_av_cb.file_cb.tick_period;
}

/*******************************************************************************
**
** Function         btui_wav_set_clk_tick
**
** Description      Set the clock tick
**
**
** Returns          void
*******************************************************************************/
void btui_wav_set_clk_tick(UINT32 tick)
{
    btui_av_cb.file_cb.tick_period = tick;
}


/*******************************************************************************
**
** Function         btui_wav_buffer_read
**
** Description      Read pcm data from file
**
** Returns          void
**
*******************************************************************************/
UINT32 btui_wav_buffer_read(tFILEHDR *f, UINT8 *buf, UINT32 datasize)
{
    int count;
    int ret;
    struct pollfd pfd;

    pfd.fd = btui_av_cb.file_cb.fd;
    pfd.events = POLLIN;

    /* poll for underrun */
    ret = poll(&pfd, 1, 58);

    if( ret == 0 )
    {
        APPL_TRACE_DEBUG0("underrun");
        return 0;
    }

    if ( (ret == - 1) || (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)))
    {
        APPL_TRACE_EVENT1("A2dp socket closed (err %d), SUSPEND remote device", errno);

        btui_codec_stop(BTA_AV_CODEC_SBC);
        BTA_AvStop( TRUE );

#ifdef STORE_PCM_DATA
        if (recv_fp != NULL)
        {
         fclose(recv_fp);
        }
#endif
        return 0;
    }

    count = recv(f->fd, buf, datasize, 0);

#if USE_BCMBTUI  == TRUE
    if (count<datasize)
        APPL_TRACE_DEBUG3("only read %d bytes (fd %d) out of %d\n", count, f->fd, datasize);
#endif

    if( count == 0 ) //Other side closed the connection
    {
         APPL_TRACE_EVENT0("A2dp socket closed, SUSPEND remote device");
         btui_codec_stop(BTA_AV_CODEC_SBC);
         //close(f->fd);
         //f->fd = -1;

         if (av_was_suspended_by_hs == FALSE)
             BTA_AvStop( TRUE );
         else
         {
             APPL_TRACE_DEBUG0("Local close on a remote suspended device");
             av_was_suspended_by_hs = FALSE;
         }
#ifdef STORE_PCM_DATA
          if (recv_fp != NULL)
          {
              fclose(recv_fp);
          }
#endif
    }

    if (count == -1)
    {
#if USE_BCMBTUI  == TRUE
       APPL_TRACE_DEBUG1("btui_wav_buffer_read: NO data available %d", errno == EWOULDBLOCK);
       fprintf(stderr, "btui_wav_buffer_read: NO data available %d", errno == EWOULDBLOCK);
#endif
       return 0;
    }

#ifdef STORE_PCM_DATA
    if (recv_fp != NULL)
    {
        fwrite(buf, 1, count, recv_fp);
    }
#endif

    return count;
}


/*******************************************************************************
**
** Function         btui_codec_cback
**
** Description      This function should be called periodically to get PCM data.
**
** Returns          void
**
*******************************************************************************/
void btui_codec_cback(void)
{
    BTUI_CODEC_FR_PER_BUF = 20;
    BTUI_CODEC_BUF_MAX = 10240;

    btui_av_cb.buf_len = BTUI_CODEC_BUF_MAX;
    btui_av_cb.out_q_max = 16;

#if (BTL_AV_DBG==TRUE)
    log_tstamps_us("codec tmr event", 0);
#endif

    if ( btui_av_cb.started && (btui_av_cb.file_cb.fd != -1) )
    {
        if ((p_wav_hdr->dwBufferLength = btui_wav_get_data(&btui_av_cb.file_cb, p_wav_hdr->p_data, BTUI_CODEC_BUF_MAX)) == 0)
        {
            return;
        }

        p_wav_hdr->free = 0;
        p_wav_hdr->dwFlags = AUDIO_HDR_READY;

        p_wav_hdr = (tWAVHDR *) p_wav_hdr->p_next;
    }
    else
    {
        LOGE( "Not started yet!" );
        btui_drain_socket(btui_av_cb.file_cb.fd);
    }
}


/*******************************************************************************
**
** Function         btui_codec_next_buffer
**
** Description      Utility function to retrieve the next available ready
**                  waveIn pcm data buffer.  A ready buffer will have flags
**                  WHDR_DONE and WHDR_PREPARED set.  Control block variable
**                  btui_av_cb.buf_idx is used to keep track of which
**                  buffer is the earliest if more than one buffer is ready.
**
** Returns          Pointer to waveIn pcm data buffer.
**
*******************************************************************************/
static tWAVHDR *btui_codec_next_buffer(void)
{
    int i;
    UINT32 idx = btui_av_cb.buf_idx;
    tWAVHDR *p_hdr = NULL;
    tWAVHDR *ptemp = p_wav_hdr;



#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_codec_next_buffer");
#endif

    for (i = 0; i < BTUI_CODEC_NUM_BUF; i++) {
        if ((ptemp->dwFlags == AUDIO_HDR_READY) ) {
 //           idx = p_wav_hdr->index;
            ptemp->dwFlags = AUDIO_HDR_ENCODED;
            p_hdr = ptemp;
	     return p_hdr;
        }
		ptemp = ptemp->p_next;
    }

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("btui_codec_next_buffer p_hdr = 0x%x", p_hdr);
#endif

    return p_hdr;
}


/*******************************************************************************
**
** Function         btui_codec_setup_sbc_encoder
**
** Description      Set up SBC encoder structure before calling encoder init.
**
** Returns          void
**
*******************************************************************************/
static void btui_codec_setup_sbc_encoder(SBC_ENC_PARAMS * p_enc, tA2D_SBC_CIE * p_cfg, FormatChunk *fmt)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_codec_setup_sbc_encoder");
#endif

    if (fmt->dwSamplesPerSec < MIN_AV_SAMPLE_RATE) {
        fmt->dwSamplesPerSec = MIN_AV_SAMPLE_RATE;
    }

    if (fmt->dwSamplesPerSec > MAX_AV_SAMPLE_RATE) {
        fmt->dwSamplesPerSec = MAX_AV_SAMPLE_RATE;
    }

#ifdef SUPPORT_ONLY_48_AND_44_KHZ
    switch (fmt->dwSamplesPerSec) {
        case 11025:
        case 22050:
        case 44100:
            p_enc->s16SamplingFreq = SBC_sf44100;
            break;

        case 16000:
        case 32000:
        case 48000:
            p_enc->s16SamplingFreq = SBC_sf48000;
            break;

        default:
            if ((fmt->dwSamplesPerSec % 2000) == 0) {
                p_enc->s16SamplingFreq = SBC_sf48000;
            } else if (fmt->dwSamplesPerSec > 441000) {
                p_enc->s16SamplingFreq = SBC_sf48000;
            } else {
                p_enc->s16SamplingFreq = SBC_sf44100;
            }
    }
#else
    switch (fmt->dwSamplesPerSec) {
        case 16000:
            if (p_cfg->samp_freq & A2D_SBC_IE_SAMP_FREQ_16) {
                p_enc->s16SamplingFreq = SBC_sf16000;
            } else {
                p_enc->s16SamplingFreq = SBC_sf48000;
            }
            break;

        case 32000:
            if (p_cfg->samp_freq & A2D_SBC_IE_SAMP_FREQ_32) {
                p_enc->s16SamplingFreq = SBC_sf32000;
            } else {
                p_enc->s16SamplingFreq = SBC_sf48000;
            }
            break;

        case 11025:
        case 22050:
        case 44100:
            p_enc->s16SamplingFreq = SBC_sf44100;
            break;

        case 24000:
        case 48000:
            p_enc->s16SamplingFreq = SBC_sf48000;
            break;

        default:
            if ((fmt->dwSamplesPerSec % 2000) == 0) {
                if ((fmt->dwSamplesPerSec < 16000) && (p_cfg->samp_freq & A2D_SBC_IE_SAMP_FREQ_16)) {
                    p_enc->s16SamplingFreq = SBC_sf16000;
                } else if ((fmt->dwSamplesPerSec < 32000) && (p_cfg->samp_freq & A2D_SBC_IE_SAMP_FREQ_32)) {
                    p_enc->s16SamplingFreq = SBC_sf32000;
                } else {
                    p_enc->s16SamplingFreq = SBC_sf48000;
                }
            } else if (fmt->dwSamplesPerSec > 44100) {
                p_enc->s16SamplingFreq = SBC_sf48000;
            } else {
                p_enc->s16SamplingFreq = SBC_sf44100;
            }
    }
#endif
    btui_av_cb.file_cb.DstSps = SbcBitRates[p_enc->s16SamplingFreq];
    btui_av_cb.file_cb.samp_freq = AVBitRates[p_enc->s16SamplingFreq];
    btui_av_cb.file_cb.ResamplingNeeded = 0;

#if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
    if (fmt->dwSamplesPerSec != btui_av_cb.file_cb.DstSps) {
        btui_av_cb.file_cb.ResamplingNeeded = 1;

        if (fmt->wChannels == 2)
            bta_av_sbc_init_up_sample (fmt->dwSamplesPerSec, btui_av_cb.file_cb.DstSps, fmt->wBitsPerSample, fmt->wChannels);
        //  else
        //DAM  bta_av_sbc_init_up_sample_mono (fmt->dwSamplesPerSec, btui_av_cb.file_cb.DstSps, fmt->wBitsPerSample);
    }
#else
    if ((fmt->dwSamplesPerSec != btui_av_cb.file_cb.DstSps) ||(fmt->wChannels != 2)) {
        btui_av_cb.file_cb.ResamplingNeeded = 1;
        bta_av_sbc_init_up_sample (fmt->dwSamplesPerSec, btui_av_cb.file_cb.DstSps, fmt->wBitsPerSample, fmt->wChannels);
    }
#endif
    if (fmt->wChannels == 1) {
        p_cfg->ch_mode = A2D_SBC_IE_CH_MD_MONO;
        p_enc->s16NumOfChannels = 1;
        p_enc->u16BitRate = (p_enc->s16SamplingFreq == SBC_sf44100) ? 127 : 132;
    } else {
        p_cfg->ch_mode = A2D_SBC_IE_CH_MD_JOINT;
        p_enc->s16NumOfChannels = 2;
        p_enc->u16BitRate = (p_enc->s16SamplingFreq == SBC_sf44100) ? 229 : 237;
    }

    p_enc->s16NumOfSubBands = (p_cfg->num_subbands == A2D_SBC_IE_SUBBAND_4) ? 4 : 8;
    p_enc->s16NumOfBlocks = btui_codec_block_tbl[p_cfg->block_len >> 5];
    p_enc->s16AllocationMethod = (p_cfg->alloc_mthd == A2D_SBC_IE_ALLOC_MD_L) ? SBC_LOUDNESS : SBC_SNR;
    p_enc->s16ChannelMode = btui_codec_mode_tbl[p_cfg->ch_mode >> 1];
}

/*******************************************************************************
**
** Function         btui_codec_fr_len
**
** Description      Calculate frame length based on SBC configuration.
**
** Returns          Frame length in bytes.
**
*******************************************************************************/
static INT16 btui_codec_fr_len(SBC_ENC_PARAMS * p_enc)
{
    INT16 len;
    INT16 join;
    tBTUI_CODEC_CFG *p_cfg = &btui_av_cb.open_cfg;

    if (p_enc->s16ChannelMode == SBC_MONO || p_enc->s16ChannelMode == SBC_DUAL) {
        len = 4 + (p_enc->s16NumOfSubBands * p_enc->s16NumOfChannels / 2) +
              (p_enc->s16NumOfBlocks * p_enc->s16NumOfChannels * p_enc->s16BitPool / 8);
    } else {
        /* stereo and dual stereo */
        join = (p_enc->s16ChannelMode == SBC_JOINT_STEREO) ? 1 : 0;
        len = 4 + (p_enc->s16NumOfSubBands * p_enc->s16NumOfChannels / 2) +
              ((join * p_enc->s16NumOfSubBands) + (p_enc->s16NumOfBlocks * p_enc->s16BitPool) / 8);
    }

    return len;
}

/*******************************************************************************
**
** Function         btui_codec_fr_per_pkt
**
** Description      calculate frames per packet based on SBC configuration
**                  and peer mtu.
**
** Returns          void
**
*******************************************************************************/
static UINT16 btui_codec_fr_per_pkt(SBC_ENC_PARAMS * p_enc, UINT16 mtu)
{
    UINT16 fr_per_pkt;
    UINT16 fr_len;

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_codec_fr_per_pkt");
#endif

    fr_per_pkt = BTUI_CODEC_FR_MAX;

    fr_len = btui_codec_fr_len(p_enc);

    /* subtract SBC frame header size from mtu */
    mtu -= 1;

    if ((fr_per_pkt * fr_len) > mtu) {
        fr_per_pkt = mtu / fr_len;
        if (fr_per_pkt == 0)
            fr_per_pkt = 1;
    }

    return fr_per_pkt;
}

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
void btui_codec_init(void)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_codec_init");
#endif

    wav_hdr.riff_id = WAV_RIFF;
    wav_hdr.file_len = 882036;
    wav_hdr.wav_id = WAV_WAVE;
    wav_hdr.fmt.fmt_id = WAV_FMT;
    wav_hdr.fmt.fmt_chunk_size = 16;
    wav_hdr.fmt.wFormatTag = WAV_PCM_CODE;
    wav_hdr.fmt.wChannels = 2;
    wav_hdr.fmt.dwSamplesPerSec = 44100;
    wav_hdr.fmt.dwAvgBytesPerSec = 176400;
    wav_hdr.fmt.wBlockAlign = 4;
    wav_hdr.fmt.wBitsPerSample = 16;
    wav_hdr.data.data_id = WAV_DATA;

    BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_44_1kHz;
    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz;
}

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

void btui_codec_dumpinfo(void)
{
    APPL_TRACE_WARNING0("### :: codec open ::");
    APPL_TRACE_WARNING5("### mtu %d, %d bytes/frame, bitrate %d, nbr ch %d, freq %d",
                        btui_av_cb.bytes_per_fr, btui_av_cb.open_cfg.mtu,
                        btui_av_cb.open_cfg.bit_rate, btui_av_cb.open_cfg.Channels,
                        btui_av_cb.open_cfg.sbc_cfg.samp_freq);

    APPL_TRACE_WARNING6("### alloc : %d, blk len %d, chmode:%d, bitpool %d:%d, subbands %d",
                        btui_av_cb.open_cfg.sbc_cfg.alloc_mthd,
                        btui_av_cb.open_cfg.sbc_cfg.block_len,
                        btui_av_cb.open_cfg.sbc_cfg.ch_mode,
                        btui_av_cb.open_cfg.sbc_cfg.max_bitpool,
                        btui_av_cb.open_cfg.sbc_cfg.min_bitpool,
                        btui_av_cb.open_cfg.sbc_cfg.num_subbands);
    APPL_TRACE_WARNING0("###");
}

void btui_codec_open(tBTUI_CODEC_CFG * p_cfg)
{
    int x, num_entries = 0;

    tA2D_SBC_CIE cur_cie;

    UINT8 update_cur_codec_info = FALSE;

    APPL_TRACE_DEBUG0("btui_codec_open");
    memcpy(&btui_av_cb.open_cfg, p_cfg, sizeof(tBTUI_CODEC_CFG));

    /* initialize control block */
    btui_av_cb.thread_done = FALSE;
    btui_av_cb.buf_idx = 0;
    btui_av_cb.started = FALSE;
    btui_av_cb.pool_id = p_cfg->pool_id;
    btui_av_cb.offset = p_cfg->offset;
    btui_av_cb.p_cback = p_cfg->p_cback;
    btui_av_cb.open_cfg.num_snk = p_cfg->num_snk;
    btui_av_cb.open_cfg.num_seps = p_cfg->num_seps;
    btui_av_cb.open_cfg.sep_info_idx = p_cfg->sep_info_idx;
    btui_av_cb.file_cb.fd = -1;

    GKI_init_q(&btui_av_cb.out_q);

#if (MP3_DECODER==FALSE)
    {
        /* TBD: get the wav_hdr from initiazation, not here */
        //btui_av_cb.file_cb.fd  = open (juke_box_wav[current_song], O_RDWR, 0666);
        //read (btui_av_cb.file_cb.fd, &wav_hdr,sizeof(WaveChunk));
    }
#endif

#if (MP3_DECODER==FALSE)
    if (wav_hdr.riff_id == WAV_RIFF && wav_hdr.wav_id == WAV_WAVE && wav_hdr.fmt.fmt_id == WAV_FMT)
    {
        if (wav_hdr.fmt.wFormatTag != WAV_PCM_CODE) {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG1("File %s is not a PCM-coded WAVE-file\n", btui_av_cb.file_cb.filename);
#endif
            return;
        }

        btui_av_cb.file_cb.tick_period = 0;    /* set to zero so we can check after setting it */
        btui_av_cb.file_cb.fmt.dwAvgBytesPerSec = wav_hdr.fmt.dwAvgBytesPerSec;
        btui_av_cb.file_cb.fmt.dwSamplesPerSec = wav_hdr.fmt.dwSamplesPerSec;
        btui_av_cb.file_cb.fmt.fmt_chunk_size = wav_hdr.fmt.fmt_chunk_size;
        btui_av_cb.file_cb.fmt.fmt_id = wav_hdr.fmt.fmt_id;
        btui_av_cb.file_cb.fmt.wBitsPerSample = wav_hdr.fmt.wBitsPerSample;
        btui_av_cb.file_cb.fmt.wBlockAlign = wav_hdr.fmt.wBlockAlign;
        btui_av_cb.file_cb.fmt.wChannels = wav_hdr.fmt.wChannels;
        btui_av_cb.file_cb.fmt.wFormatTag = wav_hdr.fmt.wFormatTag;

        btui_av_cb.file_cb.fmt.dwSamplesPerSec = wav_hdr.fmt.dwSamplesPerSec;
        btui_av_cb.file_cb.fmt.dwInputDatatype = BTUI_INPUT_DATA_IS_PCM;
        p_cfg->dwInputDatatype = BTUI_INPUT_DATA_IS_PCM;
    } else {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("File %s is not a WAVE-file\n", btui_av_cb.file_cb.filename);
#endif
        return;
    }
#endif

    btui_av_cb.file_cb.tick_period = 0;    /* set to zero so we can check after setting it */
    btui_av_cb.file_cb.fmt.dwAvgBytesPerSec = wav_hdr.fmt.dwAvgBytesPerSec;
    btui_av_cb.file_cb.fmt.dwSamplesPerSec = wav_hdr.fmt.dwSamplesPerSec;
    btui_av_cb.file_cb.fmt.fmt_chunk_size = wav_hdr.fmt.fmt_chunk_size;
    btui_av_cb.file_cb.fmt.fmt_id = wav_hdr.fmt.fmt_id;
    btui_av_cb.file_cb.fmt.wBitsPerSample = wav_hdr.fmt.wBitsPerSample;
    btui_av_cb.file_cb.fmt.wBlockAlign = wav_hdr.fmt.wBlockAlign;
    btui_av_cb.file_cb.fmt.wFormatTag = wav_hdr.fmt.wFormatTag;

#if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
    btui_av_cb.file_cb.fmt.wChannels = p_cfg->Channels;
    btui_av_cb.file_cb.fmt.dwSamplesPerSec = p_cfg->dwSamplesPerSec;
    btui_av_cb.file_cb.fmt.dwInputDatatype = p_cfg->dwInputDatatype;
#else
    btui_av_cb.file_cb.fmt.wChannels = wav_hdr.fmt.wChannels;
    btui_av_cb.file_cb.fmt.dwSamplesPerSec = wav_hdr.fmt.dwSamplesPerSec;
    btui_av_cb.file_cb.fmt.dwInputDatatype = p_cfg->dwInputDatatype;
#endif

    /* initialize SBC encoder */
    memcpy(&cur_cie, &p_cfg->sbc_cie, sizeof(tA2D_SBC_CIE));
    // NOT Sure this is a good idea as the cur_codec_info is modified to claim support for all Sampling Frequencies,
    // when in fact we ONLY support 44.1 and 48KHz
    //cur_cie.samp_freq = p_cfg->sbc_cfg.samp_freq;
    btui_codec_setup_sbc_encoder(&btui_av_cb.encoder, &cur_cie, &btui_av_cb.file_cb.fmt);

    /* If the computed bitpool for the default_line_setting exceeds the peer device's bitpool range, then
        * we need to reduce the line speed setting to match the peer's max bitpool
        * Without this fix, av start shall fail with sinks that use lower max bitpool range. For example, Moto 820 uses
        * a max bitpool of 33. For the line setting of 228kbps, the computed bit pool is 35 which is greater than the sink's max bitpool.
        * So we have to reduce the bitrate to match 33. The following logic does just that. - Kausik
        */
    {
        INT16 bitrate_from_max_bitpool;
        INT16 s16FrameLen = 0;
        /* if the peer's codec capabilities are not known, then use the setconfig codec config */
        INT16 s16Bitpool = (btui_av_cb.peer_sbc_codec_info[6] > 0) ? btui_av_cb.peer_sbc_codec_info[6] :
                                                                     cur_cie.max_bitpool;

        APPL_TRACE_DEBUG2("peer_sbc_codec_info:0x%x cur_cie.max_bitpool:0x%x", btui_av_cb.peer_sbc_codec_info[6],
                              cur_cie.max_bitpool);
        SBC_ENC_PARAMS *pEncParams = &btui_av_cb.encoder;

        s16FrameLen = 4 + (4*pEncParams->s16NumOfSubBands*
            pEncParams->s16NumOfChannels)/8
            + ( ((pEncParams->s16ChannelMode - 2) *
            pEncParams->s16NumOfSubBands)
            + (pEncParams->s16NumOfBlocks * s16Bitpool) ) / 8;

        bitrate_from_max_bitpool = (8 * s16FrameLen * SbcBitRates[pEncParams->s16SamplingFreq])
                                    / (pEncParams->s16NumOfSubBands *
                                    pEncParams->s16NumOfBlocks * 1000);
        if (bitrate_from_max_bitpool < p_cfg->bit_rate) {
            p_cfg->bit_rate = bitrate_from_max_bitpool;
        }
        btui_av_cb.encoder.u16BitRate = p_cfg->bit_rate;
        APPL_TRACE_EVENT1("Recomputed BitRate for the SBC Encoder Init:%d",btui_av_cb.encoder.u16BitRate);
    }
    SBC_Encoder_Init(&btui_av_cb.encoder);
    APPL_TRACE_EVENT1("Recomputed BitPool after SBC_Encoder_Init:0x%x", btui_av_cb.encoder.s16BitPool);
    if (btui_av_cb.encoder.s16BitPool < cur_cie.min_bitpool)
    {
        cur_cie.min_bitpool = btui_av_cb.encoder.s16BitPool;
        APPL_TRACE_DEBUG1("btui_codec_open: SBC_Encoder_Init altered cur_cie.min_bitpool to 0x%x", cur_cie.min_bitpool);
    }
    else if (btui_av_cb.encoder.s16BitPool > cur_cie.max_bitpool)
    {
        cur_cie.max_bitpool = btui_av_cb.encoder.s16BitPool;
        APPL_TRACE_DEBUG1("btui_codec_open: SBC_Encoder_Init altered cur_cie.max_bitpool to 0x%x", cur_cie.max_bitpool);
    }

    /* calculate frames per packet based on mtu and frame period */
    btui_av_cb.fr_per_pkt = btui_codec_fr_per_pkt(&btui_av_cb.encoder, p_cfg->mtu);
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG2("bitpool:%d fr_per_pkt:%d", btui_av_cb.encoder.s16BitPool, btui_av_cb.fr_per_pkt);
#endif

    btui_av_cb.bytes_per_fr = btui_av_cb.encoder.s16NumOfBlocks *
                              btui_av_cb.encoder.s16NumOfChannels * btui_av_cb.encoder.s16NumOfSubBands * sizeof(SINT16);

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG2("***fr_per_pkt = %d, bytes_per_fr %d ****", btui_av_cb.fr_per_pkt, btui_av_cb.bytes_per_fr);
#endif

    switch (p_cfg->sbc_cie.samp_freq) {
        case 0x80:   //16000
            BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_16kHz;

            if (btui_av_cb.file_cb.fmt.wChannels == 1) {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    ;  // Fix Me Later
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_16kHz_Mono;
            } else {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    ;  // Fix Me Later
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_16kHz;
            }
            break;

        case 0x40:   // 32000
            BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_32kHz;

            if (btui_av_cb.file_cb.fmt.wChannels == 1) {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    ;  // Fix Me Later
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_32kHz_Mono;
            } else {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    ;  // Fix Me Later
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_32kHz;
            }
            break;

        case 0x20:   // 44100
            BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_44_1kHz;

            if (btui_av_cb.file_cb.fmt.wChannels == 1) {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz_SBC_MONO;
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz_Mono;
            } else {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz_SBC;
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_44_1kHz;
            }
            break;

        case 0x10:   // 48000
        default:
            BTUI_CODEC_FR_PER_BUF = BTUI_CODEC_FR_PER_BUF_48_kHz;

            if (btui_av_cb.file_cb.fmt.wChannels == 1) {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_48kHz_SBC_MONO;
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_48kHz_Mono;
            } else {
                if (p_cfg->dwInputDatatype == BTUI_INPUT_DATA_IS_SBC)
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_48kHz_SBC;
                else
                    BTUI_CODEC_BUF_MAX = BTUI_CODEC_BUF_MAX_48kHz;
            }
            break;
    }

    btui_av_cb.buf_len = BTUI_CODEC_BUF_MAX;

    if (btui_av_cb.fr_per_pkt < BTUI_CODEC_FR_PER_BUF) {
        btui_av_cb.out_q_max = ((BTUI_CODEC_FR_PER_BUF / btui_av_cb.fr_per_pkt) +
                                (BTUI_CODEC_FR_PER_BUF / 2)) * p_cfg->pkt_q_max / btui_av_cb.fr_per_pkt;
    } else {
        btui_av_cb.out_q_max = p_cfg->pkt_q_max;
    }

    btui_av_cb.out_q_max = (btui_av_cb.out_q_max > BTUI_CODEC_OUT_Q_MAX) ? BTUI_CODEC_OUT_Q_MAX : btui_av_cb.out_q_max;

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG2("bytes_per_fr:%d out_q_max:%d", btui_av_cb.bytes_per_fr, btui_av_cb.out_q_max);
#endif

    /* TBD: need or not ???*/
    num_entries = sizeof(asrc_m12_freq_tbl) / sizeof(asrc_m12_freq_tbl[0]);
    for (x = 0; x < num_entries; x++) {
        if (asrc_m12_freq_tbl[x] == btui_av_cb.file_cb.DstSps) {
            btui_wav_set_clk_tick(asrc_wav_period_tbl[x]);
            magic_number = magic_number_tbl[x];
        }
    }

    // After open, open_cfg sbc_current becomes what sbc_cie has been configured with
    btui_av_cb.open_cfg.sbc_current.min_bitpool = btui_av_cb.open_cfg.sbc_cie.min_bitpool;
    btui_av_cb.open_cfg.sbc_current.max_bitpool = btui_av_cb.open_cfg.sbc_cie.max_bitpool;

    // check if the SBC_Encoder_Init has changed the bitpool, then update sbc_cie
    // so that a reconfigure would happen before the start
    if ((btui_av_cb.open_cfg.sbc_cie.min_bitpool != cur_cie.min_bitpool) ||
        (btui_av_cb.open_cfg.sbc_cie.max_bitpool != cur_cie.max_bitpool))
    {
        btui_av_cb.open_cfg.sbc_cie.min_bitpool = cur_cie.min_bitpool;
        btui_av_cb.open_cfg.sbc_cie.max_bitpool = cur_cie.max_bitpool;

        update_cur_codec_info = TRUE;
        APPL_TRACE_DEBUG0("btui_codec_open: cur_codec_info needs to be updated with new bitpool range");
    }

    if ((p_cfg->sbc_cie.samp_freq != p_cfg->sbc_cfg.samp_freq &&
        p_cfg->sbc_cfg.samp_freq & p_cfg->sbc_cfg.samp_freq))
    {
        btui_av_cb.file_cb.ResamplingNeeded = 0;
        update_cur_codec_info = TRUE;
    }

    if (update_cur_codec_info)
    {
        memcpy(&p_cfg->sbc_cie, &cur_cie, sizeof(tA2D_SBC_CIE));
        A2D_BldSbcInfo(AVDT_MEDIA_AUDIO, &cur_cie, btui_av_cb.cur_codec_info);
    }

    btui_codec_dumpinfo();
}

/*******************************************************************************
**
** Function         btui_codec_notify
**
** Description
**
** Returns          void
**
*******************************************************************************/
void btui_codec_notify(void)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_codec_notify");
#endif

    switch (btui_av_cb.str_cfg) {
        case BTUI_AV_STR_CFG_WAV_2_SBC:
            {
                if (btui_av_is_need_reconfig())
                    btui_av_reconfig();
                else
                {
                    /* remove this for now to move it to btapp_av_start_play  */
                    //BTA_AvStart();
                }
            }
            break;

        default:
            break;
    }
}

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
void btui_codec_start(tBTA_AV_CODEC codec_type)
{
    int i;
    double tick_period;

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("### btui_codec_start: codec_type %d ####", codec_type);
#endif

    if (!btui_av_cb.started) {
        /* set up wave buffers */
        for (i = 0; i < BTUI_CODEC_NUM_BUF; i++) {
            btui_av_cb.buf[i].hdr.p_data = btui_av_cb.buf[i].data;
            btui_av_cb.buf[i].hdr.index = btui_av_cb.buf_idx++;
            btui_av_cb.buf[i].hdr.free = 1;
            btui_av_cb.buf[i].hdr.dwFlags = AUDIO_HDR_NOT_READY;
            if (i < BTUI_CODEC_NUM_BUF - 1) {
                btui_av_cb.buf[i].hdr.p_next = (UINT32) & btui_av_cb.buf[i + 1].hdr;
#if USE_BCMBTUI  == TRUE
                APPL_TRACE_DEBUG6("btui_av_cb.buf[%d] : hdr = 0x%p, p_data = 0x%p, index = %d, free = %d, p_next = 0x%p",
                                  i, (UINT32) & btui_av_cb.buf[i].hdr, btui_av_cb.buf[i].hdr.p_data,
                                  btui_av_cb.buf[i].hdr.index, btui_av_cb.buf[i].hdr.free, btui_av_cb.buf[i].hdr.p_next);
#endif
            }
        }
        btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr.p_next = (UINT32) & btui_av_cb.buf[0];
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG6("btui_av_cb.buf[%d] : hdr = 0x%p, p_data = 0x%p, index = %d, free = %d, p_next = 0x%p",
                          BTUI_CODEC_NUM_BUF - 1, (UINT32) & btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr,
                          btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr.p_data,
                          btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr.index,
                          btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr.free, btui_av_cb.buf[BTUI_CODEC_NUM_BUF - 1].hdr.p_next);
#endif

        btui_av_flush_tx_q(&(btui_av_cb.out_q));/* Flush any pcm data from mp3 decoder */
//        btui_drain_socket( btui_av_cb.file_cb.fd );
        switch (codec_type) {
            case BTA_AV_CODEC_SBC:
                {
                    /* start wave and set wav buffer pointer to 1st element */
                    p_wav_hdr = &btui_av_cb.buf[0].hdr;
#if USE_BCMBTUI  == TRUE
                    APPL_TRACE_DEBUG1("p_wav_hdr = 0x%p", p_wav_hdr);
#endif
                    btui_av_cb.started = TRUE;

                    /* Let the headphones hit play first */
                    btui_wav_start_timer(btui_wav_get_clk_tick());
                    save_length = 0;

                }break;

            default:
                break;
        }
    }
}

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
void btui_codec_stop(tBTA_AV_CODEC codec_type)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("### btui_codec_stop: codec_type %d ####", codec_type);
#endif

    btui_av_cb.started = FALSE;
    started_sent = FALSE;
    drain_until = 0;
    btui_wav_stop_timer();
    btui_wav_close(&btui_av_cb.file_cb);
    btui_av_flush_tx_q(&(btui_av_cb.out_q));/* Flush any pcm data from mp3 decoder */
}

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
void btui_codec_close(tBTA_AV_CODEC codec_type)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("#### btui_codec_close: codec_type %d ####", codec_type);
#endif

    btui_codec_stop(codec_type);
    btui_av_cb.thread_done = TRUE;

    if( wav_timer_thread_created )
    {
        pthread_kill( wav_timer_thread_id, SIGINT );
        pthread_join( wav_timer_thread_id, 0 );
	wav_timer_thread_created = FALSE;
    }

}


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
BT_HDR *btui_codec_readbuf(tBTA_AV_CODEC codec_type)
{
    BT_HDR *ret=NULL;

    //APPL_TRACE_DEBUG1("btui_codec_readbuf: codec_type %d", codec_type);

    switch (codec_type) {
        case BTA_AV_CODEC_SBC:
            {
                if (btui_av_cb.out_q.count)
                    ret = (BT_HDR *)GKI_dequeue(&btui_av_cb.out_q);
            }break;
        case BTA_AV_CODEC_M12:
            {
                if (btui_av_cb.out_q.count)
                    ret = (BT_HDR *)GKI_dequeue(&btui_av_cb.out_q);
            }break;
        default:
            {
#if USE_BCMBTUI  == TRUE
                APPL_TRACE_DEBUG0("btui_codec_readbuf: Unknown codec type");
#endif
            }break;
    }

    return(ret);
}

/*******************************************************************************
**
** Function         btui_codec_get_cur_idx
**
** Description      get the current SEP info index
**
**
** Returns          void
*******************************************************************************/
UINT8 btui_codec_get_cur_idx(void)
{
    return btui_av_cb.open_cfg.sep_info_idx;
}

/*******************************************************************************
**
** Function         btui_av_sbc_codec_init
**
** Description      inittialise the current codec
**
**
** Returns          void
*******************************************************************************/
void btui_av_sbc_codec_init(UINT8 *p_codec_info, UINT16 mtu)
{
    tA2D_STATUS status;

#if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
    switch ( p_codec_info[3] & A2D_SBC_IE_CH_MD_MSK) {  // Channel mode;
        case A2D_SBC_IE_CH_MD_MONO:            //       0x08    /* b3: mono */
            switch (p_codec_info[3]  & A2D_SBC_IE_SAMP_FREQ_MSK) {
                case A2D_SBC_IE_SAMP_FREQ_16:          //     0x80    /* b7:16  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 16000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_MONO_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_32:          //     0x40    /* b6:32  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 32000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_MONO_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_48:          //     0x10    /* b4:48  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 48000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_MONO_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_44:          //      0x20    /* b5:44.1kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 44100;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_44P1_MONO_LINE_SPEED_KBPS;
                    break;

                default:
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 44100;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_44P1_MONO_LINE_SPEED_KBPS;
                    break;
            }
            btui_av_cb.codec_cfg.Channels = 1;
            break;

        case A2D_SBC_IE_CH_MD_DUAL:             //       0x04    /* b2: dual */
        case A2D_SBC_IE_CH_MD_STEREO:         //       0x02    /* b1: stereo */
        case A2D_SBC_IE_CH_MD_JOINT:            //       0x01    /* b0: joint stereo *
            switch (p_codec_info[3]  & A2D_SBC_IE_SAMP_FREQ_MSK) {
                case A2D_SBC_IE_SAMP_FREQ_16:          //     0x80    /* b7:16  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 16000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_32:          //     0x40    /* b6:32  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 32000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_48:          //     0x10    /* b4:48  kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 48000;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_48_LINE_SPEED_KBPS;
                    break;

                case A2D_SBC_IE_SAMP_FREQ_44:          //      0x20    /* b5:44.1kHz */
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 44100;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_44P1_LINE_SPEED_KBPS;
                    break;

                default:
                    btui_av_cb.codec_cfg.dwSamplesPerSec = 44100;
                    btui_av_cb.codec_cfg.bit_rate = BT_AV_44P1_LINE_SPEED_KBPS;
            }
            btui_av_cb.codec_cfg.Channels = 2;
            break;

    }
#else
    btui_av_cb.codec_cfg.bit_rate = BT_AV_44P1_LINE_SPEED_KBPS;
#endif

    btui_av_cb.timestamp = 0;
    btui_av_cb.codec_cfg.offset = AVDT_MEDIA_OFFSET + BTA_AV_SBC_HDR_SIZE;
    btui_av_cb.codec_cfg.mtu = ((GKI_BUF3_SIZE - btui_av_cb.codec_cfg.offset - sizeof(BT_HDR)) < mtu) ?
                               (GKI_BUF3_SIZE - btui_av_cb.codec_cfg.offset - sizeof(BT_HDR)) : mtu;
    btui_av_cb.codec_cfg.pkt_q_max = BTUI_AV_PKT_Q_MAX;
    btui_av_cb.codec_cfg.pool_id = GKI_POOL_ID_3;
    btui_av_cb.codec_cfg.p_cback = bta_av_audio_codec_callback;
    btui_av_cb.codec_cfg.sep_info_idx = btui_av_cb.sbc_sep_idx;

    /* SBC encoder configuration */
    status = A2D_ParsSbcInfo(&btui_av_cb.codec_cfg.sbc_cie, p_codec_info, FALSE);
    if (status != A2D_SUCCESS) {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_ERROR1("ERROR btui_av_sbc_codec_init->A2D_ParsSbcInfo failed %d", status);
#endif
    }

    /* SBC current configuration */
    A2D_ParsSbcInfo(&btui_av_cb.codec_cfg.sbc_current, default_bta_av_co_current_codec_info, FALSE);
    /* SBC stream configuration */
    A2D_ParsSbcInfo(&btui_av_cb.codec_cfg.sbc_cfg, default_bta_av_co_peer_sbc_info, TRUE);

    memcpy(&bta_av_co_open_cie, &btui_av_cb.codec_cfg.sbc_cie, sizeof(tA2D_SBC_CIE));

    /* Open the btui codec service */
    btui_codec_open(&btui_av_cb.codec_cfg);
}

int timer_tick = 0;

void av_src_read_task( void *p )
{
    tBTUI_AV_CB *p_cb = &btui_av_cb;
    UINT16 event;
	int i;

	while (1) {
//		LOGE( "Read thread: waiting" );
		event = GKI_wait (EVENT_MASK(GKI_SHUTDOWN_EVT) |BTA_AV_SRC_UNDERRUN_EVT, 0);

		if (event == BTA_AV_SRC_UNDERRUN_EVT)
		{

			//APPL_TRACE_DEBUG0( "Waiting the first buffer" );

			for( i=0 ; i<num_pcm_bufs_to_read; i++ )
				btui_codec_cback();


			num_pcm_bufs_to_read = 1;
			//APPL_TRACE_DEBUG1( "btui_codec_num_bufs = %d", btui_codec_num_bufs() );
		}

		/* Check for GKI shutdown event */
		if (event & EVENT_MASK(GKI_SHUTDOWN_EVT))
			break;
	}

	LOGI("%s is exiting.", __FUNCTION__);
}

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
void sbc_encode_task(void *p)
{
    tBTUI_AV_CB *p_cb = &btui_av_cb;
    tWAVHDR *p_hdr;
    BT_HDR *p_buf = NULL;
    UINT32 wavein_offset;
    UINT16 event;
	int q_count = 0;

#if (SBC_NO_PCM_CPY_OPTION ==  TRUE)
    UINT16 i = 0;
    UINT16 total_frames = 0;
    UINT16 total_to_encode = 0;
    UINT8 *  tmp;
#endif

    while (1) {
wait_for_evt:
        event = GKI_wait(EVENT_MASK(GKI_SHUTDOWN_EVT) | BTA_AV_CONNECT_EVT |BTA_AV_TIMER_EVT , 0);
        /* When we get this event we exit the task  - should only happen on GKI_shutdown  */
        if (event & EVENT_MASK(GKI_SHUTDOWN_EVT))
            break;

        if (event & BTA_AV_CONNECT_EVT) {
		 struct sched_param param;
		 int policy;
		int sig = SIGINT;
		sigset_t sigSet;
		sigemptyset (&sigSet);
		sigaddset (&sigSet, sig);

		pthread_sigmask (SIG_BLOCK, &sigSet, NULL);

		    if (pthread_create( &wav_timer_thread_id,
		                      NULL,
		                      btui_wav_timer_cback,
		                      NULL) != 0 )
		    {
		      APPL_TRACE_DEBUG0("pthread_create failed!\n\r");
		      return 0;
		    }
		    wav_timer_thread_created = TRUE;
		    if(pthread_getschedparam(wav_timer_thread_id, &policy, &param)==0)
		    {
		        policy = GKI_LINUX_TIMER_POLICY;
		        param.sched_priority = GKI_LINUX_AV_TIMER_PRIORITY;
		        pthread_setschedparam(wav_timer_thread_id, policy, &param);
		    }

            continue;
        }

        if ((event != BTA_AV_TIMER_EVT) || !p_cb->timer_started) {
            continue;
        }

        if( timer_tick++ & 0x30 ) APPL_TRACE_DEBUG0( "timer_tick-consumer" );

		if (!p_cb->started) {

			if( drain_until & DRAIN_UNTIL_SCO_DISC )
			{
				if (AV_WAS_SUSPENDED_BY_AG == FALSE)
				{
					LOGE("SBC_ENCODE_TASK: Clearing Draining");
					drain_until &= (~DRAIN_UNTIL_SCO_DISC);
					btapp_av_start_play();
					started_sent = TRUE;
				}
				else
				{
//    					LOGE("SBC_ENCODE_TASK:  Draining");
//					btui_drain_socket(btui_av_cb.file_cb.fd);
				}
			}

			btui_av_flush_tx_q(&(p_cb->out_q));

			continue;
		}

//    	LOGE("SBC_ENCODE_TASK: Encoding");
        /* get next waveIn buffer */
        while (((p_hdr = btui_codec_next_buffer()) != NULL) && (!p_cb->thread_done)) {
			p_buf = NULL;//Wasted my 6 hours to find this, otherwise the next one was going in the weeds

            /* only process input pcm if started */
            if (p_cb->started)
            {
                /* only process full buffers; ignore otherwise */
                if (p_hdr->dwBufferLength == p_cb->buf_len)
                {
                    /* while we have codec data */
                    wavein_offset = 0;

#if (SBC_NO_PCM_CPY_OPTION ==  TRUE)
                    total_frames = p_hdr->dwBufferLength / p_cb->bytes_per_fr ;
#endif

											/* discard output packet if queue maximum reached */
#if 1            /* THis code is required to recover from the ait bandwidth congested scenario */ /* DAM - FIXME 091207  */
					if (p_cb->out_q.count >= p_cb->out_q_max) {
						int i;

						APPL_TRACE_DEBUG2( "sbc_encode_task: outq.count(%d) > out_q_max(%d)\n", p_cb->out_q.count, p_cb->out_q_max);
						LOGE( "Congested: making space for new data" );

						for( i=0; i<3; i++ )
							GKI_freebuf( GKI_dequeue(&p_cb->out_q) );


					}
#endif

                    while (wavein_offset != p_hdr->dwBufferLength) {
                        /* set up new packet buffer */
                        if (p_buf == NULL) {
                            if ((p_buf = (BT_HDR *) GKI_getpoolbuf(p_cb->pool_id)) == NULL) {
								LOGE( "Out of GKI buffers" );
                                break;
                            }
                            p_buf->offset = p_cb->offset;
                            p_buf->len = 0;
                            p_buf->layer_specific = 0;
                        }


                        /* if packet not full;  note we're storing
                        ** frames per packet in layer_specific
                        */
                        if (p_buf->layer_specific < p_cb->fr_per_pkt) {

#if (SBC_NO_PCM_CPY_OPTION ==  TRUE)
                            total_to_encode = (total_frames >= p_cb->fr_per_pkt) ? p_cb->fr_per_pkt : total_frames;
                            p_cb->encoder.u8NumPacketToEncode = total_to_encode;

                            p_cb->encoder.ps16PcmBuffer = (SINT16 *) (p_hdr->p_data + wavein_offset);

                            wavein_offset += (p_cb->bytes_per_fr * total_to_encode);

                            /* set up output to packet */
                            p_cb->encoder.pu8Packet = (UINT8 *) (p_buf + 1) + p_buf->offset + p_buf->len;
                            tmp = p_cb->encoder.pu8Packet;

                            /* run encoder */
                            SBC_Encoder(&p_cb->encoder);

                            /* the following is done for each frame */
                            // Must de-scramble each frame

                            for (i = 0; i < total_to_encode; i++) {
                                A2D_SbcChkFrInit(tmp);
                                A2D_SbcDescramble(tmp, p_cb->encoder.u16PacketLength);
                                tmp+= p_cb->encoder.u16PacketLength;
                            }

                            total_frames -= total_to_encode;
                            p_buf->len += (p_cb->encoder.u16PacketLength * total_to_encode);
                            p_buf->layer_specific += total_to_encode;

#else
                            /* copy next pcm data chunk to encoder struct */
                            memcpy(p_cb->encoder.as16PcmBuffer, p_hdr->p_data + wavein_offset, p_cb->bytes_per_fr);
                            wavein_offset += p_cb->bytes_per_fr;

                            /* set up output to packet */
                            p_cb->encoder.pu8Packet = (UINT8 *) (p_buf + 1) + p_buf->offset + p_buf->len;

                            /* run encoder */
                            SBC_Encoder(&p_cb->encoder);

                            /* the following is done for each frame */
                            A2D_SbcChkFrInit(p_cb->encoder.pu8Packet);
                            A2D_SbcDescramble(p_cb->encoder.pu8Packet, p_cb->encoder.u16PacketLength);

                            p_buf->len += p_cb->encoder.u16PacketLength;
                            p_buf->layer_specific++;
#endif //    #if (SBC_NO_PCM_CPY_OPTION ==  TRUE)
                        }

                        /* if pkt full */
                        if (p_buf->layer_specific == p_cb->fr_per_pkt) {
                            /* set timestamp increment */
                            p_buf->event = p_cb->encoder.s16NumOfBlocks * p_cb->encoder.s16NumOfSubBands * p_buf->layer_specific;

                            /* queue packet */
                            GKI_enqueue(&p_cb->out_q, p_buf);

				APPL_TRACE_DEBUG1( "que-count = %d\n", p_cb->out_q.count );
                            outq_max = max(outq_max, p_cb->out_q.count);
                            p_buf = NULL;
                        }
                    }
                }

                /* put wave buffer back in */
                p_hdr->index = p_cb->buf_idx++;

                p_hdr->free = 1;
                p_hdr->dwFlags = AUDIO_HDR_NOT_READY;
            }
	     else
	     {
			btui_av_flush_tx_q(&(p_cb->out_q));
		   goto wait_for_evt;
	     }
		 /* if we have a partially full packet send it */
		 if (p_buf != NULL) {
			 /* set timestamp increment */
			 p_buf->event = p_cb->encoder.s16NumOfBlocks * p_cb->encoder.s16NumOfSubBands * p_buf->layer_specific;

			 /* queue packet */
			 GKI_enqueue(&p_cb->out_q, p_buf);
			 outq_max = max(outq_max, p_cb->out_q.count);
			 p_buf = NULL;
		 }


        }

//		LOGE( "enqueued for OTA %d", p_cb->out_q.count );


        /* dump the data if suspended by hs */
        if (av_was_suspended_by_hs == TRUE) {
            LOGI("sbc codec drop packets due to remote suspend");
            btui_av_flush_tx_q(&(p_cb->out_q));
        }
        /* if data was queued call application callback */
        else if (!GKI_IS_QUEUE_EMPTY(&p_cb->out_q)) {
            (*p_cb->p_cback) (BTUI_CODEC_RX_READY);
        }
    }
    APPL_TRACE_DEBUG0( "SBC TASK: EXITING TASK" );
    /* no specific exit as done in gki_task_entry! */
}

#endif // BTA_AV_INCLUDED
