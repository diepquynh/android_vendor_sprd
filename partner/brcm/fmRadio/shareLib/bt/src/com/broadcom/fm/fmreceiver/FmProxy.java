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

import com.broadcom.fm.fmreceiver.IFmReceiverEventHandler;
import com.broadcom.fm.fmreceiver.IFmProxyCallback;
import com.broadcom.fm.fmreceiver.FmReceiverServiceConfig;


import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.content.Context;
import com.broadcom.fm.fmreceiver.IFmReceiverCallback;
import com.broadcom.fm.fmreceiver.IFmReceiverService;
import android.bluetooth.BluetoothAdapter;


import java.lang.reflect.Method;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Looper;
import android.os.Process;

/**
 * FmProxy is the Java API entry point to issue commands to FM receiver
 * hardware. After a command is issued one or more FmReceiverEvents will be
 * issued to the requested client application handler. The application must
 * implement the {@link IFmReceiverEventHandler} interface to receive the
 * results of requested operations.
 * <p>
 * PUBLIC FM PROXY API
 * <p>
 * An FmProxy object acts as a proxy to the FmService/FmTransmitterService etc
 * <p>
 * Usage:
 * <p>
 * First create a reference to the FM Proxy system service:
 * <p>
 * <code> FmProxy mFmPRoxu = (FmProxy) FmProxy.getProxy(); </code>
 * <p>
 * Then register as an event handler:
 * <p>
 * <code> mFmReceiver.registerEventHandler(this); </code>
 * <p>
 * The application should then call the turnOnRadio() function and wait for a
 * confirmation status event before calling further functions.
 * <p>
 * On closing the high level application, turnOffRadio() should be called to
 * disconnect from the FmService. A confirmation status event should be
 * received before the high level application is terminated.
 * <p>
 * This class first acquires an interface to the FmService module.
 * This allows the FmProxy instance
 * to act as a proxy to the FmService through which all FM Proxy
 * related commands are relayed. The FmService answers the FmProxy
 * instance by issuing FmServiceEvents to the FmProxy instance. (In
 * practice using multiple synchronized callback functions.)
 * {@hide}
 */
public final class FmProxy {

    private static final String TAG = "FmProxy";


    public static final String FM_RECEIVER_PERM = "android.permission.ACCESS_FM_RECEIVER";

    /**
     * Event Actions
     */

    /**
     * @hide
     */
    private static final String ACTION_PREFIX = "com.broadcom.bt.app.fm.action.";

    private static final int ACTION_PREFIX_LENGTH = ACTION_PREFIX.length();

    /**
     * Broadcast Intent action for fm status event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RADIO_ON}</th>
     * <td>boolean</td>
     * <td>If true, the FM Receiver is on.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_MUTED}</th>
     * <td>boolean</td>
     * <td>If true, the FM Receiver is muted.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_FREQ}</th>
     * <td>int</td>
     * <td>FM Frequency</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RSSI}</th>
     * <td>int</td>
     * <td>Received signal strength indicator.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_PRGM_TYPE}</th>
     * <td>int</td>
     * <td>The RDS program type.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_PRGM_TYPE_NAME}</th>
     * <td>String</td>
     * <td>The RDS program name.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_PRGM_SVC}</th>
     * <td>String</td>
     * <td>The RDS program service.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_TXT}</th>
     * <td>String</td>
     * <td>The RDS program text.</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_STATUS = ACTION_PREFIX + "ON_STATUS";

    /**
     * Broadcast Intent action for fm seek complete event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <th>{@link #EXTRA_FREQ}</th>
     * <td>int</td>
     * <td>FM Frequency</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RSSI}</th>
     * <td>int</td>
     * <td>Received signal strength indicator.</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_SUCCESS}</th>
     * <td>boolean</td>
     * <td>If true, the seek was successful.</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_SEEK_CMPL = ACTION_PREFIX + "ON_SEEK_CMPL";

    /**
     * Broadcast Intent action for RDS modes event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_MODE}</th>
     * <td>int</td>
     * <td>The RDS mode</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_ALT_FREQ_MODE}</th>
     * <td>int</td>
     * <td>The alternative frequency mode.</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_RDS_MODE = ACTION_PREFIX + "ON_RDS_MODE";

    /**
     * Broadcast Intent action for fm rds data event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <th>{@link #EXTRA_RDS_IDX}</th>
     * <td>int</td>
     * <td>RDS index</td>
     * </tr>
     * <tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_DATA_TYPE}</th>
     * <td>int</td>
     * <td>RDS data type</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RDS_TXT}</th>
     * <td>int</td>
     * <td>RDS txt</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_RDS_DATA = ACTION_PREFIX + "ON_RDS_DATA";

    /**
     * Broadcast Intent action for fm audio mode data event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_AUDIO_MODE}</th>
     * <td>int</td>
     * <td>Audio mode</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_AUDIO_MODE = ACTION_PREFIX + "ON_AUDIO_MODE";

    /**
     * Broadcast Intent action for fm audio path data event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_AUDIO_PATH}</th>
     * <td>int</td>
     * <td>Audio mode</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_AUDIO_PATH = ACTION_PREFIX + "ON_AUDIO_PATH";

    /**
     * Broadcast Intent action for fm world region event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_WRLD_RGN}</th>
     * <td>int</td>
     * <td>World Region</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_WRLD_RGN = ACTION_PREFIX + "ON_WRLD_RGN";

    /**
     * Broadcast Intent action for fm estimate nfl event
     * 
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_NFL}</th>
     * <td>int</td>
     * <td>NFL</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_EST_NFL = ACTION_PREFIX + "ON_EST_NFL";

    /**
     * Broadcast Intent action for fm audio quality event
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_RSSI}</th>
     * <td>int</td>
     * <td>Rssi</td>
     * </tr>
     * </table>
     */
    public static final String ACTION_ON_AUDIO_QUAL = ACTION_PREFIX + "ON_AUDIO_QUAL";

