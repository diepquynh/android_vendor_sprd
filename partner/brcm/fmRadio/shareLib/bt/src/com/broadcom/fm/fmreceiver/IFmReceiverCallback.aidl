/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/

package com.broadcom.fm.fmreceiver;

/**
 * Callback interface for applications to receive events from the FM radio service.
 * @hide
 */
interface IFmReceiverCallback{
    /* The FM Receiver callback function list. */
    /**
     * Callback for a radio status event.
     * @param freq the radio frequency
     * @param rssi the received signal strength indicator
     * @param snr the received signal-to-noise ratio(SNR) value
     * @param radioIsOn true if the radio is turned on
     * @param rdsProgramType the RDS program type
     * @param rdsProgramService the RDS program service
     * @param rdsProgramText the RDS radio text
     * @param rdsProgramTypeName the RDS program type name
     * @param isMute if true, indicates the radio is muted
     */
    void onStatusEvent(int freq, int rssi, int snr, boolean radioIsOn,
        int rdsProgramType, in String rdsProgramService, in String rdsRadioText,
        in String rdsProgramTypeName, boolean isMute);

    /**
     * Callback for a radio seek complete event.
     * @param freq the radio frequency
     * @param rssi the received signal strength indicator
     * @param snr the received signal-to-noise ratio(SNR) value
     * @param seeksuccess if true, indicates the radio seek succeeded
     */
    void onSeekCompleteEvent(int freq, int rssi, int snr, boolean seeksuccess);

    /**
     * Callback for a RDS mode event
     * @param rdsMode the RDS Mode
     * @param alternateFreqHopEnabled if true, alternative frequency hop is enabled
     */
    void onRdsModeEvent(int rdsMode, int alternateFreqHopEnabled);

    /**
     * Callback for a RDS data event
     * @param rdsMode the RDS Mode
     * @param alternateFreqHopEnabled if true, alternative frequency hop is enabled
     */
    void onRdsDataEvent(int rdsDataType, int rdsIndex, in String rdsText);

    /**
     * Callback for an audio mode event
     * @param audioMode the audio mode
     */
    void onAudioModeEvent(int audioMode);

    /* Handle audio path event. */
    void onAudioPathEvent(int audioPath);

    /**
     * Callback for world region mode event
     * @param worldRegion the world region
     */
    void onWorldRegionEvent(int worldRegion);

    /**
     * Callback for estimate noise floor event
     * @param nfl the noise floor
     */
    void onEstimateNflEvent(int nfl);

    /**
     * Callback for live audio quality event
     * @param rssi the received signal strength indicator
     * @param snr the received signal-to-noise ratio(SNR) value
     */
    void onLiveAudioQualityEvent(int rssi, int snr);

    /**
     * Callback for FM volume event
     * @param status equal to 0 if successful. Otherwise returns a non-zero error code.
     * @param volume range from 0 to 0x100
     */
    void onVolumeEvent(int status,int volume);
}
