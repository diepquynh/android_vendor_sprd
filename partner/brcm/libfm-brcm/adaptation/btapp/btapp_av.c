/****************************************************************************
**
**  Name:          btapp_av.c
**
**  Description:   Contains application functions for advanced audio/video.
**
**
**  Copyright (c) 2004-2008, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "bt_target.h"
#include "gki.h"

#if ( defined BTA_AV_INCLUDED ) && ( BTA_AV_INCLUDED == TRUE )

#include "bta_api.h"
#include "bta_av_api.h"
#include "btui.h"
#include "btui_av.h"
#include "bd.h"
#include <stdio.h>
#include <string.h>
#include "btl_av_codec.h"
#include "btui_int.h"
#include "uinput.h"
#include "bte_appl.h"
#include "btapp_av.h"
#include "btu.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "dtun_api.h"


#define LOG_TAG "BTL-BTAPP_AV"
#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
#include "btapp_fm.h"
#endif

#ifndef BTAPP_AV_NO_SCAN
#define BTAPP_AV_NO_SCAN    TRUE
#endif


#define USE_BCMBTUI TRUE

/* Incoming data path socket address */
#define INC_DATA_SOCK_PATH  "/data/inc_data_path"

#if defined(BTA_AV_MIN_DEBUG_TRACES)
tSTACKDLL_TRACE_LEVEL btui_av_min_trace_level = {0};
tSTACKDLL_TRACE_LEVEL btui_av_max_trace_level = {BT_TRACE_LEVEL_DEBUG};
#endif


/* BTUI AV main control block */
tBTUI_AV_CB btui_av_cb;

TIMER_LIST_ENT tle;

extern BOOLEAN  av_was_suspended_by_hs;

/* this boolean is set if we initiate the AV connection */
static BOOLEAN av_connection_initiator = FALSE;

extern void btapp_rc_init(void);
extern void btui_platform_av_init(void);
extern void btui_platform_av_remote_cmd(UINT8 rc_handle, tBTA_AV_RC rc_id, tBTA_AV_STATE key_state);

void btapp_av_dtun_start(void);

static int uinput_fd = -1;

#if 0
#if (BTU_DUAL_STACK_INCLUDED == TRUE )
extern void btapp_dm_switch_bb2mm(void);
extern void btapp_dm_switch_mm2bb(void);
extern void btapp_dm_switch_bb2btc(void);
extern void btapp_dm_switch_btc2bb(void);
#endif
#endif

/*****************************************************************************
**  Local Function prototypes
*****************************************************************************/
static int send_event (int fd, uint16_t type, uint16_t code, int32_t value)
{
    struct uinput_event event;

    memset(&event, 0, sizeof(event));
    event.type  = type;
    event.code  = code;
    event.value = value;

    return write(fd, &event, sizeof(event));
}

static void send_key (int fd, uint16_t key, int pressed)
{
    if ((fd < 0) || !btl_cfg_get_avrcp_pass_thru_status()) {
        return;
    }

    LOGI("AVRCP: Send key %d (%d) fd=%d\n", key, pressed, fd);
    send_event(fd, EV_KEY, key, pressed);
    send_event(fd, EV_SYN, SYN_REPORT, 0);
}

static void handle_panel_passthrough (const unsigned char *operands, int operand_count)
{
    const char *status;
    int pressed;

    if (operand_count == 0)
        return;

    if (operands[0] & 0x80) {
        status = "released";
        pressed = 0;
    } else {
        status = "pressed";
        pressed = 1;
    }

    switch (operands[0] & 0x7F) {
        case BTA_AV_RC_PLAY:
            LOGI("AVRCP: PLAY %s\n", status);
            send_key(uinput_fd, KEY_PLAYPAUSE, pressed);
            break;
        case BTA_AV_RC_STOP:
            LOGI("AVRCP: STOP %s\n", status);
            send_key(uinput_fd, KEY_STOP, pressed);
            break;
        case BTA_AV_RC_PAUSE:
            LOGI("AVRCP: PAUSE %s\n", status);
            send_key(uinput_fd, KEY_PLAYPAUSE, pressed);
            break;
        case BTA_AV_RC_FORWARD:
            LOGI("AVRCP: NEXT %s\n", status);
            send_key(uinput_fd, KEY_NEXTSONG, pressed);
            break;
        case BTA_AV_RC_BACKWARD:
            LOGI("AVRCP: PREV %s\n", status);
            send_key(uinput_fd, KEY_PREVIOUSSONG, pressed);
            break;
        case BTA_AV_RC_REWIND:
            LOGI("AVRCP: REWIND %s\n", status);
            send_key(uinput_fd, KEY_REWIND, pressed);
            break;
        case BTA_AV_RC_FAST_FOR:
            LOGI("AVRCP: FAST FORWARD %s\n", status);
            send_key(uinput_fd, KEY_FORWARD, pressed);
            break;
        default:
            LOGI("AVRCP: unknown button %d %s\n", (operands[0] & 0x7F) , status);
            break;
    }
}

static int uinput_driver_check()
{
    if (access("/dev/uinput", O_RDWR)) {
        if (access("/dev/input/uinput", O_RDWR)) {
            if (access("/dev/misc/uinput", O_RDWR)) {
                LOGE("ERROR: uinput device is not in the system\n");
                return -1;
            }
        }
    }
    return 0;
}

static int uinput_create(char *name)
{
    struct uinput_dev dev;
    int fd, err, x = 0;

    fd = open("/dev/uinput", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/input/uinput", O_RDWR);
        if (fd < 0) {
            fd = open("/dev/misc/uinput", O_RDWR);
            if (fd < 0) {
                LOGI("Can't open uinput device\n");
                return -1;
            }
        }
    }

    memset(&dev, 0, sizeof(dev));
    if (name)
        strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE);

    dev.id.bustype = BUS_BLUETOOTH;
    dev.id.vendor  = 0x0000;
    dev.id.product = 0x0000;
    dev.id.version = 0x0000;

    if (write(fd, &dev, sizeof(dev)) < 0) {
        LOGI("Can't write device information\n");
        close(fd);
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
  //ioctl(fd, UI_SET_EVBIT, EV_REP);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);

    ioctl(fd, UI_SET_KEYBIT, KEY_PLAYPAUSE);
    ioctl(fd, UI_SET_KEYBIT, KEY_STOP);
    ioctl(fd, UI_SET_KEYBIT, KEY_NEXTSONG);
    ioctl(fd, UI_SET_KEYBIT, KEY_PREVIOUSSONG);
    ioctl(fd, UI_SET_KEYBIT, KEY_REWIND);
    ioctl(fd, UI_SET_KEYBIT, KEY_FORWARD);

    for(x = 0; x < KEY_MAX; x++)
        ioctl(fd, UI_SET_KEYBIT, x);

    if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
        LOGI("Can't create uinput device\n");
        close(fd);
        return -1;
    }

    return fd;
}

static int init_uinput (void)
{
    char *name = "AVRCP";

    uinput_fd = uinput_create(name);
    if (uinput_fd < 0) {
        LOGI("AVRCP: Failed to initialize uinput for %s (%d)\n", name, uinput_fd);
    } else {
        LOGI("AVRCP: Initialized uinput for %s (fd=%d)\n", name, uinput_fd);
    }

    return uinput_fd;
}

static void close_uinput (void)
{
    if (uinput_fd > 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);

        close(uinput_fd);
        uinput_fd = -1;
    }
}

int strcasecmp(const char* str1, const char* str2)
{
   int result = 0;
   int done = 0;
   int pos = 0;
   while (!done)
   {
      if (tolower(str1[pos]) != tolower(str2[pos]))
      {
         if (tolower(str1[pos]) < tolower(str2[pos]))
         {
            result = -1;
         }
         else
         {
            result = 1;
         }
         done = 1;
      }
      else if (str1[pos] == '\0')
      {
         done = 1;
      }
      pos++;
   }
   return result;
}



const char *av_service_names = "BRCM Advanced Audio";

/***************************************************************************
 *  btui_av_init
 *
 *
 *  - Argument:
 *
 *
 *  - Description: This file contains Advanced Audio ui init functions
 *
 ***************************************************************************/