    /**
     * Broadcast Intent action for fm volumeevent
     * This broadcast intent has the following return values
     * <table>
     * <tr>
     * <th>Extra Param Name</th>
     * <th>Data Type</th>
     * <th>Description</th>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_VOL}</th>
     * <td>int</td>
     * <td>Volume</td>
     * </tr>
     * <tr>
     * <th>{@link #EXTRA_STATUS}</th>
     * <td>int</td>
     * <td>status</td>
     * </tr>
     * </table>
     */

    public static final String ACTION_ON_VOL = "ON_VOL";

    public static final String EXTRA_FREQ = "FREQ";
    public static final String EXTRA_RSSI = "RSSI";
    public static final String EXTRA_RADIO_ON = "RADIO_ON";
    public static final String EXTRA_MUTED = "MUTED";
    public static final String EXTRA_RDS_MODE = "RDS_MODE";
    public static final String EXTRA_RDS_PRGM_TYPE = "RDS_PRGM_TYPE";
    public static final String EXTRA_RDS_PRGM_TYPE_NAME = "RDS_PRGM_TYPE_NAME";
    public static final String EXTRA_RDS_PRGM_SVC = "RDS_PRGM_SVC";
    public static final String EXTRA_RDS_TXT = "RDS_TXT";
    public static final String EXTRA_RDS_DATA_TYPE = "RDS_DATA_TYPE";
    public static final String EXTRA_RDS_IDX = "RDS_IDX";
    public static final String EXTRA_SUCCESS = "RDS_SUCCESS";
    public static final String EXTRA_ALT_FREQ_MODE = "ALT_FREQ_MODE";
    public static final String EXTRA_AUDIO_MODE = "AUDIO_MODE";
    public static final String EXTRA_AUDIO_PATH = "AUDIO_PATH";
    public static final String EXTRA_WRLD_RGN = "WRLD_RGN";
    public static final String EXTRA_NFL = "NFL";
    public static final String EXTRA_SNR = "SNR";

    public static final String EXTRA_STATUS = "STATUS";
    public static final String EXTRA_VOL = "VOL";

    /* FM functionality mask bit constants. */

    /** This mask sets the world region to North America. */
    public static final int FUNC_REGION_NA = 0x00; /*
                                                    * bit0/bit1/bit2: North
                                                    * America.
                                                    */
    /** This mask sets the world region to Europe. */
    public static final int FUNC_REGION_EUR = 0x01; /* bit0/bit1/bit2: Europe. */
    /** This mask sets the world region to Japan. */
    public static final int FUNC_REGION_JP = 0x02; /* bit0/bit1/bit2: Japan. */
    /** This mask sets the world region to Japan II (Upper region band). */
    public static final int FUNC_REGION_JP_II = 0x03; /* bit0/bit1/bit2: Japan. */

    /** This mask enables RDS. */
    public static final int FUNC_RDS = 1 << 4; /* bit4: RDS functionality */
    /** This mask enables RDBS. */
    public static final int FUNC_RBDS = 1 << 5; /*
                                                 * bit5: RDBS functionality,
                                                 * exclusive with RDS bit
                                                 */
    /** This mask enables the Alternate Frequency RDS feature. */
    public static final int FUNC_AF = 1 << 6; /* bit6: AF functionality */
    /** This mask enables SOFTMUTE. */
    public static final int FUNC_SOFTMUTE = 1 << 8; /* bit8: SOFTMUTE functionality */

    /* FM audio output mode. */
    /**
     * Allows the radio to automatically select between Mono and Stereo audio
     * output.
     */
    public static final int AUDIO_MODE_AUTO = 0; /* Auto blend by default. */
    /** Forces Stereo mode audio. */
    public static final int AUDIO_MODE_STEREO = 1; /* Manual stereo switch. */
    /** Forces Mono mode audio. */
    public static final int AUDIO_MODE_MONO = 2; /* Manual mono switch. */
    /** Allows Stereo mode audio with blend activation. */
    public static final int AUDIO_MODE_BLEND = 3; /* Deprecated. */
    // TODO: phase out previous line in favor of next line.
    public static final int AUDIO_MODE_SWITCH = 3; /* Switch activated. */
    /** No FM routing */
    public static final int AUDIO_PATH_NONE = 0; /* No FM routing */
    /** FM routing over speaker */
    public static final int AUDIO_PATH_SPEAKER = 1; /* FM routing speaker */
    /** FM routing over wired headset */
    public static final int AUDIO_PATH_WIRE_HEADSET = 2; /*
                                                          * FM routing over
                                                          * wired headset
                                                          */
    /** FM routing over I2S */
    public static final int AUDIO_PATH_DIGITAL = 3; /* FM routing over I2S */

    /* FM audio quality. */
    /**
     * The audio quality of reception.
     */
    /** Using Stereo mode audio quality. */
    public static final int AUDIO_QUALITY_STEREO = 1; /* Manual stereo switch. */
    /** Using Mono mode audio quality. */
    public static final int AUDIO_QUALITY_MONO = 2; /* Manual mono switch. */
    /** Using Blend mode audio quality. */
    public static final int AUDIO_QUALITY_BLEND = 4; /*
                                                      * Auto stereo, and switch
                                                      * activated.
                                                      */

    /* FM scan mode. */
    /** This sets default direction scanning when seeking stations. */
    public static final int SCAN_MODE_NORMAL = 0x00;
    public static final int SCAN_MODE_FAST = 0x01;

    /** This sets scanning to go downwards when seeking stations. */
    public static final int SCAN_MODE_DOWN = 0x00;

    /** This sets scanning to go upwards when seeking stations. */
    public static final int SCAN_MODE_UP = 0x80;

    /** This sets scanning to cover the whole bandwidth and return multiple hits. */
    public static final int SCAN_MODE_FULL = 0x82;

