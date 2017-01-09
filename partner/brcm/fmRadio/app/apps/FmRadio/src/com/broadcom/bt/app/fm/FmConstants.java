/** Copyright 2009-2013 Broadcom Corporation
 **
 ** This program is the proprietary software of Broadcom Corporation and/or its
 ** licensors, and may only be used, duplicated, modified or distributed
 ** pursuant to the terms and conditions of a separate, written license
 ** agreement executed between you and Broadcom (an "Authorized License").
 ** Except as set forth in an Authorized License, Broadcom grants no license
 ** (express or implied), right to use, or waiver of any kind with respect to
 ** the Software, and Broadcom expressly reserves all rights in and to the
 ** Software and all intellectual property rights therein.
 ** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 ** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 ** ALL USE OF THE SOFTWARE.
 **
 ** Except as expressly set forth in the Authorized License,
 **
 ** 1.     This program, including its structure, sequence and organization,
 **        constitutes the valuable trade secrets of Broadcom, and you shall
 **        use all reasonable efforts to protect the confidentiality thereof,
 **        and to use this information only in connection with your use of
 **        Broadcom integrated circuit products.
 **
 ** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 **        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 **        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 **        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 **        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 **        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 **        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 **        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 **        OF USE OR PERFORMANCE OF THE SOFTWARE.
 **
 ** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 **        ITS LICENSORS BE LIABLE FOR
 **        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 **              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 **              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 **              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 **        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 **              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 **              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 **              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 */

package com.broadcom.bt.app.fm;

public class FmConstants {

    public static final String PREF_LAST_TUNED_FREQUENCY = "pref_last_tuned_frequency";

    public static final String ACTION_STATE_CHANGED = "android.fm.transmit.action.STATE_CHANGED";
    public static final String EXTRA_STATE = "android.fm.transmit.extra.STATE";

    public static final int STATE_FM_TRANSIT_ON = 10;
    public static final int STATE_FM_TRANSIT_OFF = 11;
    public static final int STATE_FM_TRANSIT_TURNING_ON = 12;
    public static final int STATE_FM_TRANSIT_TURNING_OFF = 13;
    public static final int STATE_FM_TRANSIT_ERROR = -123456;

    /* Button elements. public to allow identification on events. */
    public final static int BUTTON_POWER_OFF = 5;
    public final static int BUTTON_PRESETS = 6;
    public final static int BUTTON_MUTE = 7;
    public final static int BUTTON_MUTE_ON = 8;
    public final static int BUTTON_MUTE_OFF = 9;
    public final static int BUTTON_TUNE_UP = 10;
    public final static int BUTTON_TUNE_DOWN = 11;
    public final static int BUTTON_SEEK_UP = 12;
    public final static int BUTTON_SEEK_DOWN = 13;
    public final static int BUTTON_SWITCH_FM_RX = 14;
    public final static int BUTTON_SETTINGS = 15;

    public final static int BUTTON_CH_1 = 21;
    public final static int BUTTON_CH_2 = 22;
    public final static int BUTTON_CH_3 = 23;
    public final static int BUTTON_CH_4 = 24;
    public final static int BUTTON_CH_5 = 25;
    public final static int BUTTON_CH_6 = 26;
    public final static int BUTTON_CH_7 = 27;
    public final static int BUTTON_CH_8 = 28;
    public final static int BUTTON_CH_9 = 29;
    public final static int BUTTON_CH_10 = 30;

    public final static int SCAN_STEP_50KHZ = 5;
    public final static int SCAN_STEP_100KHZ = 10;
    public final static int SCAN_STEP_200KHZ = 20;

    /* Mute status constants. */
    public final static int MUTE_STATE_UNMUTED = 1;
    public final static int MUTE_STATE_MUTED = 0;

    public final static int MAX_FREQUENCY_JAPAN = 9000;
    public final static int MIN_FREQUENCY_JAPAN = 7600;
    public final static int MAX_FREQUENCY_US_EUROPE = 10800;
    public final static int MIN_FREQUENCY_US_EUROPE = 8750;
    public final static int MAX_FREQUENCY_JAPAN_II = 10800;
    public final static int MIN_FREQUENCY_JAPAN_II = 9000;

    public final static int FM_FREQUENCY_TUNE_UP = 0;
    public final static int FM_FREQUENCY_TUNE_DOWN = 1;

    /* Button event types. */
    public final static int BUTTON_EVENT_NONE = 0;
    public final static int BUTTON_EVENT_UP = 1;
    public final static int BUTTON_EVENT_DOWN = 2;
    public final static int BUTTON_EVENT_HELD = 3;
    /* Default frequency. */
    private static final int DEFAULT_FREQUENCY = 10560;

    /* Min frequency = 1.00 MHz. */
    public final static int FREQ_VALUE_MIN = 100;
    /* Min frequency = 199.95 MHz. */
    public final static int FREQ_VALUE_MAX = 19995;

    public final static int SIGNAL_STRENGTH_NONE = 0;
    public final static int SIGNAL_STRENGTH_LOW = 1;
    public final static int SIGNAL_STRENGTH_MEDIUM = 2;
    public final static int SIGNAL_STRENGTH_HIGH = 3;

    /* RDS status constants. */
    public final static int RDS_STATE_RDS_OFF = 0;
    public final static int RDS_STATE_RDS_ON = 1;
    public final static int RDS_STATE_RBDS_ON = 2;

    /* AF status constants. */
    public final static int AF_STATE_OFF = 0;
    public final static int AF_STATE_ON = 1;

    /* RDS type identifier list. */
    public static final int RDS_ID_PTY_EVT = 2;
    public static final int RDS_ID_PS_EVT = 7;
    public static final int RDS_ID_PTYN_EVT = 8;
    public static final int RDS_ID_RT_EVT = 9;

    /* SOFTMUTE status constants. */
    public final static int SOFTMUTE_STATE_OFF = 0;
    public final static int SOFTMUTE_STATE_ON = 1;

    public final static boolean FM_SOFTMUTE_FEATURE_ENABLED = false;

    /* COMBO_SEARCH status constants. */
    public static final boolean FM_COMBO_SEARCH_ENABLED = true;
    public static final boolean COMBO_SEARCH_MULTI_CHANNEL_DEFAULT = false;

    /* List of Program Type(s) displayed. */
    public static String[] PTY_LIST = {
                    "", "News", "Current Affairs", "Information", "Sport", "Education", "Drama",
                    "Cultures", "Science", "Varied Speech", "Pop Music", "Rock Music",
                    "Easy Listening", "Light Classics M", "Serious Classics", "Other Music",
                    "Weather & Metr", "Finance", "Children's Progs", "Social Affairs", "Religion",
                    "Phone In", "Travel & Touring", "Leisure & Hobby", "Jazz Music",
                    "Country Music", "National Music", "Oldies Music", "Folk Music", "Documentary",
                    "Alarm Test", "Alarm - Alarm !"
    };
}
