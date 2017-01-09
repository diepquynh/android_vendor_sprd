/*****************************************************************************
**
**  Name:           bta_fm_co.c
**
**  Description:    This file contains the FM callout function implementation
**                   for Insight.
**
**  Copyright (c) 2003-2010, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_fm_api.h"
#include "bta_fm_co.h"

#include "btui.h"
#include "btapp_fm.h"

#define LOG_TAG "FM_CO"

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#include <stdio.h>
#include <string.h>
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#endif

/*******************************************************************************
**
** Function         bta_fm_co_init
**
** Description      This callout function is executed by FM when it is
**                  started by calling BTA_FmSetRDSMode().  This function can be
**                  used by the phone to initialize RDS decoder.
**
**
** Returns
**
*******************************************************************************/
tBTA_FM_STATUS bta_fm_co_init(tBTA_FM_RDS_B rds_mode)
{
    APPL_TRACE_DEBUG0("bta_fm_init_co");

    BTA_FmRDSInitDecoder();

    BTA_FmRDSRegister(rds_mode, 0xffffffff,
            /*BTA_FM_RDS_PI_BIT|BTA_FM_RDS_AF_BIT|BTA_FM_RDS_PS_BIT|BTA_FM_RDS_RT_BIT,*/
                btapp_fm_rdsp_cback, BTUI_RDS_APP_ID);

    BTA_FmRDSResetDecoder(BTUI_RDS_APP_ID);

    return BTA_FM_OK;
}

/*******************************************************************************
**
** Function         bta_fm_co_close
**
** Description      This callout function is executed by FM when it is
**                  started by calling BTA_FmSetRDSMode() to turn off RDS mode.
**                  This function can be used by the phone to reset RDS decoder.
**
** Returns
**
*******************************************************************************/
void bta_fm_co_close(void)
{
    BTA_FmRDSResetDecoder(BTUI_RDS_APP_ID);

}

/*******************************************************************************
**
** Function         bta_fm_co_reset_rds_engine
**
** Description      This function can be used by the phone to reset RDS decoder.
**                  after the TUNE/SEARCH operation
**
** Returns
**
*******************************************************************************/
void bta_fm_co_reset_rds_engine(void)
{

    BTA_FmRDSResetDecoder(BTUI_RDS_APP_ID);
}

/*******************************************************************************
**
** Function         bta_fm_co_rds_data/
**
** Description      This function is called by FM when RDS data is ready.
**
**
**
** Returns          void
**
*******************************************************************************/
tBTA_FM_STATUS bta_fm_co_rds_data(UINT8 * p_data, UINT16 len)
{

#define MAX_RDS_FRAME  24


    // Split the RDS buffer into n GKI_buffer of MAX_RDS_FRAME (24) optimal case
    if (len)
    {
        UINT16 l = len;
        UINT16 i = len / MAX_RDS_FRAME;
        UINT16 j;


        for (j = 0; j < i; j++)
        {
            tBTUI_FM_RDS_EVT *p_event_msg;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_PARSE_RDS_TASK_EVT;
              p_event_msg->hdr.offset = 0;
              memset(p_event_msg->parser.data,0, sizeof(tBTUI_FM_RDS_EVT));
              memcpy(p_event_msg->parser.data,p_data+(j*MAX_RDS_FRAME), MAX_RDS_FRAME);
              p_event_msg->parser.len = MAX_RDS_FRAME;
              len -= MAX_RDS_FRAME;

              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
        if (len)
        {
            tBTUI_FM_RDS_EVT *p_event_msg;

            if ((p_event_msg = (tBTUI_FM_RDS_EVT *)GKI_getbuf(sizeof(tBTUI_FM_RDS_EVT))) != NULL)
            {
              p_event_msg->hdr.event            = BTUI_FM_EVENT;
              p_event_msg->hdr.len              = sizeof(tBTUI_FM_RDS_EVT) - BT_HDR_SIZE;
              p_event_msg->hdr.layer_specific   = BTUI_FM_PARSE_RDS_TASK_EVT;
              p_event_msg->hdr.offset = 0;
              memcpy(p_event_msg->parser.data,p_data+(j*MAX_RDS_FRAME), len);
              p_event_msg->parser.len = len;

              GKI_send_msg(BTE_APPL_TASK, TASK_MBOX_0, (void *) p_event_msg);

            }
        }
        LOGI("bta_fm_co_rds_data - Send RDS data [%d bytes]",l);
    }

    return BTA_RDS_OK;

//    return BTA_FmRDSDecode(BTUI_RDS_APP_ID, p_data, len);
}