void btui_av_init(void)
{
  memset(&btui_av_cb, 0, sizeof( tBTUI_AV_CB));

  btui_av_cb.pref_codec = BTA_AV_CODEC_SBC;
  btui_av_cb.str_cfg = BTUI_AV_STR_CFG_WAV_2_SBC;

  APPL_TRACE_DEBUG0("AV : btui_av_init() ");
}

/***************************************************************************
 *  btui_a2dp_init
 *
 *
 *  - Argument:
 *
 *
 *  - Description: Initialize socket for incoming A2DP data path
 *
 ***************************************************************************/
void btui_a2dp_init(UINT8 *bd_addr)
{
    struct sockaddr_un local;
    int s, len;

	int z;

	int sndbuf=0;	 /* Send buffer size */
	int rcvbuf=0;/* Receive buffer size */
	socklen_t optlen;	/* Option length */

    APPL_TRACE_DEBUG0("AV : btui_a2dp_init() ");

    unlink(INC_DATA_SOCK_PATH);

    if ((s = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return;

    }

    local.sun_family = AF_LOCAL;
    strcpy(local.sun_path, INC_DATA_SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        return;
    }
    /* ready to receive connection from audio data source */
    if (listen(s, 3) == -1) {
        perror("listen");
        return;
    }

    btui_av_cb.serv_fd = s;
}

/*******************************************************************************
**
** Function         btapp_av_connect_device
**
** Description      Connects to the selected headset
**
**
** Returns          void
*******************************************************************************/
void btapp_av_connect_device(BD_ADDR bd_addr)
{
    av_connection_initiator = TRUE;
    BTA_AvOpen( bd_addr, btui_av_cb.hndl , TRUE, BTA_SEC_NONE);
}

/*******************************************************************************
**
** Function         btapp_av_start_play
**
** Description      Connects to the selected headset
**
**
** Returns          void
*******************************************************************************/
void btapp_av_start_play (void)
{
    APPL_TRACE_EVENT0("### AV START ###");
    btui_codec_notify();
    BTA_AvStart();
}

/***************************************************************************
 *  btui_av_app_callback
 *
 *  - Argument:    event   AV event
 *                 p_data  AV data
 *
 *  - Description: AV callback from BTA
 *
 ***************************************************************************/
BOOLEAN btui_av_app_handle_msg( tBTAPP_AV_MSG *p_msg )
{
    tDTUN_DEVICE_SIGNAL sig;
    tBTA_AV_EVT         event = (tBTA_AV_EVT) p_msg->hdr.layer_specific;
    APPL_TRACE_DEBUG1("btui_av_app_handle_msg( %d )", event );
#if 0
    switch (event)
    {
        case BTA_AV_ENABLE_EVT:
	     BTA_AvRegister(BTA_AV_CHNL_AUDIO, av_service_names, 0);
            break;

        case BTA_AV_REGISTER_EVT:
#ifdef DEMO
            if ((bte_appl_cfg.rem_addr[0] == 0 &&
                 bte_appl_cfg.rem_addr[1] == 0 &&
                 bte_appl_cfg.rem_addr[2] == 0 &&
                 bte_appl_cfg.rem_addr[3] == 0 &&
                 bte_appl_cfg.rem_addr[4] == 0 &&
                 bte_appl_cfg.rem_addr[5] == 0) ||
                (bte_appl_cfg.reconfig_baud < 921600))
            {
                APPL_TRACE_DEBUG0("btui_av_app_callback: cannot connect to A2dp since the snk address not specified");
                fprintf(stderr, "btui_av_app_callback: cannot connect to A2dp since the snk address not specified");
            }
            else
                btapp_av_connect_device(bte_appl_cfg.rem_addr);
#endif
            break;

	 case BTA_AV_OPEN_EVT:
           // TBD: need to add timer to send this event
           //usleep(2000000);

	   sig.av_event.info.event = event;
   	   sig.hdr.id = DTUN_SIG_AM_AV_EVENT;
	   sig.hdr.len = sizeof( tDTUN_SIG_AM_AV_INFO );
	   memset(sig.av_event.info.path, 0, sizeof(sig.av_event.info.path));
           strcpy(sig.av_event.info.path, INC_DATA_SOCK_PATH);
	   memcpy(sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6);
	   sig.av_event.info.status = BTA_AV_SUCCESS;
	   dtun_server_send_signal(&sig);

           GKI_send_event(SBC_ENCODE_TASK, BTA_AV_CONNECT_EVT);
           break;

	 case BTA_AV_START_EVT:
			APPL_TRACE_DEBUG0( "Connecting the SBC Codec\n" );

            break;

	 case BTA_AV_STOP_EVT:
	 	APPL_TRACE_DEBUG0( " Closing Now ****\n" );
	    	BTA_AvClose(btui_av_cb.hndl);
		break;

        case BTA_AV_CLOSE_EVT:
             /* TBD: open socket */
//             btui_a2dp_close();
            break;
    }

#endif

    return TRUE;
}

static void btapp_av_connect_notify_tmr(TIMER_LIST_ENT *tle)
{
    tDTUN_DEVICE_SIGNAL sig;
    APPL_TRACE_DEBUG0("btapp_av_connect_notify_tmr timed out");
    if ( (btui_av_cb.state == BTUI_AV_STATE_OPEN) ||
        (btui_av_cb.state == BTUI_AV_STATE_SUSPENDED) ||
        (btui_av_cb.state == BTUI_AV_STATE_CONNECTED_AS_ACP))
    {
        APPL_TRACE_DEBUG1("btapp_av_connect_notify_tmr: notify av state open. state: %d", btui_av_cb.state);
        sig.av_event.info.event = BTA_AV_OPEN_EVT;
        sig.hdr.id = DTUN_SIG_AM_AV_EVENT;
        sig.hdr.len = sizeof( tDTUN_SIG_AM_AV_INFO );
        memset(sig.av_event.info.path, 0, sizeof(sig.av_event.info.path));
        strncpy(sig.av_event.info.path, INC_DATA_SOCK_PATH, sizeof(sig.av_event.info.path));
        memcpy(sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6);
        sig.av_event.info.status = BTA_AV_SUCCESS;
        dtun_server_send_signal(&sig);
    }

    GKI_send_event(SBC_ENCODE_TASK, BTA_AV_CONNECT_EVT);
}

#if 0 /* TBD: not needed now? */
/*******************************************************************************
**
** Function         btui_av_chk_rc_open
**
** Description
**
** Returns          TRUE, if an AVRCP channel is open
*******************************************************************************/
BOOLEAN btui_av_chk_rc_open(void)
{
    BOOLEAN is_open = FALSE;
    int     xx = 0;

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        if (btui_av_cb.audio[xx].rc_open)
        {
            APPL_TRACE_DEBUG1("btui_av_chk_rc_open %d", xx);
            is_open = TRUE;
            break;
        }
    }

    /* rc only channel */
    if (btui_av_cb.audio[BTA_AV_NUM_STRS].rc_open)
    {
        APPL_TRACE_DEBUG0("btui_av_chk_rc_open rc_only");
        is_open = TRUE;
    }

    APPL_TRACE_DEBUG1("btui_av_chk_rc_open is_open:%d", is_open);
    return is_open;
}

/*******************************************************************************
**
** Function         btui_av_get_audio_ent
**
** Description      find the tBTUI_AV_AUDIO_ENT with the given bit "index"
**
** Returns          tBTUI_AV_AUDIO_ENT *
*******************************************************************************/
tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent(int index)
{
    int     xx = 0, count = 0;
    UINT8   audio;
    tBTUI_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        audio = BTUI_AV_HNDL_TO_MASK(btui_av_cb.audio[xx].av_handle);
        if (btui_av_cb.audio_open & audio)
        {
            /* this channel is open */
            count++;
            if (count == index)
            {
                p_ret = &btui_av_cb.audio[xx];
                break;
            }
        }
    }
    return p_ret;
}