    /* Deemphasis time */
    /** This sets deemphasis to the European default. */
    public static final int DEEMPHASIS_50U = 0; /*
                                                 * 6th bit in FM_AUDIO_CTRL0 set
                                                 * to 0, Europe default
                                                 */
    /** This sets deemphasis to the US default. */
    public static final int DEEMPHASIS_75U = 1 << 6; /*
                                                      * 6th bit in
                                                      * FM_AUDIO_CTRL0 set to 1,
                                                      * US default
                                                      */

    /* Step type for searching */
    /** This sets the frequency interval to 100 KHz when seeking stations. */
    public static final int FREQ_STEP_100KHZ = 0x00;
    /** This sets the frequency interval to 50 KHz when seeking stations. */
    public static final int FREQ_STEP_50KHZ = 0x10;

    public static final int FM_VOLUME_MAX = 255;

    /* Noise floor level */
    /** This sets the Noise Floor Level to LOW. */
    public static final int NFL_LOW = 0;
    /** This sets the Noise Floor Level to MEDIUM. */
    public static final int NFL_MED = 1;
    /** This sets the Noise Floor Level to FINE. */
    public static final int NFL_FINE = 2;

    /* RDS RDBS type */
    /** This deactivates all RDS and RDBS functionality. */
    public static final int RDS_MODE_OFF = 0;
    /** This activates RDS or RDBS as appropriate. */
    public static final int RDS_MODE_DEFAULT_ON = 1;
    /** This activates RDS. */
    public static final int RDS_MODE_RDS_ON = 2;
    /** This activates RDBS. */
    public static final int RDS_MODE_RBDS_ON = 3;

    /* RDS condition type */
    /** Selects no PTY or TP functionality. */
    public static final int RDS_COND_NONE = 0;
    /** Activates RDS PTY capability. */
    public static final int RDS_COND_PTY = 1;
    /** Activates RDS TP capability. */
    public static final int RDS_COND_TP = 2;
    /* Check this again! RDS PTY (Protram types) code, 0 ~ 31, when the PTY is specified in mPendingRdsType. */
    public static final int RDS_COND_PTY_VAL = 0;

    /* RDS feature values. */
    /** Specifies the Program Service feature. */
    public static final int RDS_FEATURE_PS = 4;
    /** Specifies the Program Type feature. */
    public static final int RDS_FEATURE_PTY = 8;
    /** Specifies the Traffic Program feature. */
    public static final int RDS_FEATURE_TP = 16;
    /** Specifies the Program Type Name feature. */
    public static final int RDS_FEATURE_PTYN = 32;
    /** Specifies the Radio Text feature. */
    public static final int RDS_FEATURE_RT = 64;

    /* AF Modes. */
    /** Disables AF capability. */
    public static final int AF_MODE_OFF = 0;
    /** Enables AF capability. */
    public static final int AF_MODE_ON = 1;

    /* The default constants applied on system startup. */
    /**
     * Specifies default minimum signal strength that will be identified as a
     * station when scanning.
     * */
    public static final int MIN_SIGNAL_STRENGTH_DEFAULT = 105;
    /** Specifies default radio functionality. */
    public static final int FUNCTIONALITY_DEFAULT = FUNC_REGION_NA;
    /** Specifies default world frequency region. */
    public static final int FUNC_REGION_DEFAULT = FUNC_REGION_NA;
    /** Specifies default frequency scanning step to use. */
    public static final int FREQ_STEP_DEFAULT = FREQ_STEP_100KHZ;
    /** Specifies if live audio quality sampling is enabled by default. */
    public static final boolean LIVE_AUDIO_QUALITY_DEFAULT = false;
    /** Specifies the default estimated Noise Floor Level. */
    public static final int NFL_DEFAULT = NFL_MED;
    /** Specifies the default signal poll interval in ms. */
    public static final int SIGNAL_POLL_INTERVAL_DEFAULT = 100;
    /** Specifies the default signal poll interval in ms. */
    public static final int DEEMPHASIS_TIME_DEFAULT = DEEMPHASIS_75U;
    /** Default Alternate Frequency mode (DISABLED). */
    public static final int AF_MODE_DEFAULT = AF_MODE_OFF;

    /** Minimum allowed SNR Threshold */
    public static final int FM_MIN_SNR_THRESHOLD = 0;
    /** Maximum allowed SNR Threshold */
    public static final int FM_MAX_SNR_THRESHOLD = 31;

    /* Return status codes. */
    /** Function executed correctly. Parameters checked OK. */
    public static final int STATUS_OK = 0;
    /** General nonspecific error occurred. */
    public static final int STATUS_FAIL = 1;
    /** Server call resulted in exception. */
    public static final int STATUS_SERVER_FAIL = 2;
    /** Function could not be executed at this time. */
    public static final int STATUS_ILLEGAL_COMMAND = 3;
    /** Function parameters are out of allowed range. */
    public static final int STATUS_ILLEGAL_PARAMETERS = 4;

    /* Internal reference to client application event handler. */
    private IFmReceiverEventHandler mEventHandler = null;

    /* Generic remote service reference. */
    private IFmReceiverService mService;

    /** Callback handler **/
    private IFmReceiverCallback mCallback;

    /**
     * Get a proxy to the this service
     * @param cb
     * @return
     */

    public static boolean getProxy(Context ctx, IFmProxyCallback cb) {
        boolean status = false;
        FmProxy p = null;

        try {
            p = new FmProxy(ctx, cb);
        } catch (Throwable t) {
            Log.e(TAG, "Unable to get FM Proxy", t);
            return false;
        }


        return true;

    }

    public FmProxy(Context ctx, IFmProxyCallback cb) {
         Log.d(TAG, "FmProxy object created obj ="+this);
         mContext = ctx;
         mProxyCback = cb;

        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
        if(adapter == null) {
            Log.w(TAG, "BluetoothAdapter is null.");
            return ;
        }

        Intent intent = new Intent(IFmReceiverService.class.getName());
        intent.setComponent(intent.resolveSystemService(mContext.getPackageManager(), 0));
        if (!mContext.bindService(intent, mConnection, Context.BIND_AUTO_CREATE)) {
            Log.e(TAG, "Could not bind to IFmReceiverService Service");
        }
    }

