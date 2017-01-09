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
 * Applications wishing use the FmProxy must implement the 
 * functions described by the
 * IFmReceiverEventHandler interface to receive asynchronous
 * information and status updates from the FM Receiver.
 * {@hide}
 */
public interface IFmReceiverEventHandler {

    /**
     * Is called when the FM Receiver has finished processing a successful
     * turnOnradio(), turnOffRadio(), tuneRadio(), getStatus() request.
     * This is a useful function to call to update on the basic information
     * required to update a GUI.
     * @param freq indicates the currently tuned frequency. 
     * @param rssi indicates the RSSI of the currently tuned frequency.
     * @param snr indicates the SNR value of the currently tuned frequency.
     * @param radioIsOn is true if the radio is on and false if not.
     * @param rdsProgramType indicates the type of program currently playing 
     *        if that information is available on the selected frequency and 
     *        PTY is enabled.
     * @param rdsProgramService indicates the name of the currently selected
     *        station if that information is available.
     * @param rdsRadioText indicates the radio text from the currently selected 
     *        station if available.
     * @param rdsProgramTypeName indicates the string version of the program 
     *        currently playing if that information is available on the 
     *        selected frequency.
     * @param isMute is true if the audio is muted at hardware level.
     */
    public void onStatusEvent(int freq, int rssi, int snr, boolean radioIsOn,
            int rdsProgramType, String rdsProgramService, String rdsRadioText,
            String rdsProgramTypeName, boolean isMute);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * seekStation(), seekSpecificStation() or seekStationAbort() request.
     * 
     * @param freq indicates the frequency of the located station if found. 
     * @param rssi indicates the RSSI of the located station if found.
     * @param snr indicates the SNR value of the located station if found.
     * @param seeksuccess is true if a valid station was located.
     */
    public void onSeekCompleteEvent(int freq, int rssi, int snr, boolean seeksuccess);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * setRdsMode() request.
     * 
     * @param rdsMode indicates the current RDS or RDBS mode.
     * @param alternateFreqHopEnabled indicates whether alternate frequency 
     *        hopping is enabled.
     */
    public void onRdsModeEvent(int rdsMode, int alternateFreqHopEnabled);

    /**
     * Is called when the FM Receiver has compiled an RDS message for the
     * application. Only those RDS message types requested when setting 
     * the RDS mode will be relayed.
     * 
     * @param rdsDataType indicates the type of RDS data.
     * @param rdsIndex indicates the integer value of the data if available.
     * @param rdsText indicates the string value of the data if available.
     */
    public void onRdsDataEvent(int rdsDataType, int rdsIndex, String rdsText);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * setAudioMode() request.
     * 
     * @param audioMode indicates the current audio mode.
     */
    public void onAudioModeEvent(int audioMode);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * setAudioPath() request.
     * 
     * @param audioPath indicates the current audio path.
     */
    public void onAudioPathEvent(int audioPath);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * setFrequencyParameters() request.
     * 
     * @param worldRegion indicates the world frequency band region in use.
     */
    public void onWorldRegionEvent(int worldRegion);

    /**  
     * Is called when the FM Receiver has finished processing a successful
     * estimateNoiseFloorLevel() request. The returned nfl parameter can be
     * used as a requested minimum signal strength value when seeking stations.
     * 
     * @param nfl indicates the current frequencies Noise Floor Level.
     */ 
    public void onEstimateNoiseFloorLevelEvent(int nfl);

    /**
     *  Is called repeatedly when the FM Receiver is actively sampling the signal
     *  quality. Provides a live stream of signal quality data to the application.
     * 
     * @param rssi the signal quality at the time of the latest audio sampling.
     * @param snr the SNR(Signal Noise Reading) reading at the time of the
     */
    public void onLiveAudioQualityEvent(int rssi, int snr);

    /**
     * Is called when the FM Receiver has finished processing a successful
     * setFMVolume() request.
         * @param status equal to 0 if successful. Otherwise returns a non-zero error code.
         * @param volume range from 0 to 0x100
     */
         public void onVolumeEvent(int status,int volume);
        
}