#if (defined BTA_FM_INCLUDED &&(BTA_FM_INCLUDED == TRUE))
/*******************************************************************************
*******************************************************************************/
void btui_av_close_fm_device(void)
{
    int     xx = 0;
    UINT8   audio;

    btapp_av_stop_stream();

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        audio = BTUI_AV_HNDL_TO_MASK(btui_av_cb.audio[xx].av_handle);
        if ((btui_av_cb.audio_open & audio) && btui_av_cb.audio[xx].fm_connected)
        {
            btui_av_cb.fm_disconnect = TRUE;
            /* this channel is open */
            btapp_av_close(btui_av_cb.audio[xx].av_handle);
            btui_av_cb.audio[xx].fm_connected = FALSE;
        }
    }

    btui_av_cb.play_fm = FALSE;

}
#endif
/*******************************************************************************
**
** Function         btui_av_get_audio_ent_by_addr
**
** Description      find the tBTUI_AV_AUDIO_ENT with the given peer address
**
** Returns          tBTUI_AV_AUDIO_ENT *
*******************************************************************************/
tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent_by_addr(BD_ADDR addr)
{
    int     xx;
    tBTUI_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        if (bdcmp(btui_av_cb.audio[xx].peer_addr, addr) == 0)
        {
            p_ret = &btui_av_cb.audio[xx];
            break;
        }
    }

    if (!p_ret)
    {
        /* check rc only channel */
        if (bdcmp(btui_av_cb.audio[BTA_AV_NUM_STRS].peer_addr, addr) == 0)
            p_ret = &btui_av_cb.audio[BTA_AV_NUM_STRS];
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btui_av_get_audio_ent_by_hndl
**
** Description      find the tBTUI_AV_AUDIO_ENT with the given BTA AV handle
**
** Returns          tBTUI_AV_AUDIO_ENT *
*******************************************************************************/
tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent_by_hndl(tBTA_AV_HNDL handle)
{
    int     xx;
    tBTUI_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        if (btui_av_cb.audio[xx].av_handle == handle)
        {
            p_ret = &btui_av_cb.audio[xx];
            break;
        }
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btui_av_get_audio_ent_by_rc_hndl
**
** Description      find the tBTUI_AV_AUDIO_ENT with the given BTA AV RC handle
**
** Returns          tBTUI_AV_AUDIO_ENT *
*******************************************************************************/
tBTUI_AV_AUDIO_ENT * btui_av_get_audio_ent_by_rc_hndl(UINT8 handle)
{
    int     xx;
    tBTUI_AV_AUDIO_ENT *p_ret = NULL;

    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        if (btui_av_cb.audio[xx].rc_handle == handle && btui_av_cb.audio[xx].rc_open)
        {
            p_ret = &btui_av_cb.audio[xx];
            APPL_TRACE_DEBUG5("btui_av_get_audio_ent_by_rc_hndl xx:%d, rc_handle:%d av_handle:x%x addr:%06x-%06x",
                xx, handle, p_ret->av_handle,
                (p_ret->peer_addr[0] << 16) + (p_ret->peer_addr[1] << 8) + p_ret->peer_addr[2],
                (p_ret->peer_addr[3] << 16) + (p_ret->peer_addr[4] << 8) + p_ret->peer_addr[5]);
            break;
        }
    }

    if (!p_ret)
    {
        /* check rc only channel */
        if (btui_av_cb.audio[BTA_AV_NUM_STRS].rc_handle == handle && btui_av_cb.audio[BTA_AV_NUM_STRS].rc_open)
        {
            p_ret = &btui_av_cb.audio[BTA_AV_NUM_STRS];
            APPL_TRACE_DEBUG4("btui_av_get_audio_ent_by_rc_hndl rc only, rc_handle:%d av_handle:x%x addr:%06x-%06x",
                handle, p_ret->av_handle,
                (p_ret->peer_addr[0] << 16) + (p_ret->peer_addr[1] << 8) + p_ret->peer_addr[2],
                (p_ret->peer_addr[3] << 16) + (p_ret->peer_addr[4] << 8) + p_ret->peer_addr[5]);
        }
    }
    return p_ret;
}

/*******************************************************************************
**
** Function         btui_av_free_audio_ent
**
** Description      free the given tBTUI_AV_AUDIO_ENT
**
** Returns          void
*******************************************************************************/
void btui_av_free_audio_ent(tBTUI_AV_AUDIO_ENT *p_ent)
{
    BD_ADDR tmp_bdaddr={0, 0, 0, 0, 0, 0};
    UINT8   mask;

    if (p_ent)
    {
        mask = BTUI_AV_HNDL_TO_MASK(p_ent->av_handle);
        APPL_TRACE_DEBUG2("btui_av_free_audio_ent rc_open:%d, rc_play:%d", p_ent->rc_open, p_ent->rc_play);
        APPL_TRACE_DEBUG5("mask:x%x aopen:x%x, vopen:x%x addr:%06x-%06x", mask,
            btui_av_cb.audio_open, btui_av_cb.video_open,
            (p_ent->peer_addr[0] << 16) + (p_ent->peer_addr[1] << 8) + p_ent->peer_addr[2],
            (p_ent->peer_addr[3] << 16) + (p_ent->peer_addr[4] << 8) + p_ent->peer_addr[5]);
        if ((mask & btui_av_cb.audio_open) == 0 && (mask & btui_av_cb.video_open) == 0)
        {
            APPL_TRACE_DEBUG2("freed rc_open:%d, av_handle:x%x", p_ent->rc_open, p_ent->av_handle);
            if (p_ent->rc_open && p_ent->av_handle)
            {
                /* A2DP closes to cause the free ent - mark this ent as rc_only */
                p_ent->rc_only = TRUE;
            }
            else
            {
                p_ent->rc_supt = BTUI_AV_RC_SUPT_UNKNOWN;
                bdcpy(p_ent->peer_addr, tmp_bdaddr);
            }
            p_ent->rc_play = FALSE;
        }
        else
        {
            /* A2DP for VDP is still connected. mark rc_open FALSE */
            p_ent->rc_open = FALSE;
            p_ent->rc_play = FALSE;
        }
    }
}

/*******************************************************************************
**
** Function         btui_av_get_unused_audio_hndl
**
** Description      find an unused BTA AV handle
**
** Returns          tBTA_AV_HNDL
*******************************************************************************/
tBTA_AV_HNDL btui_av_get_unused_audio_hndl(tBTA_AV_HNDL hndl)
{
    tBTA_AV_HNDL handle = 0, temp;
    int     xx;
    UINT8   audio;

    APPL_TRACE_DEBUG2("btui_av_get_unused_audio_hndl:x%x, hndl:x%x",
        btui_av_cb.audio_open, hndl);
    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        temp = xx+1;
        audio = BTA_AV_HNDL_TO_MSK(temp);
        temp |= BTA_AV_CHNL_AUDIO;
        APPL_TRACE_DEBUG2("audio:x%x, temp:x%x", audio, temp);
        if ((btui_av_cb.audio_open & audio) == 0 && temp != hndl)
        {
            handle = temp;
            break;
        }
    }
    return handle;
}

/*******************************************************************************
**
** Function         btui_av_audio_open_count
**
** Description      make sure the audio_open_count is correct.
**                  Called after audio_open mask is changed
**
** Returns          void
*******************************************************************************/
static void btui_av_audio_open_count(void)
{
    int     xx = 0, count = 0;
    UINT8   audio;
    UINT8   mask, edr_count = 0;

    /* this mask is the EDR headsets that supports only 2Mbps */
    mask    = btui_av_cb.audio_2edr & (~btui_av_cb.audio_3edr);
    for (xx=0; xx<btui_av_cb.num_count; xx++)
    {
        audio = BTUI_AV_HNDL_TO_MASK(btui_av_cb.audio[xx].av_handle);
        if (btui_av_cb.audio_open & audio)
        {
            /* this channel is open */
            count++;
            if (mask & audio)
                edr_count++;
        }
    }
    btui_av_cb.audio_open_count = count;
    btui_av_cb.audio_2edr_count = edr_count;
}

