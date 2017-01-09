/*****************************************************************************
**                                                                           *
**  Name:          btui_av.h                                                 *
**                                                                           *
**  Description:    This is the interface file for the btui                  *
**																			 *
**                                                                           *
**  Broadcom Bluetooth Core. Proprietary and confidential.                   *
******************************************************************************/
#ifndef BTUI_AV_H
#define BTUI_AV_H


/*==================================================

  Constans

 ===================================================*/
#define BTUI_AV_NUM_FILL_UP 3 /* in 40ms unit */

#define BTUI_AV_TX_TIMER_TICK_PERIOD 40000 /* in us unit */

#define BTUI_A2DP_EVENT BTUI_BTA_EVENT(BTA_ID_AV)

/* Enumeration for AA application */
enum
{
    /* Events from BTEM stack */
    BTUI_AV_ENABLE_EVT,       /* 0  AA enabled */
    BTUI_AV_OPEN_EVT,         /* 1  connection opened */
    BTUI_AV_CLOSE_EVT,        /* 2  connection closed */
    BTUI_AV_START_EVT,        /* 3  stream data transfer started */
    BTUI_AV_STOP_EVT,         /* 4  stream data transfer stopped */
    BTUI_AV_PROTECT_REQ_EVT,  /* 5  content protection request */
    BTUI_AV_PROTECT_RSP_EVT,  /* 6  content protection response */
    BTUI_AV_RC_OPEN_EVT,      /* 7  remote control channel open */
    BTUI_AV_RC_CLOSE_EVT,     /* 8  remote control channel closed */
    BTUI_AV_REMOTE_CMD_EVT,   /* 9  remote control command */
    BTUI_AV_REMOTE_RSP_EVT,   /* 10 remote control response */
    BTUI_AV_VENDOR_CMD_EVT,   /* 11 vendor dependent remote control command */
    BTUI_AV_VENDOR_RSP_EVT,   /* 12 vendor dependent remote control response */
    BTUI_AV_RECONFIG_EVT,     /* 13 reconfigure response */
    BTUI_AV_SUSPEND_EVT,      /* 14 suspend response */

    /* Events for AV application */
    BTUI_AV_DO_START_EVT,     /* 15 */
    BTUI_AV_DO_PLAY_EVT,      /* 16 */
    BTUI_AV_DO_STOP_EVT,      /* 17 */
    BTUI_AV_DO_PAUSE_EVT,     /* 18 */
    BTUI_AV_DO_REWIND_EVT,    /* 19 */
    BTUI_AV_DO_FAST_FOR_EVT,  /* 20 */
    BTUI_AV_DO_FORWARD_EVT,   /* 21 */
    BTUI_AV_DO_BACKWARD_EVT,  /* 22 */
    BTUI_AV_DO_RECONFIG_EVT  /* 23 */

};


/*==================================================

 FUNCTION PROTOTYPES

 ===================================================*/
/***************************************************************************
 *  Function       bta_av_handle_rc_cmd
 *
 *  - Argument:    tBTA_AV_RC rc_id   remote control command ID
 *                 tBTA_AV_STATE key_state status of key press
 *
 *  - Description: Remote control command handler
 *
 ***************************************************************************/
extern void bta_av_handle_rc_cmd(tBTA_AV_RC rc_id, tBTA_AV_STATE key_state);

/*******************************************************************************
**
** Function         btui_av_flush_tx_q
**
** Description     Flush the gki tx q.
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_flush_tx_q(BUFFER_Q *pQueue);

/*******************************************************************************
**
** Function         btui_av_reconfig
**
** Description      initiate a reconfig.
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_reconfig(void);

/*******************************************************************************
**
** Function         btui_av_open_pcm_source
**
** Description     copy pcm data in pBuf
**
**
** Returns          void
*******************************************************************************/
extern BOOLEAN btui_av_open_pcm_source(const char * p_path);

/*******************************************************************************
**
** Function         btui_av_close_pcm_source
**
** Description     copy pcm data in pBuf
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_close_pcm_source(void);

/*******************************************************************************
**
** Function         btui_av_open_mp3_file
**
** Description     Get mp3 streaming caracteristique
**
**
** Returns          TRUE if success
*******************************************************************************/
//extern BOOLEAN btui_av_open_mp3_file(const char * p_path, WaveChunk * p_wav_hdr);

/*******************************************************************************
**
** Function         btui_av_is_different_cfg
**
** Description      check difference between current codec config and file specified
**
**
** Returns          FALSE if the file specified has the same config than the current codec and previous
**                       and current file the same kind.
**                      TRUE if different
*******************************************************************************/
extern BOOLEAN btui_av_is_different_cfg(const char *p_path);

/*******************************************************************************
**
** Function         btui_av_start
**
** Description      start the streaming
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_start(const char *p_path);

/*******************************************************************************
**
** Function         btui_av_stop
**
** Description      stop the streaming
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_stop(void);

/*******************************************************************************
**
** Function         btui_av_close
**
** Description      clost AA connetion
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_close(void);

/*******************************************************************************
**
** Function         btui_av_m12_adjusting_timer
**
** Description
**
**
** Returns          void
*******************************************************************************/
extern UINT8 btui_av_m12_adjusting_timer(void);

/*******************************************************************************
**
** Function         btui_av_adjusting_timer
**
** Description
**
**
** Returns          void
*******************************************************************************/
extern UINT8 btui_av_adjusting_timer(void);

/*******************************************************************************
**
** Function         btui_av_get_mult_packet
**
** Description     get Numpkt multiumadia packet
**
**
** Returns          void
*******************************************************************************/
extern void  btui_av_get_mult_packet(UINT8 Numpkt);

/*******************************************************************************
**
** Function         btui_av_start_tx_timer
**
** Description     Start streaming timer that encodes and send packet to the sink.
**                      Connection must be open  and streaming started. See btui_av_timer_routine
**
** Returns          void
*******************************************************************************/
extern void btui_av_start_tx_timer(UINT32 timeout);


/*******************************************************************************
**
** Function         btui_av_stop_tx_timer
**
** Description     start aa timer.
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_stop_tx_timer(void);

/*******************************************************************************
**
** Function         btui_av_timer_routine
**
** Description     Streaming timer. It encode and send packet to the sink. Connection must be open
**                      and streaming started
**
**
** Returns          void
*******************************************************************************/
extern void btui_av_timer_routine(void);

/*******************************************************************************
**
** Function         btui_av_send_evt_4_aa
**
** Description      Send event to BTE_APPL_TASK for AA application
**
**
** Returns          void
**
*******************************************************************************/
extern void btui_av_send_evt_4_aa(UINT16 event);

/*******************************************************************************
**
** Function         btui_av_is_need_reconfig
**
** Description      Check the reconfiguration flag
**
**
** Returns          void
**
*******************************************************************************/
extern UINT8 btui_av_is_need_reconfig(void);

/*******************************************************************************
**
** Function         btui_av_start_timer
**
** Description      Start WAV timer
**
**
** Returns          void
**
*******************************************************************************/
extern void btui_av_start_timer(void);

/*******************************************************************************
**
** Function         btui_av_stop_timer
**
** Description      Stop WAV timer
**
**
** Returns          void
**
*******************************************************************************/
extern void btui_av_stop_timer(void);

/*******************************************************************************
**
** Function         btui_av_get_pref_codec
**
** Description      Get preferred codec
**
**
** Returns          If prefered codec is SBC, return BTA_AA_CODEC_SBC
**                  If prefered codec is MP3, treurn BTA_AA_CODEC_M12
**
*******************************************************************************/
extern UINT8 btui_av_get_pref_codec(void);

#endif
