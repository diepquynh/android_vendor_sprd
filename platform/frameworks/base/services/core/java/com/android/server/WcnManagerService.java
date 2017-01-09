/*
 * Copyright (C) 2013 Sprdtrum.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.IBinder;
import android.os.Message;
import android.util.Slog;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import java.util.HashMap;

//TODO:

public class WcnManagerService {
    private static final String TAG = "WcnManagerService";

    private static final boolean DBG = true;

    private static final int EVENT_WIFI_SOFTAP_STARTED = 1000;

    private static final int EVENT_WIFI_SOFTAP_STOPPED = 1001;

    private static final int EVENT_LTE_BAND_CHANGED = 1002;

    private static final int EVENT_LISTEN_BAND_INFO = 1003;

    private static final int EVENT_LISTEN_NONE = 1004;


    private static final int DISABLE_LTE_BAND_REPORT = 0;

    private static final int ENABLE_LTE_BAND_REPORT = 1;

    /**
     * Binder context for this service
     */
    private Context mContext;

    private WifiManager mWifiManager = null;
    private WifiConfiguration mWifiApConfig = null;

    private RadioInteractor mRi;
    private boolean mSoftApStarted = false;

    private volatile String mLTEBand;
    private volatile String mLTEInfo;

    private boolean mLteBandReportEnabled = false;

    private final WakeLock mWakeLock;

    private InternalHandler mHandler = null;
    private boolean mRestartWifiApAfterChannelChange = false;


    private static final HashMap<String, Integer> softapBandMap = new HashMap<String, Integer>();

    //the band info from Tel/RIL
    static {
        softapBandMap.put("40", 11); //band 40
        softapBandMap.put("41", 11); //band 41
        softapBandMap.put("38", 11); //band 38
        softapBandMap.put("7", 1); // band 7
    }


    /**
     * Constructs a new WcnManagerService instance
     *
     * @param context  Binder context for this service
     */
    private WcnManagerService(Context context) {
        mContext = context;

        mWifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);

        HandlerThread handlerThread = new HandlerThread(TAG);
        handlerThread.start();
        mHandler = new InternalHandler(handlerThread.getLooper());

        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "WcnManagerService");

        mContext.bindService(new Intent(
                "com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
                .setPackage("com.android.sprd.telephony.server"),new ServiceConnection(){
            public void onServiceConnected(ComponentName name, IBinder service){
                log("on radioInteractor service connected");
                mRi = new RadioInteractor(mContext);
                IntentFilter filter = new IntentFilter();
                filter.addAction(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
                mContext.registerReceiver(mReceiver, filter);
                mHandler.sendMessage(mHandler.obtainMessage(EVENT_LISTEN_BAND_INFO));
            }
            public void onServiceDisconnected(ComponentName name){
                log("on radioInteractor service desconnect");
                mContext.unregisterReceiver(mReceiver);
                mHandler.sendMessage(mHandler.obtainMessage(EVENT_LISTEN_NONE));
            }
        },0);
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            handleBroadcastEvent(context, intent);
        }
    };
    /**
     * +SPCLB:<band>,<frequency>,<mode>,<state>,<paging_period>,<ts_type>,<tx_timing_advance>,<rssp>
     * or <band>,<frequency>,<mode>,<state>,<paging_period>,<ts_type>,<tx_timing_advance>,<rssp>
     * <band>: 1 ~ 41
     * <mode>: 0, master
     *                  1, slave
     * <state>: 0, cell searching
     *               1, idle
     *               2, connect
     * <ts_type>:
     * <rssp>:  dbm value
    */
    private void parseLteBandInfo(String bandInfoStr) {

        mLTEInfo = bandInfoStr; //TODO

        log("parse bindinfo:" + bandInfoStr);

        if (bandInfoStr != null) {
            String[] tokens = bandInfoStr.split(":");
            String bandValueInfo = null;

            if (tokens.length < 2) {
                mLTEInfo = tokens[0];
                bandValueInfo = tokens[0].split(",")[0];
            } else {
                mLTEInfo = tokens[1];
                bandValueInfo = tokens[1].split(",")[0];
            }

            log("bandValueInfo:" + bandValueInfo);
            if (bandValueInfo != null) {
                String[] bandVlaues = bandValueInfo.split(" ");
                if(bandVlaues.length < 2) {
                    mLTEBand = bandVlaues[0];
                } else {
                    mLTEBand = bandVlaues[1];
                }
            }
        }

        log("Get Current LTE BAND Value:" + mLTEBand + ", mLTEInfo: " + mLTEInfo);
    }

    public static WcnManagerService create(Context context) throws InterruptedException {
        final WcnManagerService service = new WcnManagerService(context);
        Slog.d(TAG, "Creating WcnManagerService");
        return service;
    }



    private void handleBroadcastEvent(Context context, Intent intent) {
        String action = intent.getAction();

        if (WifiManager.WIFI_AP_STATE_CHANGED_ACTION.equals(action)) {
            int softap_state = intent.getIntExtra(
                    WifiManager.EXTRA_WIFI_AP_STATE, WifiManager.WIFI_AP_STATE_FAILED);

            if ( softap_state == WifiManager.WIFI_AP_STATE_ENABLING ||softap_state == WifiManager.WIFI_AP_STATE_ENABLED) {
                mHandler.sendMessage(mHandler.obtainMessage(EVENT_WIFI_SOFTAP_STARTED));
            } else if (softap_state == WifiManager.WIFI_AP_STATE_DISABLED) {
                mHandler.sendMessage(mHandler.obtainMessage(EVENT_WIFI_SOFTAP_STOPPED));
            }
        }
    }

    private boolean isInteractionWithSoftap() {
        int ideaChannel = 0;
        try {
            Integer channel = null;

            if (mLTEBand != null) {
                channel = softapBandMap.get(mLTEBand);
            }

            if (channel != null)
                ideaChannel = channel;
        } catch (Exception e) {
            log("Exception: " + e);
        }

        if(mWifiApConfig != null) {
            log("ideaChannel:" + ideaChannel + " mWifiApConfig.apChannel:"+ mWifiApConfig.apChannel);
            if (mWifiApConfig.apChannel  == ideaChannel || ideaChannel == 0) {
                return false;
            }
        }
        mWifiApConfig.apChannel = ideaChannel;

        return true;
    }


    //On softap started
    private void onSoftApStarted() {

        String bandInfoStr = null;

        mLteBandReportEnabled = true;
        //param1:enable/disable LTE band report,param2:sim card
        mRi.setBandInfoMode(ENABLE_LTE_BAND_REPORT,0);

        //Get current LTE Working band
        bandInfoStr = mRi.getBandInfo(0);

        log("onSoftApStarted Get Current LTE BAND info:" + bandInfoStr);

        //parse and get the band value and other infor
        try {
            parseLteBandInfo(bandInfoStr);
        } catch (Exception e) {

        }
        mWifiApConfig = mWifiManager.getWifiApConfiguration();

        if(!isInteractionWithSoftap()) {
            return ;
        }
         if (mWifiApConfig != null) {
             mRestartWifiApAfterChannelChange = true;
             log("onSoftApStarted stop softap");
             mWifiManager.setWifiApEnabled(null, false);
         }
    }

    //On softap stopped
    private void onSoftApStopped() {

        //close band report
        if (mLteBandReportEnabled) {
            mLteBandReportEnabled = false;
            mRi.setBandInfoMode(DISABLE_LTE_BAND_REPORT,0);
        }
        if (mRestartWifiApAfterChannelChange) {
            mRestartWifiApAfterChannelChange = false;
            log("onSoftApStopped start softap");
            mWifiManager.setWifiApEnabled(mWifiApConfig, true);
        }
    }

    private void onLteBandChanged() {

     if (mSoftApStarted) {
            //check if an interaction will happens
             mWifiApConfig = mWifiManager.getWifiApConfiguration();
            if (!isInteractionWithSoftap()) {
                return;
            }
            if (mWifiApConfig != null) {
                mRestartWifiApAfterChannelChange = true;
                log("onLteBandChanged stop softap");
                mWifiManager.setWifiApEnabled(null, false);
            }
        } else {
            mWifiManager.setWifiApConfiguration(mWifiApConfig);
        }

    }


    private class InternalHandler extends Handler {
        public InternalHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (mRi == null) {
                log("mRi is NULL, return!");
                return;
            }
            switch (msg.what) {
                case EVENT_WIFI_SOFTAP_STARTED: {
                    if (mSoftApStarted) {
                        break;
                    }
                    mSoftApStarted = true;
                    log("EVENT_WIFI_SOFTAP_STARTED");
                    onSoftApStarted();
                    break;
                }

                case EVENT_WIFI_SOFTAP_STOPPED: {
                    if (!mSoftApStarted) {
                        break;
                    }
                    mSoftApStarted = false;
                    log("EVENT_WIFI_SOFTAP_STOPPED");
                    onSoftApStopped();
                    break;
                }

                case EVENT_LTE_BAND_CHANGED: {
                    onLteBandChanged();
                    break;
                }

                case EVENT_LISTEN_BAND_INFO: {
                    mRi.listen(mRadioInteractorCallbackListener,
                            RadioInteractorCallbackListener.LISTEN_BAND_INFO_EVENT, false);
                    break;
                }

                case EVENT_LISTEN_NONE: {
                    mRi.listen(mRadioInteractorCallbackListener,
                            RadioInteractorCallbackListener.LISTEN_NONE, false);
                    break;
                }
            }

        }
    }

    private RadioInteractorCallbackListener mRadioInteractorCallbackListener = new RadioInteractorCallbackListener(){

        public void onbandInfoEvent(Object  o) {
            String info = (String) ((AsyncResult) o).result;
            log("onbandInfoEvent:" + info);
            parseLteBandInfo(info);
            mHandler.sendMessage(mHandler.obtainMessage(EVENT_LTE_BAND_CHANGED));
        }
    };
    private void log(String s) {
        Slog.d(TAG, s);
    }

}