/*******************************************************************************
**
** Function         btapp_av_chk_a2dp_open
**
** Description      Given an AVRCP handle, check if any A2DP channel is open
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_av_chk_a2dp_open(tBTUI_AV_AUDIO_ENT  *p_ent)
{
    BOOLEAN is_open = FALSE;

    if (p_ent && p_ent->av_handle)
    {
        is_open = TRUE;
    }
    return is_open;
}

/*******************************************************************************
**
** Function         btapp_av_rc_play
**
** Description      AV received AVRCP play command or the equivelent (like PlayItem)
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_av_rc_play(tBTUI_AV_AUDIO_ENT  *p_ent)
{
    tBTA_AV_HNDL new_handle;
    UINT8   mask;
    BOOLEAN is_open = FALSE;

    APPL_TRACE_DEBUG3("btapp_av_rc_play rc_open:%d hdl rc:x%x, av:x%x", p_ent->rc_open, p_ent->rc_handle, p_ent->av_handle);
    new_handle = p_ent->av_handle;
    if (p_ent->av_handle == 0)
        new_handle = btui_av_get_unused_audio_hndl(0);
    btui_audio_read_cfg(NULL);

    /* check if A2DP is already open */
    mask = BTUI_AV_HNDL_TO_MASK(new_handle);
    if (btui_av_cb.audio_open & mask)
    {
        is_open = TRUE;
        APPL_TRACE_DEBUG0("rc_play causing api_start");
        btapp_av_start_stream();
    }
    else
    {
        p_ent->rc_play = TRUE;
        btapp_av_open(p_ent->peer_addr, new_handle);
    }

    return is_open;
}


#if (BTU_DUAL_STACK_INCLUDED == TRUE )
/*******************************************************************************
**
** Function         btui_av_audio_start_count
**
** Description      Gte the number of audio-streams ongoing
**
** Returns          num audio streams started
*******************************************************************************/
static UINT8 btui_av_audio_start_count(void)
{
    int     xx = 0, count = 0;

    for (xx=0; xx<BTA_AV_NUM_STRS; xx++)
    {
        if (btui_av_cb.audio[xx].started)
        {
            count++;
        }
    }
    return count;
}
#endif

#endif /* if 0 TBD */


/*******************************************************************************
**
** Function         btui_av_cback
**
** Description      AV callback from BTA
**
**
** Returns          void
*******************************************************************************/
void btui_av_callback(tBTA_AV_EVT event, tBTA_AV *p_data)
{
    tDTUN_DEVICE_SIGNAL sig;

    APPL_TRACE_DEBUG1("btui_av_callback %d", event);

    sig.av_event.info.event = event;
    sig.hdr.id = DTUN_SIG_AM_AV_EVENT;
    sig.hdr.len = sizeof( tDTUN_SIG_AM_AV_INFO );

    switch (event) {
    case BTA_AV_ENABLE_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BTA_AV_ENABLE_EVT");
#endif
        av_was_suspended_by_hs = FALSE;

        BTA_AvRegister(BTA_AV_CHNL_AUDIO, av_service_names, 0);

    }break;


    case BTA_AV_REGISTER_EVT:
#if USE_BCMBTUI  == TRUE
           APPL_TRACE_DEBUG0("BTA_AV_REGISTER_EVT");
#endif
           if (p_data->registr.status == BTA_AV_SUCCESS)
           {
                /* TBD: need to differentiate with video??? */
                btui_av_cb.chnl = p_data->registr.chnl;
                btui_av_cb.hndl = p_data->registr.hndl;

#ifdef DEMO
            if ((bte_appl_cfg.rem_addr[0] == 0 &&
                 bte_appl_cfg.rem_addr[1] == 0 &&
                 bte_appl_cfg.rem_addr[2] == 0 &&
                 bte_appl_cfg.rem_addr[3] == 0 &&
                 bte_appl_cfg.rem_addr[4] == 0 &&
                 bte_appl_cfg.rem_addr[5] == 0) ||
                (bte_appl_cfg.reconfig_baud < 921600))
            {
                APPL_TRACE_DEBUG0("btui_av_app_callback: cannot connect to A2dp since the snk address not specified");
                fprintf(stderr, "btui_av_app_callback: cannot connect to A2dp since the snk address not specified");
            }
            else
                btapp_av_connect_device(bte_appl_cfg.rem_addr);
#endif
           }
           else
           {
#if USE_BCMBTUI  == TRUE
           APPL_TRACE_DEBUG0("BTA_AV_REGISTER_EVT - Failure");
#endif
			}

           break;
    case BTA_AV_PENDING_EVT:
    {
        APPL_TRACE_DEBUG0("BTA_AV_PENDING_EVT");
        if (av_connection_initiator == FALSE)
        {
            /* we are the acceptor of the connection.
                 * The newer headsets that comply with the AV whitepaper only initiate the signalling connection
                 * They expect the AV source to setup and start media channel.
                 * In order to interoperate with headsets that are not WP compliant, we start the
                 * AV open right away. The stack handles collisions if needed.
                 * TODO: In future, this AV_Open shall not be triggered here, instead only
                 * when the play is received
                 */
            btui_av_cb.state = BTUI_AV_STATE_CONNECTED_AS_ACP;
            BTA_AvOpen( p_data->pend.bd_addr, btui_av_cb.hndl , TRUE, BTA_SEC_NONE);
        }
    }break;
    case BTA_AV_OPEN_EVT:
    {
		APPL_TRACE_DEBUG0( "Opened\n" );
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BTA_AV_OPEN_EVT");
        APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x", \
                          p_data->open.bd_addr[0], p_data->open.bd_addr[1], \
                          p_data->open.bd_addr[2], p_data->open.bd_addr[3], \
                          p_data->open.bd_addr[4], p_data->open.bd_addr[5]);
        APPL_TRACE_DEBUG1("  status:%d", p_data->open.status);
#endif
	memcpy( bte_appl_cfg.rem_addr, p_data->open.bd_addr, 6 );
        av_was_suspended_by_hs = FALSE;

        APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x\n", \
                          p_data->open.bd_addr[0], p_data->open.bd_addr[1], \
                          p_data->open.bd_addr[2], p_data->open.bd_addr[3], \
                          p_data->open.bd_addr[4], p_data->open.bd_addr[5]);

	if (p_data->open.status == BTA_AV_SUCCESS)
	{
            btui_a2dp_init(p_data->open.bd_addr);

            /* if we are the acceptor of the connection, then send the open event right away */
            if (btui_av_cb.state != BTUI_AV_STATE_CONNECTED_AS_ACP) {
            /* start the timer for 2s */
            APPL_TRACE_DEBUG0("BTA_AV_OPEN_EVT start 2s timer");
            memset(&tle, 0, sizeof(tle));
            tle.param = (UINT32)btapp_av_connect_notify_tmr;
            btu_start_timer(&tle, BTU_TTYPE_USER_FUNC, 2);
	}
        else
        {
               btapp_av_connect_notify_tmr(NULL);
            }
            btui_av_cb.state = BTUI_AV_STATE_OPEN;
        }
        else
        {
            memset(sig.av_event.info.path, 0, sizeof(sig.av_event.info.path));
            memcpy(sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6);
            sig.av_event.info.status = p_data->open.status;
            dtun_server_send_signal(&sig);
            /* trace failure reason: BTA_AV_FAIL_SDP mostly likely page timeout, BTA_AV_FAIL_STREAM,
             * most likely getcapabilities error, possibily also page timeout */
            APPL_TRACE_WARNING1( "btui_av_callback(BTA_AV_OPEN_EVT::FAILED (page-timeout or protocol)::status: %d",
                                 p_data->open.status );
        }
    }break;
    case BTA_AV_CLOSE_EVT:
    {

#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BTA_AV_CLOSE_EVT");
#endif

	     /* CR 389777 - PTS issue. check if there is a pending open event, and force trigger that event here.
	          ** This will set the states correctly in the app(CachedBluetoothDevice), so that we clear the work queue
	          */
	     if ( tle.in_use )
	     {
	         APPL_TRACE_ERROR0("BTA_AV_CLOSE_EVT: Force triggering pending BTA_AV_OPEN_EVT");
	         btapp_av_connect_notify_tmr (NULL);
	     }

	     APPL_TRACE_DEBUG0( "Sending Closed Evt\n" );
	        btui_av_cb.state = BTUI_AV_STATE_CLOSED;
	        av_was_suspended_by_hs = FALSE;
	        av_connection_initiator = FALSE;
	    close(   btui_av_cb.serv_fd );

	        //TBD: btapp_rc_change_play_status (AVRC_PLAYSTATE_STOPPED);
	    memcpy( sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6 );

	    dtun_server_send_signal(&sig);

    }break;

    case BTA_AV_START_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("BTA_AV_START_EVT  status:%d", p_data->start.status);
#endif
	APPL_TRACE_DEBUG1("BTA_AV_START_EVT  status:%d", p_data->start.status);

        if( p_data->start.status == BTA_SUCCESS )
        {
        btui_av_cb.state = BTUI_AV_STATE_STARTED;
        av_was_suspended_by_hs = FALSE;
            if (p_data->start.suspending == FALSE)
            {
                sig.av_event.info.status = p_data->start.status;
                sig.av_event.info.path[0] = 0;
                memcpy( sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6 );
                dtun_server_send_signal(&sig);
            }
        }
		else
		{
	        btui_av_cb.state = BTUI_AV_STATE_STOPPED;

		}

        //TBD: btapp_rc_change_play_status (AVRC_PLAYSTATE_PLAYING);

    }break;

    case BTA_AV_STOP_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("BTA_AV_STOP_EVT  status:%d", p_data->start.status);
