/*****************************************************************************
**
**  Name:           bta_ag_co.c
**
**  Description:    This file contains the audio gateway callout function
**                  implementation for Insight.
**
**  Copyright (c) 2003-2006, Broadcom Corp., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_ag_api.h"
#include "bta_ag_co.h"
#include "btl_ifs.h"
#include "bte_appl.h"

#define LOG_TAG "BTA_AG_CO: "

#ifndef LINUX_NATIVE
#include <cutils/properties.h>
#include <cutils/log.h>
#else
#include <stdio.h>
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGD(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format"\n", ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format"\n", ## __VA_ARGS__)
#endif


/* TODO: move to header file */
void btapp_ag_on_tx_data(UINT16 ag_handle, UINT8 * p_data, UINT16 len);


/*******************************************************************************
**
** Function         bta_ag_co_init
**
** Description      This callout function is executed by AG when it is
**                  started by calling BTA_AgEnable().  This function can be
**                  used by the phone to initialize audio paths or for other
**                  initialization purposes.
**
**
** Returns          Void.
**
*******************************************************************************/
void bta_ag_co_init(void)
{
    tBTM_STATUS rc;

    LOGI("bta_ag_co_init: ag_voice_settings = %d", bte_appl_cfg.ag_voice_settings);

    BTM_WriteVoiceSettings(bte_appl_cfg.ag_voice_settings);

    /* For SCO and PCM interface parameters :
     * Byte0 -- 0 for PCM, 1 for transport, 2 for codec, 3 for I2S
     * Byte1 -- 0 for 128 KHz, 1 for 256, 2 for 512, 3 for 1024, 4 for 2048
     * Byte2 -- 0 for short frame sync, 1 for long
     * Byte3 -- 0 for slave role, 1 for master role for SYNC pin
     * Byte4 -- 0 for slave role, 1 for master role for CLK pin
     */
    LOGI("bta_ag_co_init: ag_vsc_sco_pcm(%p) = {%d, %d, %d, %d, %d}",
         bte_appl_cfg.ag_vsc_sco_pcm,
         bte_appl_cfg.ag_vsc_sco_pcm[0], bte_appl_cfg.ag_vsc_sco_pcm[1],
         bte_appl_cfg.ag_vsc_sco_pcm[2], bte_appl_cfg.ag_vsc_sco_pcm[3],
         bte_appl_cfg.ag_vsc_sco_pcm[4]);

    rc = BTM_VendorSpecificCommand(0x001C, 5, bte_appl_cfg.ag_vsc_sco_pcm, NULL);
    if (rc != BTM_SUCCESS)
    {
	    LOGE("bta_ag_co_init: Failed to set SCO PCM interface, rc = %d", rc);
        LOGE("    ag_vsc_sco_pcm = {%d, %d, %d, %d, %d}",
             bte_appl_cfg.ag_vsc_sco_pcm[0], bte_appl_cfg.ag_vsc_sco_pcm[1],
             bte_appl_cfg.ag_vsc_sco_pcm[2], bte_appl_cfg.ag_vsc_sco_pcm[3],
             bte_appl_cfg.ag_vsc_sco_pcm[4]);
    }

    /* Set PCM interface configuration. The PCM parameters format is as following:
     * Byte0 LSB_First        -- 0 for MSb first
     * Byte1 Fill value(0-1)  -- Specified the value with which to fill unused bits if
     *                           Fill option is set to programmable.
     *                           Use 0x0 for programmed fill bits.
     * Byte2 Fill option(0-3) -- Specifies the method of filling unused data bits values
     *                           0x0:0's, 0x1:1's, 0x2:signed, 0x3:programmable
     * Byte3 Fill bits(0-3)   -- Fill all 3 bits.
     * Byte4 Right justify    -- Use left justify(fill data shifted lout last)
     */
    LOGI("bta_ag_co_init: ag_vsc_pcm_config(%p) = {%d, %d, %d, %d, %d}",
         bte_appl_cfg.ag_vsc_pcm_config,
         bte_appl_cfg.ag_vsc_pcm_config[0], bte_appl_cfg.ag_vsc_pcm_config[1],
         bte_appl_cfg.ag_vsc_pcm_config[2], bte_appl_cfg.ag_vsc_pcm_config[3],
         bte_appl_cfg.ag_vsc_pcm_config[4]);

    rc = BTM_VendorSpecificCommand(0x001E, 5, bte_appl_cfg.ag_vsc_pcm_config, NULL);
    if (rc != BTM_SUCCESS)
    {
	    LOGE("bta_ag_co_init: Failed to set PCM interface configuration, rc = %d", rc);
        LOGE("    ag_vsc_pcm_config = {%d, %d, %d, %d, %d}",
             bte_appl_cfg.ag_vsc_pcm_config[0], bte_appl_cfg.ag_vsc_pcm_config[1],
             bte_appl_cfg.ag_vsc_pcm_config[2], bte_appl_cfg.ag_vsc_pcm_config[3],
             bte_appl_cfg.ag_vsc_pcm_config[4]);
    }
}


/*******************************************************************************
**
** Function         bta_ag_co_audio_state
**
** Description      This function is called by the AG before the audio connection
**                  is brought up, after it comes up, and after it goes down.
**
** Parameters       handle - handle of the AG instance
**                  state - Audio state
**                      BTA_AG_CO_AUD_STATE_OFF     - Audio has been turned off
**                      BTA_AG_CO_AUD_STATE_OFF_XFER - Audio has been turned off (xfer)
**                      BTA_AG_CO_AUD_STATE_ON      - Audio has been turned on
**                      BTA_AG_CO_AUD_STATE_SETUP   - Audio is about to be turned on
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_audio_state(UINT16 handle, UINT8 app_id, UINT8 state)
{
    if (state == BTA_AG_CO_AUD_STATE_OFF_XFER)
    {
        APPL_TRACE_DEBUG1("bta_ag_co_audio_state: handle %d, Closed (XFERRING)", handle);
    }
    else
    {
        APPL_TRACE_DEBUG2("bta_ag_co_audio_state: handle %d, state %d", handle, state);
    }
}


/*******************************************************************************
**
** Function         bta_ag_co_data_open
**
** Description      This function is executed by AG when a service level connection
**                  is opened.  The phone can use this function to set
**                  up data paths or perform any required initialization or
**                  set up particular to the connected service.
**
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_open(UINT16 handle, tBTA_SERVICE_ID service)
{

    APPL_TRACE_DEBUG2("bta_ag_co_data_open handle:%d service:%d", handle, service);

}

/*******************************************************************************
**
** Function         bta_ag_co_data_close
**
** Description      This function is called by AG when a service level
**                  connection is closed
**
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_data_close(UINT16 handle)
{
    APPL_TRACE_DEBUG1("bta_ag_co_data_close handle:%d", handle);


}



/*******************************************************************************
**
** Function         bta_ag_co_tx_write
**
** Description      This function is called by the AG to send data to the
**                  phone when the AG is configured for AT command pass-through.
**                  The implementation of this function must copy the data to
**                  the phone’s memory.
**
** Returns          void
**
*******************************************************************************/
void bta_ag_co_tx_write(UINT16 handle, UINT8 * p_data, UINT16 len)
{
    UINT32 skt_conn_id;

    btapp_ag_on_tx_data(handle, p_data, len);
}

