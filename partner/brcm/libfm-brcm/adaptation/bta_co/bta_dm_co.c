/*****************************************************************************
**
**  Name:           bta_dm_co.c
**
**  Description:    This file contains the device manager callout function
**                  implementation for Insight.
**
**  Copyright (c) 2006, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_dm_co.h"
#include "bta_dm_ci.h"

/* during first phase of porting,
 * this can be set to FALSE to reduce the number of files/functions included from platform dependent code */
#ifndef BTA_DM_CO_INCLUDE_BTUI
#define BTA_DM_CO_INCLUDE_BTUI      TRUE
#endif

#if (BTM_SCO_HCI_INCLUDED == TRUE )
#include "btui_sco_codec.h"
#endif
#if (BTA_DM_CO_INCLUDE_BTUI == TRUE)
#include "btui.h"
#include "btui_int.h"
#include "btapp_dm.h"
#endif

/*******************************************************************************
**
** Function         bta_dm_co_get_compress_memory
**
** Description      This callout function is executed by DM to get memory for compression

** Parameters       id  -  BTA SYS ID
**                  memory_p - memory return by callout
**                  memory_size - memory size
**
** Returns          TRUE for success, FALSE for fail.
**
*******************************************************************************/
BOOLEAN bta_dm_co_get_compress_memory(tBTA_SYS_ID id, UINT8 **memory_p, UINT32 *memory_size)
{
     BOOLEAN status = FALSE;
     /* fix memory side for memory level 2, window bits 3 for compress expected 24568 bytes*/
     static UINT8    ftc_compress_memory[26000];
     static UINT8    fts_compress_memory[26000];
     static UINT8    opc_compress_memory[26000];
     static UINT8    ops_compress_memory[26000];

     APPL_TRACE_DEBUG1("bta_dm_co_get_compress_memory %d", id);

     if (id == BTA_ID_FTC) {
         *memory_p = ftc_compress_memory;
         *memory_size = sizeof(ftc_compress_memory);
         status = TRUE;
     }

     if (id == BTA_ID_FTS) {
         *memory_p = fts_compress_memory;
         *memory_size = sizeof(fts_compress_memory);
         status = TRUE;
     }

     if (id == BTA_ID_OPC) {
         *memory_p = opc_compress_memory;
         *memory_size = sizeof(opc_compress_memory);
         status = TRUE;
     }

     if (id == BTA_ID_OPS) {
         *memory_p = ops_compress_memory;
         *memory_size = sizeof(ops_compress_memory);
         status = TRUE;
     }

     return (status);

}

/*******************************************************************************
**
** Function         bta_dm_co_io_req
**
** Description      This callout function is executed by DM to get IO capabilities
**                  of the local device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  *p_io_cap - The local Input/Output capabilities
**                  *p_oob_data - TRUE, if OOB data is available for the peer device.
**                  *p_auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_req(BD_ADDR bd_addr, tBTA_IO_CAP *p_io_cap, tBTA_OOB_DATA *p_oob_data,
                                     tBTA_AUTH_REQ *p_auth_req, BOOLEAN is_orig)
{
    /* if OOB is not supported, this call-out function does not need to do anything
     * otherwise, look for the OOB data associated with the address and set *p_oob_data accordingly
     * If the answer can not be obtained right away,
     * set *p_oob_data to BTA_OOB_UNKNOWN and call bta_dm_ci_io_req() when the answer is available */

    /* *p_auth_req by default is FALSE for devices with NoInputNoOutput; TRUE for other devices. */

#if (BTA_DM_CO_INCLUDE_BTUI == TRUE)
    btapp_dm_proc_io_req(bd_addr, p_io_cap, p_oob_data, p_auth_req, is_orig);
#endif
}

/*******************************************************************************
**
** Function         bta_dm_co_io_rsp
**
** Description      This callout function is executed by DM to report IO capabilities
**                  of the peer device for the Simple Pairing process
**
** Parameters       bd_addr  - The peer device
**                  io_cap - The remote Input/Output capabilities
**                  oob_data - TRUE, if OOB data is available for the peer device.
**                  auth_req - TRUE, if MITM protection is required.
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_io_rsp(BD_ADDR bd_addr, tBTA_IO_CAP io_cap,
                      tBTA_OOB_DATA oob_data, tBTA_AUTH_REQ auth_req)
{
#if (BTA_DM_CO_INCLUDE_BTUI == TRUE)
    btapp_dm_proc_io_rsp(bd_addr, io_cap, auth_req);
#endif
}

/*******************************************************************************
**
** Function         bta_dm_co_lk_upgrade
**
** Description      This callout function is executed by DM to check if the
**                  platform wants allow link key upgrade
**
** Parameters       bd_addr  - The peer device
**                  *p_upgrade - TRUE, if link key upgrade is desired.
**
** Returns          void.
**
*******************************************************************************/
void  bta_dm_co_lk_upgrade(BD_ADDR bd_addr, BOOLEAN *p_upgrade )
{
#if (BTA_DM_CO_INCLUDE_BTUI == TRUE)
    btapp_dm_proc_lk_upgrade(bd_addr, p_upgrade);
#endif
}