#endif

	APPL_TRACE_DEBUG1( "BTA_AV_STOP_EVT  status:%d\n", p_data->suspend.status );
	if( btui_av_cb.state == BTUI_AV_STATE_DISCONNECTING )
	{

	        btui_av_cb.state = BTUI_AV_STATE_STOPPED;

                BTA_AvClose(btui_av_cb.hndl);
	}

    }break;

    case BTA_AV_PROTECT_REQ_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("BTA_AV_PROTECT_REQ_EVT  data len:%d", p_data->protect_req.len);
#endif
        /* send protect response with same data for testing purposes */

        BTA_AvProtectRsp(p_data->protect_req.hndl, BTA_AV_ERR_NONE,  p_data->protect_req.p_data,  p_data->protect_req.len);

    }break;

    case BTA_AV_PROTECT_RSP_EVT:

    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG2("BTA_AV_PROTECT_RSP_EVT  err_code:%d len:%d", p_data->protect_rsp.err_code, p_data->protect_rsp.len);
#endif
    }break;

    case BTA_AV_RC_OPEN_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("BTA_AV_RC_OPEN_EVT  peer_features:%x", p_data->rc_open.peer_features);
#endif
        init_uinput();
    }break;

    case BTA_AV_RC_CLOSE_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BTA_AV_RC_CLOSE_EVT");
#endif
        close_uinput();
    }break;

    case BTA_AV_REMOTE_CMD_EVT:
    {
#if USE_BCMBTUI  == TRUE
        LOGI("BTA_AV_REMOTE_CMD_EVT  rc_id:%x key_state:%d", p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
#endif
        bta_av_handle_rc_cmd(p_data->remote_cmd.rc_id, p_data->remote_cmd.key_state);
    }break;

    case BTA_AV_REMOTE_RSP_EVT:
    {
#if USE_BCMBTUI  == TRUE
        LOGI("BTA_AV_REMOTE_RSP_EVT  rc_id:%d key_state:%d", p_data->remote_rsp.rc_id, p_data->remote_rsp.key_state);
        LOGI("  rsp_code:%d label:%d", p_data->remote_rsp.rsp_code, p_data->remote_rsp.label);
#endif
    }break;

    case BTA_AV_VENDOR_CMD_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG2("BTA_AV_VENDOR_CMD_EVT  code:%d label:%d", p_data->vendor_cmd.code, p_data->vendor_cmd.label);
        APPL_TRACE_DEBUG2("  company_id:0x%x len:%d", p_data->vendor_cmd.company_id, p_data->vendor_cmd.len);
#endif

        /* send vendor response with same data for testing purposes */
        BTA_AvVendorRsp(p_data->vendor_cmd.rc_handle, p_data->vendor_cmd.label, BTA_AV_RSP_ACCEPT, p_data->vendor_cmd.p_data, p_data->vendor_cmd.len, p_data->vendor_cmd.company_id);
    }break;

    case BTA_AV_VENDOR_RSP_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG2("BTA_AV_VENDOR_RSP_EVT  code:%d label:%d", p_data->vendor_rsp.code, p_data->vendor_rsp.label);
        APPL_TRACE_DEBUG2("  company_id:0x%x len:%d", p_data->vendor_rsp.company_id, p_data->vendor_rsp.len);
#endif
    }break;

    case BTA_AV_RECONFIG_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BTA_AV_RECONFIG_EVT");
        APPL_TRACE_DEBUG1("  status:%d", p_data->reconfig.status);
#endif
        btapp_av_start_play();
    }break;

    case BTA_AV_SUSPEND_EVT:
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_EVENT2("suspend:%d by initiator:%d",p_data->suspend.status, p_data->suspend.initiator);
#endif
        /* TBD: need to send ReConfig??? */

        btui_av_cb.state = BTUI_AV_STATE_SUSPENDED;

        if (p_data->suspend.initiator == FALSE)
            av_was_suspended_by_hs = TRUE;

            sig.av_event.info.status = p_data->suspend.status;
            sig.av_event.info.path[0] = 0;
            memcpy( sig.av_event.info.peer_addr.b, bte_appl_cfg.rem_addr, 6 );
            dtun_server_send_signal(&sig);
    }break;

    default:
        break;
    }


}


#if 0

/*******************************************************************************
**
** Function         btapp_av_init
**
** Description      Initializes AV
**
**
** Returns          void
*******************************************************************************/
void btapp_av_init(void)
{
    if (btui_cfg.av_included)
    {
        memset(&btui_av_cb, 0, sizeof(tBTUI_AV_CB));
#if (BTU_DUAL_STACK_BTC_INCLUDED == TRUE) && (BTU_DUAL_STACK_NETWORK_MUSIC_INCLUDED == TRUE)
        btui_av_cb.aux_path = btui_cfg.av_aux_path;
#endif
        if ((btui_cfg.av_features & (BTA_AV_FEAT_RCTG|BTA_AV_FEAT_RCCT)) == 0)
        {
            /* metadata & vendor specific commands are not valid features, if AVRCP CT/TG roles are not enabled */
            btui_cfg.av_features &= ~(BTA_AV_FEAT_VENDOR|BTA_AV_FEAT_METADATA);
        }
        if ((btui_cfg.av_features & BTA_AV_FEAT_METADATA) == 0)
        {
            /* advanced control is a superset of metadata */
            btui_cfg.av_features &= ~(BTA_AV_FEAT_ADV_CTRL);
        }
        if ((btui_cfg.av_features & BTA_AV_FEAT_ADV_CTRL) == 0)
        {
            /* browsing channel is a superset of advanced control */
            btui_cfg.av_features &= ~(BTA_AV_FEAT_BROWSE);
        }
        BTA_AvEnable(btui_cfg.av_security,
                     btui_cfg.av_features, btui_av_cback);
#if (defined(BTA_AV_MIN_DEBUG_TRACES) && BTA_AV_MIN_DEBUG_TRACES == TRUE)
        btui_av_min_trace_level.avp_trace_level = BT_TRACE_LEVEL_ERROR;
#endif

        btui_platform_av_init();

        bta_av_co_init();

        btapp_rc_init();
    }
}

/*******************************************************************************
**
** Function         btapp_av_increase_vol
**
** Description      increase volume
**
**
** Returns          void
*******************************************************************************/
void btapp_av_increase_vol(void)
{
    tBTUI_AV_AUDIO_ENT *p_ent = &btui_av_cb.audio[btui_av_cb.active_audio_idx];
    UINT8 rc_handle;

    if (p_ent && p_ent->rc_open)
    {
        rc_handle = p_ent->rc_handle;
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_UP, BTA_AV_STATE_PRESS);
        btapp_rc_next_label(p_ent);
        GKI_delay(200);
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_UP, AVRC_STATE_RELEASE);
        btapp_rc_next_label(p_ent);
    }
    else
    {
        APPL_TRACE_ERROR1("BTUI can not find CB for active_audio_idx:%d", btui_av_cb.active_audio_idx);
    }

}
/*******************************************************************************
**
** Function         btapp_av_decrease_vol
**
** Description      decrease volume
**
**
** Returns          void
*******************************************************************************/
void btapp_av_decrease_vol(void)
{
    tBTUI_AV_AUDIO_ENT *p_ent = &btui_av_cb.audio[btui_av_cb.active_audio_idx];
    UINT8 rc_handle;

    if (p_ent && p_ent->rc_open)
    {
        rc_handle = p_ent->rc_handle;
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_DOWN, BTA_AV_STATE_PRESS);
        btapp_rc_next_label(p_ent);
        GKI_delay(200);
        BTA_AvRemoteCmd(rc_handle, p_ent->label, BTA_AV_RC_VOL_DOWN, AVRC_STATE_RELEASE);
        btapp_rc_next_label(p_ent);
    }
    else
    {
        APPL_TRACE_ERROR1("BTUI can not find CB for active_audio_idx:%d", btui_av_cb.active_audio_idx);
    }
}