    /**
     * Initialize the proxy with the service
     * @hide
     */
    protected boolean init(IBinder service) {
        try {
            mService = (IFmReceiverService) IFmReceiverService.Stub.asInterface(service);
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "Unable to initialize BluetoothFM proxy with service", t);
            return false;
        }
    }

    /**
     * Register a callback event handler to receive OPP events.
     * <p/>
     * @param handler the application handler to use for FM Receiver
     *                to use for handling callback events.
     */
    public synchronized void registerEventHandler(IFmReceiverEventHandler handler) {
        Log.v(TAG, "registerEventHandler()");
        registerEventHandler(handler, null, true, DEFAULT_BROADCAST_RECEIVER_PRIORITY);
    }

    public synchronized void registerEventHandler(
            IFmReceiverEventHandler eventHandler, IntentFilter filter,
            boolean createCallbackThread, int receiverPriority) {
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            registerEventHandler(eventHandler, filter,
                    createCallbackThread ? initEventCallbackHandler() : null,
                    receiverPriority);
        } else {
            registerEventHandler(eventHandler, null, null, receiverPriority);
        }
    }

    public synchronized void registerEventHandler(
            IFmReceiverEventHandler eventHandler, IntentFilter filter,
            Handler threadHandler, int receiverPriority) {
        // Store the client event handler
        mEventHandler = eventHandler;

        // Register a Broadcast receiver or the callback handler,
        // depending on configuration
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            if (mBroadcastReceiver == null) {
                try {
                    if (filter == null) {
                        filter = createFilter(null);
                    }
                    filter.setPriority(receiverPriority);
                    mBroadcastReceiver = new FmBroadcastReceiver();
                    mContext.registerReceiver(mBroadcastReceiver, filter,
                                              FM_RECEIVER_PERM, threadHandler);
                } catch (Throwable t) {
                    Log.e(TAG, "Error registering broadcast receiver");
                }
            }
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            if (mCallback == null) {
                mCallback = new FmReceiverCallback();
                try {
                    mService.registerCallback(mCallback);
                } catch (RemoteException e) {
                    Log.e(TAG, "Error registering callback handler", e);
                }
            }
        }
    }

    /**
     * Creates a filter for all FM events
     * @param filter
     * @return
     */
    public static IntentFilter createFilter(IntentFilter filter) {
        if (filter == null) {
            filter = new IntentFilter();
        }
        filter.addAction(ACTION_ON_AUDIO_MODE);
        filter.addAction(ACTION_ON_AUDIO_PATH);
        filter.addAction(ACTION_ON_AUDIO_QUAL);
        filter.addAction(ACTION_ON_EST_NFL);
        filter.addAction(ACTION_ON_RDS_DATA);
        filter.addAction(ACTION_ON_RDS_MODE);
        filter.addAction(ACTION_ON_SEEK_CMPL);
        filter.addAction(ACTION_ON_STATUS);
        filter.addAction(ACTION_ON_VOL);
        filter.addAction(ACTION_ON_WRLD_RGN);
        return filter;
    }

    public synchronized void unregisterEventHandler() {
        Log.v(TAG, "unregisterEventHandler()");

        mEventHandler = null;

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS) {
            if (mBroadcastReceiver != null) {
                mContext.unregisterReceiver(mBroadcastReceiver);
                mBroadcastReceiver = null;
            }
        } else {
            try {
                mService.unregisterCallback(mCallback);
            } catch (Throwable t) {
                Log.e(TAG, "Unable to unregister callback", t);
            }
        }
    }

    public synchronized void finish() {
        if (mEventHandler != null) {
            mEventHandler = null;
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS) {
            if (mBroadcastReceiver != null) {
                mContext.unregisterReceiver(mBroadcastReceiver);
                mBroadcastReceiver = null;
            }
        } else {
            if (mCallback != null && mService != null) {
                try {
                    mService.unregisterCallback(mCallback);
                } catch (Throwable t) {
                    Log.e(TAG, "Unable to unregister callback", t);
                }
                mCallback = null;
            }
        }
        baseFinish();
        mContext = null;
        mService = null;
    }

    /**
     * Turns on the radio and plays audio using the specified functionality
     * mask.
     * <p>
     * After executing this function, the application should wait for a
     * confirmatory status event callback before calling further API functions.
     * Furthermore, applications should call the {@link #turnOffRadio()}
     * function before shutting down.
     * @param functionalityMask
     *            is a bitmask comprised of one or more of the following fields:
     *            {@link #FUNC_REGION_NA}, {@link #FUNC_REGION_JP},
     *            {@link #FUNC_REGION_EUR}, {@link #FUNC_RDS},
     *            {@link #FUNC_RBDS} and {@link #FUNC_AF}
     *            
     * @param clientPackagename
     * 				is the the client application package name , this is required for the
     * 				fm service to clean up it state when the client process gets killed
     * 				eg scenario: when client app dies without calling turnOffRadio()
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public synchronized int turnOnRadio(int functionalityMask, String clientPackagename) {
        int returnCode = STATUS_SERVER_FAIL;

        Log.d(TAG,"Fmproxy"+FmProxy.this+"mService"+mService);
        /* Request this action from the server. */
        try {
            returnCode = mService.turnOnRadio(functionalityMask, clientPackagename.toCharArray());
        } catch (RemoteException e) {
            Log.e(TAG, "turnOnRadio() failed", e);
        }

        return returnCode;
    }

    /**
     * Turns on the radio and plays audio.
     * <p>
     * After executing this function, the application should wait for a
     * confirmatory status event callback before calling further API functions.
     * Furthermore, applications should call the {@link #turnOffRadio()}
     * function before shutting down.
     * @param clientPackagename
     * 				is the the client application package name , this is required for the
     * 				fm service to clean up it state when the client process gets killed
     * 				eg scenario: when client app dies without calling turnOffRadio()  
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * 
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public int turnOnRadio(String clientPackagename) {
        return turnOnRadio(FUNCTIONALITY_DEFAULT, clientPackagename);
    }
    
    /**
     * Turns off the radio.
     * <p>
     * After executing this function, the application should wait for a
     * confirmatory status event callback before shutting down.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public synchronized int turnOffRadio() {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.turnOffRadio();
        } catch (RemoteException e) {
            Log.e(TAG, "turnOffRadio() failed", e);
            return returnCode;
        }

        return returnCode;
    }

    /**
     * Initiates forced clean-up of FMReceiverService from the application
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int cleanupFmService() {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            mService.cleanupFmService();
        } catch (RemoteException e) {
            Log.e(TAG, "cleanupFmService() failed", e);
        }
        Log.i(TAG, "cleanup triggered");
        return returnCode;
    }

    /**
     * Tunes radio to a specific frequency. If successful results in a status
     * event callback.
      * @param freq
     *            the frequency to tune to.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public synchronized int tuneRadio(int freq) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.tuneRadio(freq);
        } catch (RemoteException e) {
            Log.e(TAG, "tuneRadio() failed", e);
        }

        return returnCode;
    }

    /**
     * Gets current radio status. This results in a status event callback.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public synchronized int getStatus() {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.getStatus();
        } catch (RemoteException e) {
            Log.e(TAG, "getStatus() failed", e);
        }

        return returnCode;
    }

    /**
     * Get the On/Off status of FM radio receiver module.
     * @return true if radio is on, otherwise returns false.
     */
    public boolean getRadioIsOn() {
        boolean returnStatus = false;
        if (mService == null) {
            return false;
        }
        try {
            returnStatus = mService.getRadioIsOn();
        } catch (RemoteException e) {
            Log.e(TAG, "getRadioIsOn() failed", e);
        }
        return returnStatus;
    }

    /**
     * Get the Audio Mode -
     *            {@link FmProxy#AUDIO_MODE_AUTO},
     *            {@link FmProxy#AUDIO_MODE_STEREO},
     *            {@link FmProxy#AUDIO_MODE_MONO} or
     *            {@link FmProxy#AUDIO_MODE_BLEND}.
     * @param none
     * @return the mAudioMode
     */
    public int getMonoStereoMode() {
        int returnStatus = AUDIO_MODE_AUTO;
        try {
            returnStatus = mService.getMonoStereoMode();
        } catch (RemoteException e) {
            Log.e(TAG, "getMonoStereoMode() failed", e);
        }
        return returnStatus;
    }

    /**
     *  Returns the present tuned FM Frequency
     * @param none
     * @return the mFreq
     */
    public int getTunedFrequency() {
        int returnStatus = 0;
        try {
            returnStatus = mService.getTunedFrequency();
        } catch (RemoteException e) {
            Log.e(TAG, "getTunedFrequency() failed", e);
        }
        return returnStatus;
    }

    /**
     * Returns whether MUTE is turned ON or OFF
     * @param none
     * @return false if MUTE is OFF ; true otherwise
     */
    public boolean getIsMute() {
        boolean returnStatus = false;
        try {
            returnStatus = mService.getIsMute();
        } catch (RemoteException e) {
            Log.e(TAG, "getIsMute() failed", e);
        }
        return returnStatus;
    }

    /**
     * Mutes/unmutes radio audio. If muted the hardware will stop sending audio.
     * This results in a status event callback.
     * @param mute
     *            True to mute audio, False to unmute audio.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onStatusEvent().
     */
    public synchronized int muteAudio(boolean mute) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.muteAudio(mute);
        } catch (RemoteException e) {
            Log.e(TAG, "muteAudio() failed", e);
        }

        return returnCode;
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel. Will
     * result in a seek complete event callback.
     * <p>
     * 
     * @param scanMode
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_DOWN},
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_FULL}.
     * @param minSignalStrength
     *            Minimum signal strength, default =
     *            {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * 
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public synchronized int seekStation(int scanMode, int minSignalStrength) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.seekStation(scanMode, minSignalStrength);
        } catch (RemoteException e) {
            Log.e(TAG, "seekStation() failed", e);
        }

        return returnCode;
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel. Will
     * result in a seek complete event callback.
     * <p>
     * Scans with default signal strength setting =
     * {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param scanMode
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_DOWN},
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_FULL}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public int seekStation(int scanMode) {
        return seekStation(scanMode, MIN_SIGNAL_STRENGTH_DEFAULT);
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel depending on the
     * scanDirection. Will do wrap around when reached to mMaxFreq/mMinFreq.
     * When no wrap around is needed, use the low_bound or high_bound as endFrequency.
     * Will result in a seek complete event callback.
     * <p>
     *
     * @param startFrequency
     *            Starting frequency of search operation range.
     * @param endFrequency
     *            Ending frequency of search operation
     * @param minSignalStrength
     *            Minimum signal strength, default =
     *            {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param scanDirection
     *            the direction to search in, it can only be either
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_DOWN}.
     * @param scanMethod
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_FAST},
     * @param multi_channel
     *            Is multiple channels are required, or only find next valid channel(seek).
     * @param rdsType
     *            the type of RDS condition to scan for.
     * @param rdsTypeValue
     *            the condition value to match.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     *
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public synchronized int seekStationCombo(int startFrequency, int endFrequency,
            int minSignalStrength, int scanDirection,
            int scanMethod, boolean multi_channel, int rdsType, int rdsTypeValue) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.seekStationCombo (startFrequency, endFrequency, minSignalStrength, scanDirection, scanMethod, multi_channel, rdsType, rdsTypeValue);
        } catch (RemoteException e) {
            Log.e(TAG, "seekStation() failed", e);
        }

        return returnCode;
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel that
     * supports the requested RDS functionality. Will result in a seek complete
     * event callback.
     * <p>
     * @param scanMode
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_DOWN},
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_FULL}.
     * @param minSignalStrength
     *            Minimum signal strength, default =
     *            {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param rdsCondition
     *            the type of RDS condition to scan for.
     * @param rdsValue
     *            the condition value to match.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public synchronized int seekRdsStation(int scanMode, int minSignalStrength,
            int rdsCondition, int rdsValue) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.seekRdsStation(scanMode, minSignalStrength,
                                                 rdsCondition, rdsValue);
        } catch (RemoteException e) {
            Log.e(TAG, "seekRdsStation() failed", e);
        }

        return returnCode;
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel that
     * supports the requested RDS functionality.. Will result in a seek complete
     * event callback.
     * <p>
     * Scans with default signal strength setting =
     * {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param scanMode
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_DOWN},
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_FULL}.
     * @param rdsCondition
     *            the type of RDS condition to scan for.
     * @param rdsValue
     *            the condition value to match.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public int seekRdsStation(int scanMode, int rdsCondition, int rdsValue) {
        return seekRdsStation(scanMode, MIN_SIGNAL_STRENGTH_DEFAULT, rdsCondition, rdsValue);
    }

    /**
     * Aborts the current station seeking operation if any. Will result in a
     * seek complete event containing the last scanned frequency.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onSeekCompleteEvent().
     */
    public synchronized int seekStationAbort() {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.seekStationAbort();
        } catch (RemoteException e) {
            Log.e(TAG, "seekStationAbort() failed", e);
        }

        return returnCode;
    }

    /**
     * Enables/disables RDS/RDBS feature and AF algorithm. Will result in a RDS
     * mode event callback.
     * <p>
     * @param rdsMode
     *            Turns on the RDS or RBDS. See {@link #RDS_MODE_OFF},
     *            {@link #RDS_MODE_DEFAULT_ON}, {@link #RDS_MODE_RDS_ON},
     *            {@link #RDS_MODE_RBDS_ON}
     * @param rdsFeatures
     *            the features to enable in RDS parsing.
     * @param afMode
     *            enables AF algorithm if True. Disables it if False
     * @param afThreshold
     *            the RSSI that the AF should jump to an alternate frequency on.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onRdsModeEvent().
     */
    public synchronized int setRdsMode(int rdsMode, int rdsFeatures,
            int afMode, int afThreshold) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setRdsMode(rdsMode, rdsFeatures, afMode, afThreshold);
        } catch (RemoteException e) {
            Log.e(TAG, "setRdsMode() failed", e);
        }

        return returnCode;
    }

    /**
     * Configures FM audio mode to be mono, stereo or blend. Will result in an
     * audio mode event callback.
     * @param audioMode
     *            the audio mode such as stereo or mono. The following values
     *            should be used {@link #AUDIO_MODE_AUTO},
     *            {@link #AUDIO_MODE_STEREO}, {@link #AUDIO_MODE_MONO} or
     *            {@link #AUDIO_MODE_BLEND}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onAudioModeEvent().
     */
    public synchronized int setAudioMode(int audioMode) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setAudioMode(audioMode);
        } catch (RemoteException e) {
            Log.e(TAG, "setAudioMode() failed", e);
        }

        return returnCode;
    }

    /**
     * Configures FM audio path to AUDIO_PATH_NONE, AUDIO_PATH_SPEAKER,
     * AUDIO_PATH_WIRED_HEADSET or AUDIO_PATH_DIGITAL. Will result in an audio
     * path event callback.

     * @param audioPath
     *            the audio path such as AUDIO_PATH_NONE, AUDIO_PATH_SPEAKER,
     *            AUDIO_PATH_WIRED_HEADSET or AUDIO_PATH_DIGITAL. The following
     *            values should be used {@link #AUDIO_PATH_NONE},
     *            {@link #AUDIO_PATH_SPEAKER}, {@link #AUDIO_PATH_WIRE_HEADSET}
     *            or {@link #AUDIO_PATH_DIGITAL}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onAudioPathEvent().
     */
    public synchronized int setAudioPath(int audioPath) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setAudioPath(audioPath);
        } catch (RemoteException e) {
            Log.e(TAG, "setAudioPath() failed", e);
        }

        return returnCode;
    }

    /**
     * Sets the minimum frequency step size to use when scanning for stations.
     * This function does not result in a status callback and the calling
     * application should therefore keep track of this setting.
     * @param stepSize
     *            a frequency interval set to {@link #FREQ_STEP_100KHZ} or
     *            {@link #FREQ_STEP_50KHZ}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setStepSize(int stepSize) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setStepSize(stepSize);
        } catch (RemoteException e) {
            Log.e(TAG, "setStepSize() failed", e);
        }

        return returnCode;
    }

    /**
     * Sets the FM volume.
     * @param volume
     *            range from 0 to 255
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onVolumeEvent().
     */
    public synchronized int setFMVolume(int volume) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setFMVolume(volume);
        } catch (RemoteException e) {
            Log.e(TAG, "setFMVolume() failed", e);
        }

        return returnCode;
    }

    /**
     * Sets a the world frequency region and the deemphasis time. This results
     * in a world frequency event callback.
     * @param worldRegion
     *            the world region the FM receiver is located. Set to
     *            {@link #FUNC_REGION_NA}, {@link #FUNC_REGION_EUR},
     *            {@link #FUNC_REGION_JP}, {@link #FUNC_REGION_JP_II}.
     * @param deemphasisTime
     *            the deemphasis time that can be set to either
     *            {@link #DEEMPHASIS_50U} or {@link #DEEMPHASIS_75U}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onWorldRegionEvent().
     */
    public synchronized int setWorldRegion(int worldRegion, int deemphasisTime) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setWorldRegion(worldRegion, deemphasisTime);
        } catch (RemoteException e) {
            Log.e(TAG, "setWorldRegion() failed", e);
        }

        return returnCode;
    }

    /**
     * Estimates the noise floor level given a specific type request. This
     * function returns an RSSI level that is useful for specifying as the
     * minimum signal strength for scan operations.
     * @param nflLevel
     *            estimate noise floor for {@link #NFL_LOW}, {@link #NFL_MED} or
     *            {@link #NFL_FINE}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onEstimateNflEvent().
     */
    public synchronized int estimateNoiseFloorLevel(int nflLevel) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.estimateNoiseFloorLevel(nflLevel);
        } catch (RemoteException e) {
            Log.e(TAG, "estimateNoiseFloorLevel() failed", e);
        }

        return returnCode;
    }

    /**
     * Enables or disables the live polling of audio quality on the currently
     * tuned frequency using a specific poll interval.
     * NOTE : SNR value will be returned a 0 for chips not supporting this SNR feature.
     * @param liveAudioPolling
     *            enables/disables live polling of audio quality.
     * @param signalPollInterval
     *            the sample interval for live polling of audio quality.
     * 
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     * @see IFmReceiverEventHandler.onLiveAudioQualityEvent().
     */
    public synchronized int setLiveAudioPolling(boolean liveAudioPolling, int signalPollInterval) {
        int returnCode = STATUS_SERVER_FAIL;
        /* Request this action from the server. */
        try {
            returnCode = mService.setLiveAudioPolling(liveAudioPolling, signalPollInterval);
        } catch (RemoteException e) {
            Log.e(TAG, "setLiveAudioPolling() failed", e);
        }

        return returnCode;
    }

    /**
     * Sets the SNR threshold for the subsequent FM frequency tuning.
     * This value will be used by BTA stack internally.
     *
     * @param signalPollInterval
     *           SNR Threshold value (0 ~ 31 (BTA_FM_SNR_MAX) )
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error code.
     */
    public synchronized int setSnrThreshold(int snrThreshold) {
        int returnCode = STATUS_SERVER_FAIL;

        /* Request this action from the server. */
        try {
            returnCode = mService.setSnrThreshold(snrThreshold);
        } catch (RemoteException e) {
            Log.e(TAG, "setSnrThreshold() failed", e);
        }
        return returnCode;
    }

    protected void finalize() {
        baseFinalize();
    }

    /**
     * The class containing all the FmProxy callback function handlers. These
     * functions will be called by the FmService module when callback
     * events occur. They in turn relay the callback information back to the
     * main applications callback handler.
     */
    private class FmReceiverCallback extends IFmReceiverCallback.Stub {

        public synchronized void onStatusEvent(int freq, int rssi, int snr,
                boolean radioIsOn, int rdsProgramType,
                String rdsProgramService, String rdsRadioText,
                String rdsProgramTypeName, boolean isMute)
                throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler) {
                mEventHandler.onStatusEvent(freq, rssi, snr, radioIsOn,
                    rdsProgramType, rdsProgramService, rdsRadioText,
                    rdsProgramTypeName, isMute);
            }
        }

        public synchronized void onSeekCompleteEvent(int freq, int rssi, int snr,
                boolean seeksuccess) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onSeekCompleteEvent(freq, rssi, snr, seeksuccess);
        }

        public synchronized void onRdsModeEvent(int rdsMode,
                int alternateFreqHopEnabled) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onRdsModeEvent(rdsMode, alternateFreqHopEnabled);
        }


        public synchronized void onRdsDataEvent(int rdsDataType, int rdsIndex,
                String rdsText) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onRdsDataEvent(rdsDataType, rdsIndex, rdsText);
        }


        public synchronized void onAudioModeEvent(int audioMode) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onAudioModeEvent(audioMode);
        }


        public synchronized void onAudioPathEvent(int audioPath) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onAudioPathEvent(audioPath);
        }


        public synchronized void onEstimateNflEvent(int nfl) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onEstimateNoiseFloorLevelEvent(nfl);
        }


        public synchronized void onLiveAudioQualityEvent(int rssi, int snr) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onLiveAudioQualityEvent(rssi, snr);
        }


        public synchronized void onWorldRegionEvent(int worldRegion) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onWorldRegionEvent(worldRegion);
        }

        public synchronized void onVolumeEvent(int status, int volume) throws RemoteException {
            /* Process and hand this event information to the application. */
            if (null != mEventHandler)
                mEventHandler.onVolumeEvent(status, volume);
        }
    };

    private class FmBroadcastReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            IFmReceiverEventHandler handler = mEventHandler;
            if (handler == null) {
                return;
            }

            abortBroadcast();

            String action = intent.getAction();
            if (actionsEqual(ACTION_ON_STATUS, action, ACTION_PREFIX_LENGTH)) {
                handler.onStatusEvent(intent.getIntExtra(EXTRA_FREQ, 0),
                                      intent.getIntExtra(EXTRA_RSSI, 0),
                                      intent.getIntExtra(EXTRA_SNR, -126),
                                      intent.getBooleanExtra(EXTRA_RADIO_ON, false),
                                      intent.getIntExtra(EXTRA_RDS_PRGM_TYPE, -1),
                                      intent.getStringExtra(EXTRA_RDS_PRGM_SVC),
                                      intent.getStringExtra(EXTRA_RDS_TXT),
                                      intent.getStringExtra(EXTRA_RDS_PRGM_TYPE_NAME),
                                      intent.getBooleanExtra(EXTRA_MUTED, false));
            } else if (actionsEqual(ACTION_ON_AUDIO_MODE, action, ACTION_PREFIX_LENGTH)) {
                handler.onAudioModeEvent(intent.getIntExtra(EXTRA_AUDIO_MODE, -1));
            } else if (actionsEqual(ACTION_ON_AUDIO_PATH, action, ACTION_PREFIX_LENGTH)) {
                handler.onAudioPathEvent(intent.getIntExtra(EXTRA_AUDIO_PATH, -1));
            } else if (actionsEqual(ACTION_ON_AUDIO_QUAL, action, ACTION_PREFIX_LENGTH)) {
                handler.onLiveAudioQualityEvent(intent.getIntExtra(EXTRA_RSSI, -1),
                                                intent.getIntExtra(EXTRA_SNR, -126));
            } else if (actionsEqual(ACTION_ON_EST_NFL, action, ACTION_PREFIX_LENGTH)) {
                handler.onEstimateNoiseFloorLevelEvent(intent.getIntExtra(EXTRA_NFL, -1));
            } else if (actionsEqual(ACTION_ON_RDS_DATA, action, ACTION_PREFIX_LENGTH)) {
                handler.onRdsDataEvent(intent.getIntExtra(EXTRA_RDS_DATA_TYPE, -1),
                                       intent.getIntExtra(EXTRA_RDS_IDX, -1),
                                       intent.getStringExtra(EXTRA_RDS_TXT));
            } else if (actionsEqual(ACTION_ON_RDS_MODE, action, ACTION_PREFIX_LENGTH)) {
                handler.onRdsModeEvent(intent.getIntExtra(EXTRA_RDS_MODE, -1),
                                       intent.getIntExtra(EXTRA_ALT_FREQ_MODE, -1));
            } else if (actionsEqual(ACTION_ON_SEEK_CMPL, action, ACTION_PREFIX_LENGTH)) {
                handler.onSeekCompleteEvent(intent.getIntExtra(EXTRA_FREQ, -1),
                                            intent.getIntExtra(EXTRA_RSSI, -1),
                                            intent.getIntExtra(EXTRA_SNR, -126),
                                            intent.getBooleanExtra(EXTRA_SUCCESS, false));
            } else if (actionsEqual(ACTION_ON_VOL, action, ACTION_PREFIX_LENGTH)) {
                handler.onVolumeEvent(intent.getIntExtra(EXTRA_STATUS, -1),
                                      intent.getIntExtra(EXTRA_VOL, -1));
            } else if (actionsEqual(ACTION_ON_WRLD_RGN, action, ACTION_PREFIX_LENGTH)) {
                handler.onWorldRegionEvent(intent.getIntExtra(EXTRA_WRLD_RGN, -1));
            }
        }
    }


    private static final boolean D = true;

    /**
     * Permission required to interact with Bluetooth profiles
     */
    public static final String BLUETOOTH_PERM = android.Manifest.permission.BLUETOOTH;

    /**
     * The default priority for a Broadcast Receiver created by the
     * proxy object, if Broadcast Intents are used to for events and
     * a receiver priority is not specified.
     */
    public static final int DEFAULT_BROADCAST_RECEIVER_PRIORITY = 200;


    /**
     * Fast comparison function to determine if two Bluetooth action strings
     * are equal. The first 'offset' characters are ignored.
     * @param a1
     * @param a2
     * @param offset
     * @return
     */
    protected static boolean actionsEqual(String a1, String a2, int offset) {
        if (a1 == a2)
            return true;
        int a1length = a1.length();
        if (a1length != a2.length())
            return false;

        return (a1.regionMatches(offset, a2, offset, a1length - offset));
    }

    protected Context mContext;
    protected EventCallbackHandler mEventCallbackHandler;
    protected BroadcastReceiver mBroadcastReceiver;
    protected int mReceiverPriority = DEFAULT_BROADCAST_RECEIVER_PRIORITY;
    protected boolean mIsAvailable;


    protected synchronized Handler initEventCallbackHandler() {
        if (mEventCallbackHandler == null) {
            mEventCallbackHandler = new EventCallbackHandler();
            mEventCallbackHandler.start();
            while (mEventCallbackHandler.mHandler ==null) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {}
            }
        }
        return mEventCallbackHandler.mHandler;
    }

    protected synchronized void finishEventCallbackHandler() {
        if (mEventCallbackHandler != null) {
            mEventCallbackHandler.finish();
            mEventCallbackHandler=null;
        }
    }

    /**
     * Closes connection to the Bluetooth service and releases resources.
     * Applications must call this method when it is done interacting with
     * the Bluetooth service.
     */
    public synchronized void baseFinish() {
        if (D) {
            Log.d(TAG,"finish() mContext = "+ mContext);
        }
        if (mEventCallbackHandler != null) {
            mEventCallbackHandler.finish();
            mEventCallbackHandler=null;
        }

        if (mContext != null) {
            mContext.unbindService(mConnection);
            mContext = null;
        }
    }

    /**
     * @return
     */
    public boolean requiresAccessProcessing() {
        return false;
    }

    protected void baseFinalize() {
        finish();
    }

    /**
     */
    private class EventCallbackHandler extends Thread {
        public Handler mHandler;
        public EventCallbackHandler() {
            super();
            setPriority(Process.THREAD_PRIORITY_BACKGROUND);
        }
        public void run() {
            Looper.prepare();
            mHandler = new Handler ();
            Looper.loop();
        }

        public void finish() {
            if (mHandler != null) {
                Looper l = mHandler.getLooper();
                if (l != null) {
                    l.quit();
                }
            }
        }
    }

    /**
     */
    protected IFmProxyCallback mProxyCback;

    /** Fm to BT service connection
     *  onProxyUnavailable() is in most cases an unexpected stop of FM service in BT
     *  process
     */
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            if (D) {
                Log.d(TAG, "Fm proxy onServiceConnected() name = " + className + ", service = "
                        + service);
            }
            if (service == null || !init(service) && mProxyCback != null) {
                Log.e(TAG, "Unable to create proxy");
            }
            if (mProxyCback != null) {
                mProxyCback.onProxyAvailable(FmProxy.this);
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            if (D)
                Log.d(TAG, "Fm Proxy object disconnected");
            mService = null;
            if (mProxyCback != null) {
                mProxyCback.onProxyUnAvailable();
                mProxyCback = null;
            }
        }
    };
}