#if (BTM_OOB_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         bta_dm_co_loc_oob
**
** Description      This callout function is executed by DM to report the OOB
**                  data of the local device for the Simple Pairing process
**
** Parameters       valid - TRUE, if the local OOB data is retrieved from LM
**                  c     - Simple Pairing Hash C
**                  r     - Simple Pairing Randomnizer R
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r)
{
#if (BTA_DM_CO_INCLUDE_BTUI == TRUE)
    extern void btui_dm_proc_loc_oob(BOOLEAN valid, BT_OCTET16 c, BT_OCTET16 r);
    /* process the local oob data. */
    btui_dm_proc_loc_oob(valid, c, r);
#endif
}

/*******************************************************************************
**
** Function         bta_dm_co_rmt_oob
**
** Description      This callout function is executed by DM to request the OOB
**                  data for the remote device for the Simple Pairing process
**                  Need to call bta_dm_ci_rmt_oob() in response
**
** Parameters       bd_addr  - The peer device
**
** Returns          void.
**
*******************************************************************************/
void bta_dm_co_rmt_oob(BD_ADDR bd_addr)
{
}
#endif /* BTM_OOB_INCLUDED */

#if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         btui_sco_codec_callback
**
** Description      Callback for btui codec.
**
**
** Returns          void
**
*******************************************************************************/
static void btui_sco_codec_callback(UINT16 event, UINT16 sco_handle)
{
    bta_dm_sco_ci_data_ready(event, sco_handle);
}
/*******************************************************************************
**
** Function         bta_dm_sco_co_init
**
** Description      This function can be used by the phone to initialize audio
**                  codec or for other initialization purposes before SCO connection
**                  is opened.
**
**
** Returns          tBTA_DM_SCO_ROUTE_TYPE: SCO routing configuration type.
**
*******************************************************************************/
tBTA_DM_SCO_ROUTE_TYPE bta_dm_sco_co_init(UINT32 rx_bw, UINT32 tx_bw,
                                    tBTA_CODEC_INFO * p_codec_type, UINT8 app_id)
{
    tBTM_SCO_ROUTE_TYPE route = BTA_DM_SCO_ROUTE_PCM;

    APPL_TRACE_DEBUG0("bta_dm_sco_co_init");

    /* set up SCO routing configuration if SCO over HCI app ID is used and run time
		configuration is set to SCO over HCI */
    /* HS invoke this call-out */
    if (
#if (BTA_HS_INCLUDED == TRUE ) && (BTA_HS_INCLUDED == TRUE)
        (app_id == BTUI_DM_SCO_4_HS_APP_ID && btui_cfg.hs_sco_over_hci) ||
#endif
        /* AG invoke this call-out */
        (app_id != BTUI_DM_SCO_4_HS_APP_ID && btui_cfg.ag_sco_over_hci ))
    {
        route = btui_cb.sco_hci = BTA_DM_SCO_ROUTE_HCI;
    }
    /* no codec is is used for the SCO data */
    if (p_codec_type->codec_type == BTA_SCO_CODEC_PCM && route == BTA_DM_SCO_ROUTE_HCI)
    {
        /* initialize SCO codec */
        if (!btui_sco_codec_init(rx_bw, tx_bw))
        {
            APPL_TRACE_ERROR0("codec initialization exception!");
        }
    }

    return route;
}



/*******************************************************************************
**
** Function         bta_dm_sco_co_open
**
** Description      This function is executed when a SCO connection is open.
**
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_open(UINT16 handle, UINT8 pkt_size, UINT16 event)
{
    tBTUI_SCO_CODEC_CFG cfg;

    if (btui_cb.sco_hci)
    {
        APPL_TRACE_DEBUG2("bta_dm_sco_co_open handle:%d pkt_size:%d", handle, pkt_size);
        /* use dedicated SCO buffer pool for SCO TX data */
        cfg.pool_id = HCI_SCO_POOL_ID;
        cfg.p_cback = btui_sco_codec_callback;
        cfg.pkt_size = pkt_size;
        cfg.cb_event = event;
        /* open and start the codec */
        btui_sco_codec_open(&cfg);
        btui_sco_codec_start(handle);
    }
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_close
**
** Description      This function is called when a SCO connection is closed
**
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_close(void)
{
    if (btui_cb.sco_hci)
    {
        APPL_TRACE_DEBUG0("bta_dm_sco_co_close close codec");
        /* close sco codec */
        btui_sco_codec_close();

        btui_cb.sco_hci = FALSE;
    }
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_in_data
**
** Description      This function is called to send incoming SCO data to application.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_in_data(BT_HDR  *p_buf)
{
    if (btui_cfg.sco_use_mic)
        btui_sco_codec_inqdata (p_buf);
    else
        GKI_freebuf(p_buf);
}

/*******************************************************************************
**
** Function         bta_dm_sco_co_out_data
**
** Description      This function is called to send SCO data over HCI.
**
** Returns          void
**
*******************************************************************************/
void bta_dm_sco_co_out_data(BT_HDR  **p_buf)
{
    btui_sco_codec_readbuf(p_buf);
}

#endif /* #if (BTM_SCO_HCI_INCLUDED == TRUE ) && (BTM_SCO_INCLUDED == TRUE)*/