/*******************************************************************************
**
** Function         btapp_av_open_rc
**
** Description      open AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_open_rc(void)
{
    tBTUI_AV_AUDIO_ENT *p_ent = &btui_av_cb.audio[btui_av_cb.active_audio_idx];

    if (p_ent && !p_ent->rc_open)
    {
        BTA_AvOpenRc(p_ent->av_handle);
    }
    else
    {
        APPL_TRACE_ERROR1("BTUI can not find CB for active_audio_idx:%d", btui_av_cb.active_audio_idx);
    }
}

/*******************************************************************************
**
** Function         btapp_av_close_rc
**
** Description      close AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_close_rc(void)
{
    tBTUI_AV_AUDIO_ENT *p_ent = &btui_av_cb.audio[btui_av_cb.active_audio_idx];

    if (p_ent && p_ent->rc_open)
    {
        BTA_AvCloseRc(p_ent->rc_handle);
    }
    else
    {
        APPL_TRACE_ERROR1("BTUI can not find CB for active_audio_idx:%d", btui_av_cb.active_audio_idx);
    }
}

/*******************************************************************************
**
** Function         btapp_av_close_rc_only
**
** Description      close AVRC channel
**
**
** Returns          void
*******************************************************************************/
void btapp_av_close_rc_only(void)
{
    tBTUI_AV_AUDIO_ENT *p_ent = &btui_av_cb.audio[BTA_AV_NUM_STRS];

    if (p_ent && p_ent->rc_open)
    {
        BTA_AvCloseRc(p_ent->rc_handle);
    }
    else
    {
        APPL_TRACE_ERROR0("RC only connection is not open:%d");
    }
}

#endif /* if 0 */


/***************************************************************************
 *  +++ btui_av_enable +++
 *
 *
 *  - Argument: UINT8 av_en
 *
 *  - Description:
 *
 ***************************************************************************/
#ifndef BTAPP_AV_SECMASK
#define BTAPP_AV_SECMASK  (BTA_SEC_AUTHENTICATE)
#endif

void btapp_av_enable(UINT8 av_en)
{
    if (av_en == TRUE)
    {
        /* initialize aa control blocks */
        btui_av_init();

        /* register dtun interface */
        btapp_av_dtun_start();

        if (uinput_driver_check() == 0)
        {
           BTA_AvEnable(BTA_SEC_AUTHENTICATE, BTA_AV_FEAT_RCTG, btui_av_callback);
           //Removed Auth BTA_AvEnable(BTA_SEC_AUTHENTICATE | BTA_SEC_AUTHORIZE, BTA_AV_FEAT_RCTG, btui_av_callback);
            LOGI("AVRCP enabled\n");
        }
        else
        {
            BTA_AvEnable(BTAPP_AV_SECMASK , 0x00, btui_av_callback);
            //Removed Auth BTA_AvEnable(BTA_SEC_AUTHENTICATE | BTA_SEC_AUTHORIZE, 0x00, btui_av_callback);
        }
        LOGI("Advanced Audio enabled");
    }
    else
    {
        LOGI("AV disabled\n");
        BTA_AvDisable();
    }
}

/***************************************************************************
 *  Function       bta_av_handle_rc_cmd
 *
 *  - Argument:    tBTA_AV_RC rc_id   remote control command ID
 *                 tBTA_AV_STATE key_state status of key press
 *
 *  - Description: Remote control command handler
 *
 ***************************************************************************/
void bta_av_handle_rc_cmd (tBTA_AV_RC rc_id, tBTA_AV_STATE key_state)
{
    unsigned char operands[1];
    int operand_count = uinput_fd;

    LOGI("bta_av_handle_rc_cmd 0x%x %d\n", rc_id, key_state);

    switch (rc_id)
    {
        case BTA_AV_RC_PAUSE:
        case BTA_AV_RC_PLAY:
        {
            if (key_state == AVRC_STATE_RELEASE)
            {
                if (rc_id == BTA_AV_RC_PAUSE)
                {
                    operands[0] = (AVRC_ID_PAUSE | AVRC_KEYPRESSED_RELEASE);
                }
                else
                {
                    operands[0] = (AVRC_ID_PLAY | AVRC_KEYPRESSED_RELEASE);
                }
                handle_panel_passthrough(operands, operand_count);
                return;
            }

            if (btui_av_cb.timer_started == FALSE)
            {
                LOGI("PLAY\n");
                operands[0] = AVRC_ID_PLAY;
                handle_panel_passthrough(operands, operand_count);
            }
            else
            {
                LOGI("PAUSE\n");
                operands[0] = AVRC_ID_PAUSE;
                handle_panel_passthrough(operands, operand_count);
            }
        }
        break;

        case BTA_AV_RC_STOP:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("STOP Key Pressed\n");
                operands[0] = AVRC_ID_STOP;
            }
            else
            {
                LOGI("STOP Key Released\n");
                operands[0] = (AVRC_ID_STOP | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_FORWARD:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("FORWARD Key Pressed\n");
                operands[0] = AVRC_ID_FORWARD;
            }
            else
            {
                LOGI("FORWARD Key Released\n");
                operands[0] = (AVRC_ID_FORWARD | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_BACKWARD:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("BACKWARD Key Pressed\n");
                operands[0] = AVRC_ID_BACKWARD;
            }
            else
            {
                LOGI("BACKWARD Key Released\n");
                operands[0] = (AVRC_ID_BACKWARD | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_FAST_FOR:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("FAST FORWARD Key Pressed\n");
                operands[0] = AVRC_ID_FAST_FOR;
            }
            else
            {
                LOGI("FAST FORWARD Key Released\n");
                operands[0] = (AVRC_ID_FAST_FOR | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_REWIND:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("REWIND Key Pressed\n");
                operands[0] = AVRC_ID_REWIND;
            }
            else
            {
                LOGI("REWIND Key Released\n");
                operands[0] = (AVRC_ID_REWIND | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_VOL_UP:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("VOLUME UP Key Pressed\n");
                operands[0] = AVRC_ID_VOL_UP;
            }
            else
            {
                LOGI("VOLUME UP Key Released\n");
                operands[0] = (AVRC_ID_VOL_UP | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_VOL_DOWN:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("VOLUME DOWN Key Pressed\n");
                operands[0] = AVRC_ID_VOL_DOWN;
            }
            else
            {
                LOGI("VOLUME DOWN Key Released\n");
                operands[0] = (AVRC_ID_VOL_DOWN | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        case BTA_AV_RC_MUTE:
        {
            if (key_state == AVRC_STATE_PRESS)
            {
                LOGI("MUTE Key Pressed\n");
                operands[0] = AVRC_ID_MUTE;
            }
            else
            {
                LOGI("MUTE Key Released\n");
                operands[0] = (AVRC_ID_MUTE | AVRC_KEYPRESSED_RELEASE);
            }
            handle_panel_passthrough(operands, operand_count);
        }
        break;

        default:
            LOGI("bta_av_handle_rc_cmd: UNKNOWN RC Command\n");
            break;
    }
}

#if 0 /* HLONG: tmp */
void bta_av_handle_rc_cmd(tBTA_AV_RC rc_id, tBTA_AV_STATE key_state)
{

    unsigned char operands[1];
    int operand_count = uinput_fd;

#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("bta_av_handle_rc_cmd %x\n", rc_id);
#endif

    switch (rc_id)
    {
    case BTA_AV_RC_PAUSE:
    case BTA_AV_RC_PLAY:
    {
        if(key_state == AVRC_STATE_RELEASE)
	 {
            // Added for AVRCP support on Linux Systems
           if (rc_id == BTA_AV_RC_PAUSE)
              operands[0] = (PAUSE_OP | AVRC_KEYPRESSED_RELEASE);
	    else
              operands[0] = (PLAY_OP | AVRC_KEYPRESSED_RELEASE);

           handle_panel_passthrough(operands, operand_count);
	     return;
        }

        if (btui_av_cb.timer_started==FALSE)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("PLAY\n");
#endif

#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // callback to customer AV_CallbackRCPlay();
            // Added for AVRCP support on Linux Systems
           operands[0] = PLAY_OP;
           handle_panel_passthrough(	operands, operand_count);
	 //DAM   btui_wav_start_timer(btui_wav_get_clk_tick());

#else
           btui_wav_start_timer(btui_wav_get_clk_tick());
#endif
	}
	else
	{

#if USE_BCMBTUI  == TRUE
	    APPL_TRACE_DEBUG0("PAUSE\n");
#endif

#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	    // customer AV_CallbackRCPause();
	    // Added for AVRCP support on Linux Systems
           operands[0] = PAUSE_OP;
           handle_panel_passthrough(	operands, operand_count);
          //DAM btui_wav_stop_timer();
#else
	    btui_wav_stop_timer();
#endif
	}

    }break;

    case BTA_AV_RC_STOP:
    {
        if(key_state == AVRC_STATE_RELEASE)
        {
            // Added for AVRCP support on Linux Systems
           operands[0] = (STOP_OP | AVRC_KEYPRESSED_RELEASE);
           handle_panel_passthrough(	operands, operand_count);
           return;
    	 }
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("STOP\n");
#endif

#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
       //customer AV_CallbackRCStop();
       // Added for AVRCP support on Linux Systems
       operands[0] = STOP_OP;
       handle_panel_passthrough(operands, operand_count);
      //DAM btui_av_stop();
#else
       btui_av_stop();
#endif

    }break;

    case BTA_AV_RC_FORWARD:
    {
        if(key_state == AVRC_STATE_RELEASE)
	 {
            // Added for AVRCP support on Linux Systems
           operands[0] = (NEXT_OP | AVRC_KEYPRESSED_RELEASE);
           handle_panel_passthrough(	operands, operand_count);
	     return;
        }
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("FORWARD\n");
#endif

#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	// customer AV_CallbackRCForward();
       // Added for AVRCP support on Linux Systems
       operands[0] = NEXT_OP;
       handle_panel_passthrough(operands, operand_count);
       //DAMBTA_AvStop(FALSE);
#else
       #if defined( READ_SBC_FILE ) && ( READ_SBC_FILE == TRUE )
	     btui_wav_stop_timer();
            btui_wav_close(&btui_av_cb.file_cb);
            btui_wav_read_next();
	     btui_wav_start_timer(btui_wav_get_clk_tick());
       #else
	     btui_wav_stop_timer();
            btui_av_stop();
            btui_wav_close(&btui_av_cb.file_cb);
            btui_wav_read_next();
       #endif
#endif

    }break;

    case BTA_AV_RC_BACKWARD:
    {
        if(key_state == AVRC_STATE_RELEASE)
	 {
            // Added for AVRCP support on Linux Systems
           operands[0] = (PREV_OP | AVRC_KEYPRESSED_RELEASE);
           handle_panel_passthrough(	operands, operand_count);
	     return;
        }
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG0("BACKWARD\n");
#endif

#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	 // customer AV_CallbackRCBackward();
        // Added for AVRCP support on Linux Systems
        operands[0] = PREV_OP;
        handle_panel_passthrough(	operands, operand_count);
//DAM	 BTA_AvStop(FALSE);
#else
	#if defined( READ_SBC_FILE ) && ( READ_SBC_FILE == TRUE )
            btui_wav_stop_timer();
            btui_wav_close(&btui_av_cb.file_cb);
            btui_wav_read_prev();
	     btui_wav_start_timer(btui_wav_get_clk_tick());
	#else
	     btui_wav_stop_timer();
            btui_av_stop();
            btui_wav_close(&btui_av_cb.file_cb);
            btui_wav_read_prev();
    #endif
#endif

    }break;

    case BTA_AV_RC_FAST_FOR:
    {
        if(key_state == AVRC_STATE_PRESS)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("FAST FORWARD Key Pressed\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // customer AV_CallbackRCFastForward(AVRC_STATE_PRESS);
	     // Added for AVRCP support on Linux Systems
            operands[0] = FAST_FORWARD_OP;
            handle_panel_passthrough(operands, operand_count);
	     return;
#endif
        }
        else
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("FAST FORWARD Key Released\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	 {
             // customer AV_CallbackRCFastForward(AVRC_STATE_RELEASE);
	      // Added for AVRCP support on Linux Systems
            operands[0] = (FAST_FORWARD_OP | AVRC_KEYPRESSED_RELEASE);
            handle_panel_passthrough(operands, operand_count);
	     return;
        }
#endif
        }
    }break;

    case BTA_AV_RC_REWIND:
    {
        if(key_state == AVRC_STATE_PRESS)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("REWIND Key Pressed\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // customer AV_CallbackRCRewind(AVRC_STATE_PRESS);
	     // Added for AVRCP support on Linux Systems
            operands[0] = REWIND_OP;
            handle_panel_passthrough(operands, operand_count);

#endif
        }
        else
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("REWIND Key Released\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
            // customer AV_CallbackRCRewind(AVRC_STATE_RELEASE);
	     // Added for AVRCP support on Linux Systems
            operands[0] = (REWIND_OP | AVRC_KEYPRESSED_RELEASE);
            handle_panel_passthrough(operands, operand_count);

#endif
        }

    }break;

    case BTA_AV_RC_VOL_UP:
    {
        if(key_state == AVRC_STATE_PRESS)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("VOLUME UP Key Pressed\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // customer AV_CallbackRCRewind(AVRC_STATE_PRESS);
	     // Added for AVRCP support on Linux Systems
            operands[0] = VOLUP_OP;
            handle_panel_passthrough(operands, operand_count);

#endif
        }
        else
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("VOLUME UP Key Released\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
            // customer AV_CallbackRCRewind(AVRC_STATE_RELEASE);
	     // Added for AVRCP support on Linux Systems
            operands[0] = (VOLUP_OP | AVRC_KEYPRESSED_RELEASE);
            handle_panel_passthrough(operands, operand_count);

#endif
        }

    }break;

    case BTA_AV_RC_VOL_DOWN:
    {
        if(key_state == AVRC_STATE_PRESS)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("VOLUME DOWN Key Pressed\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // customer AV_CallbackRCRewind(AVRC_STATE_PRESS);
	     // Added for AVRCP support on Linux Systems
            operands[0] = VOLDOWN_OP;
            handle_panel_passthrough(operands, operand_count);

#endif
        }
        else
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("VOLUME DOWN Key Released\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
            // customer AV_CallbackRCRewind(AVRC_STATE_RELEASE);
	     // Added for AVRCP support on Linux Systems
            operands[0] = (VOLDOWN_OP | AVRC_KEYPRESSED_RELEASE);
            handle_panel_passthrough(operands, operand_count);

#endif
        }

    }break;

    case BTA_AV_RC_MUTE:
    {
        if(key_state == AVRC_STATE_PRESS)
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("MUTE Key Pressed\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
	     // customer AV_CallbackRCRewind(AVRC_STATE_PRESS);
	     // Added for AVRCP support on Linux Systems
            operands[0] = MUTE_OP;
            handle_panel_passthrough(operands, operand_count);

#endif
        }
        else
        {
#if USE_BCMBTUI  == TRUE
            APPL_TRACE_DEBUG0("MUTE Key Released\n");
#endif
#if (defined (MMP_SBC_ENCODER) && (MMP_SBC_ENCODER == TRUE)) || (defined(MP3_DECODER) && (MP3_DECODER == TRUE))
            // customer AV_CallbackRCRewind(AVRC_STATE_RELEASE);
	     // Added for AVRCP support on Linux Systems
            operands[0] = (MUTE_OP | AVRC_KEYPRESSED_RELEASE);
            handle_panel_passthrough(operands, operand_count);

#endif
        }

    }break;

    default:
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("bta_av_handle_rc_cmd: UNKNOWN RC Command\n");
#endif
    break;
    }
}
#endif /* if 0 */

/*******************************************************************************
**
** Function         btui_av_flush_tx_q
**
** Description     Flush the gki tx q.
**
**
** Returns          void
*******************************************************************************/
void btui_av_flush_tx_q(BUFFER_Q *pQueue)
{
    while(pQueue->count)
    {
        GKI_freebuf(GKI_dequeue(pQueue));
    }
}

/*******************************************************************************
**
** Function         btui_av_reconfig
**
** Description      initiate a reconfig.
**
**
** Returns          void
*******************************************************************************/
void btui_av_reconfig(void)
{
    BOOLEAN suspend = TRUE;
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_API6("btui_av_reconfig [%x;%x;%x;%x;%x;%x]", \
        btui_av_cb.cur_codec_info[1], btui_av_cb.cur_codec_info[2], btui_av_cb.cur_codec_info[3], \
        btui_av_cb.cur_codec_info[4], btui_av_cb.cur_codec_info[5], btui_av_cb.cur_codec_info[6]);
#endif

   if(btui_av_cb.str_cfg != BTUI_AV_STR_CFG_MP3_2_MP3)
        memcpy(&btui_av_cb.open_cfg.sbc_current, &btui_av_cb.open_cfg.sbc_cie, sizeof(tA2D_SBC_CIE));

    /* initiate reconfig */
    if(btui_av_cb.rmt_version.manufacturer == 18) /* If HP stereo headset, set the Suspend param as FALSE */
        suspend = FALSE;

    BTA_AvReconfig(btui_av_cb.hndl, suspend, btui_av_cb.sep_info_idx, (UINT8 *) btui_av_cb.cur_codec_info, 0, NULL);
}

/*******************************************************************************
**
** Function         btui_av_start
**
** Description      start the streaming
**
**
** Returns          void
*******************************************************************************/
void btui_av_start(const char *p_path)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_API1("btui_av_start filename %s", p_path);
#endif

    if(p_path == NULL)
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_ERROR1("ERROR btui_av_start: WRONG FILE %s", p_path);
#endif
        return;
    }

    if(btui_av_cb.state)
        BTA_AvStart();
}

/*******************************************************************************
**
** Function         btui_av_stop
**
** Description      stop the streaming
**
**
** Returns          void
*******************************************************************************/
void btui_av_stop(void)
{
    APPL_TRACE_EVENT1("### AV STOP (state %d) ###", btui_av_cb.state);

    BTA_AvStop(FALSE);

    btui_av_stop_tx_timer();
    btui_av_flush_tx_q(&(btui_av_cb.out_q));
}

/*******************************************************************************
**
** Function         btui_av_close
**
** Description      clost AA connetion
**
**
** Returns          void
*******************************************************************************/
void btui_av_close(void)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG1("btui_av_close state %d", btui_av_cb.state);
#endif

    BTA_AvClose(btui_av_cb.hndl);
}



void btui_av_disc(BD_ADDR bd_addr)
{
        btui_av_cb.state = BTUI_AV_STATE_DISCONNECTING;
	btui_av_stop();
}

/*******************************************************************************
**
** Function         btui_av_stop_tx_timer
**
** Description     start aa timer.
**
**
** Returns          void
*******************************************************************************/
void btui_av_stop_tx_timer(void)
{
#if USE_BCMBTUI  == TRUE
    APPL_TRACE_DEBUG0("btui_av_stop_tx_timer");
#endif

    btui_wav_stop_timer();
}

#if 0 /* TBD: */
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
void btui_av_timer_routine(void)
{
    UINT8 NbFrames;

    if(btui_av_cb.state != BTUI_AV_STATE_STARTED)
    {
#if USE_BCMBTUI  == TRUE
        APPL_TRACE_DEBUG1("btui_av_timer_routine wrong state %d", btui_av_cb.state);
#endif
        btui_av_stop_tx_timer();
        return;
    }

    if((NbFrames = btui_av_adjusting_timer()) == 0)
    {
        /* skip current timer */
        return;
    }

    btui_av_get_mult_packet(NbFrames);
    bta_av_ci_src_data_ready();
}

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
void btui_av_send_evt_4_aa(UINT16 event)
{
    btui_send_evt2mbox(TASK_MBOX_0, event);
}
#endif /* if 0 */

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
UINT8 btui_av_is_need_reconfig(void)
{
    UINT8 reconfig_needed = FALSE;

    if ((btui_av_cb.open_cfg.sbc_cie.min_bitpool != btui_av_cb.open_cfg.sbc_current.min_bitpool) ||
        (btui_av_cb.open_cfg.sbc_cie.max_bitpool != btui_av_cb.open_cfg.sbc_current.max_bitpool))
    {
        APPL_TRACE_DEBUG0("btui_av_is_need_reconfig: Is needed due to bitpool change");
        reconfig_needed = TRUE;
    }

#if defined( MONO_SUPPORT ) && ( MONO_SUPPORT == TRUE )
	if ((btui_av_cb.open_cfg.sbc_cie.samp_freq != btui_av_cb.open_cfg.sbc_current.samp_freq) ||
	     (btui_av_cb.open_cfg.sbc_cie.ch_mode != btui_av_cb.open_cfg.sbc_current.ch_mode))
#else
	if (btui_av_cb.open_cfg.sbc_cie.samp_freq != btui_av_cb.open_cfg.sbc_current.samp_freq)
#endif
    {
        APPL_TRACE_DEBUG0("btui_av_is_need_reconfig: Is needed due to sampling frequency change");
        reconfig_needed = TRUE;
    }

    return reconfig_needed;
}

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
void btui_av_start_timer(void)
{
    btui_wav_start_timer(btui_wav_get_clk_tick());
}

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
void btui_av_stop_timer(void)
{
    btui_wav_stop_timer();
}

/*******************************************************************************
**
** Function         btui_av_get_pref_codec
**
** Description      Get preferred codec
**
**
** Returns          If prefered codec is SBC, return BTA_AV_CODEC_SBC
**                  If prefered codec is MP3, treurn BTA_AV_CODEC_M12
**
*******************************************************************************/
UINT8 btui_av_get_pref_codec(void)
{
    return btui_av_cb.pref_codec;
}



void btapp_am_AvOpen(tDTUN_DEVICE_METHOD *msg)
{
	//Try opening an AV link with the headset

        btapp_av_connect_device(msg->av_open.bdaddr.b);
}

void btapp_am_AvDisc(tDTUN_DEVICE_METHOD *msg)
{
	BD_ADDR  bd_addr;

        APPL_TRACE_DEBUG6("  bd_addr:%02x-%02x-%02x-%02x-%02x-%02x\n", \
                          msg->av_disc.bdaddr.b[0], msg->av_disc.bdaddr.b[1], \
                          msg->av_disc.bdaddr.b[2], msg->av_disc.bdaddr.b[3], \
                          msg->av_disc.bdaddr.b[4], msg->av_disc.bdaddr.b[5]);

	btui_av_disc(msg->av_disc.bdaddr.b);

//        BTA_AvDisconnect(msg->av_disc.bdaddr);
}

void btapp_am_AvStartStop(tDTUN_DEVICE_METHOD *msg)
{
	//Try opening an AV link with the headset

	switch( msg->av_startstop.op ) {

		case 0: /* Start */
			BTA_AvStart();
			break;

		case 1: /* Stop */
			BTA_AvStop( FALSE );
			break;

		case 2: /* Suspend */
			BTA_AvStop( TRUE );
			break;
	}

}


#if 0
const tDTUN_METHOD av_method_tbl[] =
{
    btapp_am_AvOpen,
    btapp_am_AvDisc,
    btapp_am_AvStartStop
};
#endif

void btapp_av_dtun_start(void)
{
    APPL_TRACE_EVENT0("Starting DTUN [AV] Interface");
//    dtun_server_register_interface(DTUN_INTERFACE_AV, (tDTUN_METHOD*)&av_method_tbl);
//    dtun_server_start();
}



#endif
