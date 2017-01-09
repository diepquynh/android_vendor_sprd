/*
 ** Copyright 2015 The Spreadtrum.com
 */


package com.android.server.power;

import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.AlarmManager;
import android.app.PowerGuru;
import android.app.IPowerGuru;
import android.app.PowerGuruAlarmInfo;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;

import android.net.Uri;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.wifi.WifiManager;
import android.os.Debug;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.WorkSource;
import android.telephony.CarrierConfigManager;
import android.telephony.TelephonyManager;

import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.Time;

import android.util.Log;
import android.util.Pair;
import android.util.Slog;
import android.util.TimeUtils;
import android.util.AtomicFile;
import android.util.Xml;

import java.io.File;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;


import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.HashSet;

import java.util.TimeZone;
import java.util.Iterator;

import java.util.Locale;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.Iterator;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.util.FastXmlSerializer;
import org.xmlpull.v1.XmlPullParserException;

import static android.app.AlarmManager.RTC_WAKEUP;
import static android.app.AlarmManager.RTC;
import static android.app.AlarmManager.ELAPSED_REALTIME_WAKEUP;
import static android.app.AlarmManager.ELAPSED_REALTIME;
import static android.app.AlarmManagerWrapper.POWER_OFF_WAKEUP;

import static android.app.PowerGuru.HEARTBEAT_ENABLE;


import com.android.internal.util.LocalLog;
import android.telephony.SubscriptionManager;

class PowerGuruService extends AbsPowerGuruService {


    /******************final members******************/
    private static final String TAG = "PowerGuruService";

    public final boolean DBG = isDebug();



    private static final int LOAD_DATA                      = 0;
    private static final int NEW_ALARM                 = 1;
    private static final int SCREEN_OFF                 = 2;
    private static final int SCREEN_ON                 = 3;
    private static final int SUSPECTED_HEARTBEAT_ALARM_CHECK                = 4;
    private static final int HEARTBEAT_ALARM_LIST_UPDATE                    = 5;
    private static final int WIFI_NETWORK_CONNECTION_CHANGE                 = 6;
    private static final int VPN_NETWORK_CONNECTION_CHANGE                  = 7;
    private static final int SAVE_WHITE_LIST_TO_DISK                        = 8;
    private static final int SAVE_STUDY_HEARTBEAT_LIST_TO_DISK              = 9;
    private static final int UPDATE_DATA_ON_PACKAGES_CHANGED                    = 10;
    private static final int CMD_BATTERY_CHANGED                    = 11;
    private static final int ALIGN_TIMES_CHANGED = 12;

    private static final int PACKAGES_CHANGE_REASON_ADD = 0;
    private static final int PACKAGES_CHANGE_REASON_REMOVE = 1;
    private static final int PACKAGES_CHANGE_REASON_AVAILABLE = 2;
    private static final int PACKAGES_CHANGE_REASON_UNAVAILABLE = 3;
    private static final int PACKAGES_CHANGE_REASON_REPLACED = 4;

    private static final String XML_ALARMINFO_FILE_TAG =  "alarm_info_list";
    private static final String XML_ALARMINFO_BODY_TAG =  "alarmInfo_body";
    private static final String XML_ALARMINFO_PKG_TAG =   "package_name";
    private static final String XML_ALARMINFO_ACT_TAG =   "action_name";
    private static final String XML_ALARMINFO_CPN_TAG =   "conponent";
    private static final String XML_ALARMINFO_TYPE_TAG =  "alarm_type";
    private static final String XML_ALARMINFO_MERGE_TAG = "merge";
    private static final String XML_ALARMINFO_GMS_TAG =   "isGms";
    private static final String XML_ALARMINFO_AVAILABLE_TAG =   "isAvail";

    private static final String XML_APP_LIST_FILE_TAG = "app_list";
    private static final String XML_APP_LIST_PKG_BODY_TAG = "packageItem";
    private static final String XML_APP_LIST_PKG_NAME_TAG  = "pkg_name";


    /**
     * Binder context for this service
     */
    private Context mContext;

    private InternalHandler mHandler = null;



    private Object mHeartbeatListLock = new Object();


    private List<AlarmInfo> mHistoryAlarmList = new ArrayList<AlarmInfo>();

    //all heart beat alarm since system bootup
    //and the data required from alarm based on this
    private List<AlarmInfo> mHeartbeatAlarmList = new ArrayList<AlarmInfo>();

    private List<AlarmInfo> mSuspectedHeartbeatAlarmList = new ArrayList<AlarmInfo>();

    //user selected which is unnecessary to monitor
    private List<String> mWhiteAppList = new ArrayList<String>();

    /*Heartbeat List that preset in system */
    private List<PowerGuruAlarmInfo> mPresetHeartbeatList = new ArrayList<PowerGuruAlarmInfo>();

    /*Heartbeat List that saved during runtime */
    //all studied
    private List<PowerGuruAlarmInfo> mSavedHeartbeatList = new ArrayList<PowerGuruAlarmInfo>();

    private List<String> mCandicateWhiteList = new ArrayList<String>();


    private static final int DEFAULT_ALARM_DETECT_DURATION = 60 * 60 * 1000; /* 60 minutes */



    private static final String LEVEL1_INTERVAL_PROPERTY_KEY = "heartbeat.interval.level1";
    private static final String LEVEL2_INTERVAL_PROPERTY_KEY = "heartbeat.interval.level2";

    /**
     */
    private static final int DEFAULT_ALARM_REPEAT_INTERVAL_MINS_LEVEL1 = 15; /* 15 minutes */
    private static final int DEFAULT_ALARM_REPEAT_INTERVAL_MINS_LEVEL2 = 10; /* 10 minutes */


    private long mAlarmRepeatIntervalMsLevel1;
    private long mAlarmRepeatIntervalMsLevel2;


    /**
     */
    private AlarmManager mAlarmManager;



    private static final String ACTION_VPN_ON= "android.intent.action.VPN_ON";
    private static final String ACTION_VPN_OFF= "android.intent.action.VPN_OFF";


    private boolean mHasVpn = false;
    private boolean mWifiConnected = false;
    private boolean mScreenOff = false;
    private boolean mHeartbeatListUpdate = false;
    private boolean mCurrentAdjustEnabled = false;
    private boolean mAllGmsAlarmDisabled = false;
    private int mPluggedType = 0;
    private int mDelayTime = 60 * 1000;
    private TelephonyManager mTelephonyManager;
    private SprdNatTimeUtils mSprdNatTimeUtils;
    private boolean SET_ALARM_ALIGNTIME = (1 == SystemProperties.getInt("persist.sys.alarm.aligntime", 0));

    /*Black App list, all alarm from Black app are needed to be adjusted ??? */
    private List<String> mBlackAppList = new ArrayList<String>();

    private final String[] mGMSAppList = new String[] {
        "com.google.android.gms",
        "com.google.android.talk",
        "com.google.android.apps.plus",
        "com.android.vending",
        "com.google.android.googlequicksearchbox",
        "com.google.android.apps.docs",
        "com.google.android.apps.magazines",
        "com.google.android.videos",
        "com.google.android.music",
        "com.google.android.gm",
        "com.google.android.youtube",
        "com.google.android.apps.maps",
        "com.google.android.play.games",
        "com.google.android.youtube",
        "com.android.chrome",
        "com.google.android.partnersetup",
        "com.google.android.gms.drive",
        "com.google.process.gapps",
        "com.google.android.apps.books",
        "com.google.android.apps.translate",
        "com.google.earth"
    };


    /*This white list is used for some app like CTS*/
    private final String[] mInternalWhiteAppList = new String[] {
        "android.app.cts"
    };



    private final String[] mHeartbeatFetureStrings = new String[] {
        "KEEP_ALIVE",
        "KEEPALIVE",
        /*"heartbeat",*/
        "PING",
    };

    private final File mRootDataDir = Environment.getDataDirectory();
    private AtomicFile mPresetHeartBeatRecordFile = null;
    private AtomicFile mStudiedHeartBeatRecordFile = null;
    private AtomicFile mWhiteAppFile = null;
    private AtomicFile mBlackAppFile = null;


    private DelayedDiskWrite mDelayedDiskWrite = null;


    public static boolean isEnabled() {
        Log.d(TAG, "powergutu isEnabled : " + (1 == SystemProperties.getInt(HEARTBEAT_ENABLE, 1)));
        if (1 == SystemProperties.getInt(HEARTBEAT_ENABLE, 1)){
            return true;
        }
        return false;
    }



    /******************inner classes*******************/
    private class AlarmInfo {

        public int type;
        public long when;
        public long windowLength;
        public long whenElapsed;    // 'when' in the elapsed time base
        public long maxWhen;        // also in the elapsed time base
        public long repeatInterval;
        public PendingIntent operation;

        public String packageName;
        public String action;
        public String componentName;

        public long firstSetTime;
        public long lastSetTime;

        public int setCount;
        public int userSetCount;
        public int setMinsAlignCount; //the trigger time is align to minutes with seconds is 0

        public boolean isFirstTriggerTimeAlignToMins; //the first time set the trigger time is align to minutes with seconds is 0
        public boolean isClockAlarm;

        public boolean isRepeat;

        public boolean isFromGms;

        public boolean isAvailable;

        public AlarmInfo(int _type, long _when, long _whenElapsed, long _windowLength, long _maxWhen,
                long _interval, PendingIntent _op) {
            type = _type;
            when = _when;
            whenElapsed = _whenElapsed;
            windowLength = _windowLength;
            maxWhen = _maxWhen;
            repeatInterval = _interval;
            operation = _op;

            firstSetTime = System.currentTimeMillis();
            lastSetTime = firstSetTime;
            setCount = 1;
            userSetCount = 0;
            setMinsAlignCount = 0;

            isFirstTriggerTimeAlignToMins = false;
            isClockAlarm = false;

            isRepeat = true;
            if (repeatInterval == 0) isRepeat = false;

            isFromGms = false;
            isAvailable = true;

            action = null;
            componentName = null;
            packageName = null;

            ComponentName cn = null;
            Intent intent = null;
            try{
                if(operation != null){
                    intent = operation.getIntent();
                    packageName = operation.getTargetPackage();
                }
                if(intent != null){
                    action = intent.getAction();
                    cn = intent.getComponent();
                    if (cn != null)
                        componentName = cn.getClassName();
                }

            }catch (Exception e) {
                //log("Unkown Exception:" + e);
            }

        }

        public String toString(){
            return (Integer.toString(type)
                + Long.toString(when)
                + Long.toString(whenElapsed)
                + Long.toString(windowLength)
                + Long.toString(maxWhen)
                + Long.toString(repeatInterval)
                + operation
                + packageName
                + setCount);
        }
    }


    public PowerGuruService(Context context) {
        super(context);
        mContext = context;

        HandlerThread handlerThread = new HandlerThread(TAG);
        handlerThread.start();
        mHandler = new InternalHandler(handlerThread.getLooper());



        int timeMinsLevel1 = SystemProperties.getInt(LEVEL1_INTERVAL_PROPERTY_KEY, DEFAULT_ALARM_REPEAT_INTERVAL_MINS_LEVEL1);
        int timeMinsLevel2 = SystemProperties.getInt(LEVEL2_INTERVAL_PROPERTY_KEY, DEFAULT_ALARM_REPEAT_INTERVAL_MINS_LEVEL2);

        mAlarmRepeatIntervalMsLevel1 = timeMinsLevel1 * 60 * 1000; //ms
        mAlarmRepeatIntervalMsLevel2 = timeMinsLevel2 * 60 * 1000; //ms

        log("Level1  alarm repeat interval: " + mAlarmRepeatIntervalMsLevel1 + ", Level2 alarm repeat interval: " + mAlarmRepeatIntervalMsLevel2);


        registerForBroadcasts();


        /*can not get alarm manager here. it will fail, because alarm manager service is registered after PowerGuru*/


        mPresetHeartBeatRecordFile = new AtomicFile(new File(new File(mRootDataDir, "system"), "pwrGuruPreset.xml"));
        mStudiedHeartBeatRecordFile = new AtomicFile(new File(new File(mRootDataDir, "system"), "pwrGuruStudied.xml"));
        mWhiteAppFile = new AtomicFile(new File(new File(mRootDataDir, "system"), "whiteAppList.xml"));
        mBlackAppFile = new AtomicFile(new File("/system/etc/blackAppList.xml"));

        mHandler.sendMessage(mHandler.obtainMessage(LOAD_DATA));
        mDelayedDiskWrite = new DelayedDiskWrite();
        mTelephonyManager = (TelephonyManager)mContext.getSystemService(mContext.TELEPHONY_SERVICE);
        if(SET_ALARM_ALIGNTIME){
            mSprdNatTimeUtils = new SprdNatTimeUtils(mContext);
        }
    }



    private class InternalHandler extends Handler {
        public InternalHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case LOAD_DATA:
                    log("LOAD_DATA");
                    initData();

                    break;

                case NEW_ALARM:
                    log("NEW_ALARM");

                    AlarmInfo alarm = (AlarmInfo) msg.obj;

                    try {
                        processInputAlarm(alarm);
                    } catch (Exception e) {
                        loge("Unkown Exception:" + e);
                    }
                    break;


                case SCREEN_OFF:
                    log("SCREEN_OFF, mPluggedType = " + mPluggedType);
                    mScreenOff = true;
                    if (shouldApplyAlarmAdjust(mPluggedType)) {
                        if (mAlarmManager == null) {
                            mAlarmManager = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
                        } else {
                            if (SET_ALARM_ALIGNTIME && mSprdNatTimeUtils != null) {
                                int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
                                int mNetworkClass = TelephonyManager
                                        .getNetworkClass(mTelephonyManager
                                                .getNetworkType(dataSubId));
                                int newAlignTimes = mSprdNatTimeUtils.getNatOvertimeByNetClass(
                                        mNetworkClass, dataSubId);
                                log("SCREEN_OFF, setAlignTimeLength = " + newAlignTimes);
                                mAlarmManager.setAlignLength(newAlignTimes);
                            }
                            mAlarmManager.setHeartBeatAdjustEnable(true, mHeartbeatListUpdate);
                            mCurrentAdjustEnabled = true;
                        }
                    }
                    break;

                case SCREEN_ON:
                    log("SCREEN_ON, mCurrentAdjustEnabled = " + mCurrentAdjustEnabled);
                    mScreenOff = false;
                    if (mCurrentAdjustEnabled) { //current is enabled, now screen is on, disable it
                        if (mAlarmManager != null) {
                            mAlarmManager.setHeartBeatAdjustEnable(false, mHeartbeatListUpdate);
                            mCurrentAdjustEnabled = false;
                        } else {
                            mAlarmManager = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
                        }
                    }
                    break;

                case SUSPECTED_HEARTBEAT_ALARM_CHECK:
                    log("SUSPECTED_HEARTBEAT_ALARM_CHECK");


                    break;

                case HEARTBEAT_ALARM_LIST_UPDATE:
                    log("HEARTBEAT_ALARM_LIST_UPDATE (mScreenOff = " + mScreenOff + ")" + " mPluggedType = " + mPluggedType);
                    if (mScreenOff && shouldApplyAlarmAdjust(mPluggedType)) {
                        if (mAlarmManager != null) {
                            mAlarmManager.setHeartBeatAdjustEnable(true, mHeartbeatListUpdate);
                            mCurrentAdjustEnabled = true;
                        } else {
                            mAlarmManager = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
                        }
                    }
                    break;

                case WIFI_NETWORK_CONNECTION_CHANGE:
                    boolean iswificonnected = (msg.arg1 == 1);

                    if (mWifiConnected != iswificonnected) {
                        mWifiConnected = iswificonnected;
                        log("WIFI_NETWORK_CONNECTION_CHANGE (mWifiConnected = " + mWifiConnected + ")");

                        try {
                            netConnectionChanged();
                        } catch (Exception e) {
                            loge("Unkown Exception:" + e);
                        }
                    }

                    break;

                case VPN_NETWORK_CONNECTION_CHANGE:
                    boolean isvpnconnected = (msg.arg1 == 1);

                    if (mHasVpn != isvpnconnected) {
                        mHasVpn = isvpnconnected;
                        log("VPN_NETWORK_CONNECTION_CHANGE (mHasVpn = " + mHasVpn + ")");

                        try {
                            netConnectionChanged();
                        } catch (Exception e) {
                            loge("Unkown Exception:" + e);
                        }

                    }
                    break;

                case SAVE_WHITE_LIST_TO_DISK:
                    log("SAVE_WHITE_LIST_TO_DISK");
                    saveWhileListToDisk();
                    break;

                case SAVE_STUDY_HEARTBEAT_LIST_TO_DISK:
                    log("SAVE_STUDY_HEARTBEAT_LIST_TO_DISK");
                    //saveStudyHeartbeatListToDisk(); //just do not need to save to disk
                    break;

                case UPDATE_DATA_ON_PACKAGES_CHANGED:
                    log("UPDATE_DATA_ON_PACKAGES_CHANGED");
                    try {
                        List<String> removedList = (List<String>) msg.obj;
                        int changeType = (int)msg.arg1;
                        int updateReason = (int)msg.arg2;
                        updateDataOnPackagesChanged(removedList,changeType,updateReason);
                        notifyDataChanged();
                    } catch (Exception exp){
                        loge("exp while update data,exp:"+exp);
                    }
                    break;

                case CMD_BATTERY_CHANGED:
                    int newPlugType = msg.arg1;

                    /** Power source is an AC charger.
                     * public static final int BATTERY_PLUGGED_AC = 1;
                     * Power source is a USB port.
                     * public static final int BATTERY_PLUGGED_USB = 2;
                     * Power source is wireless.
                     * public static final int BATTERY_PLUGGED_WIRELESS = 4;
                     */
                    log("CMD_BATTERY_CHANGED (newPlugType = " + newPlugType + ")");

                    boolean needApplyByPlugType = shouldApplyAlarmAdjust(newPlugType);
                    if (mScreenOff && shouldApplyAlarmAdjust(mPluggedType) != needApplyByPlugType) {
                        if (mAlarmManager != null) {
                            mAlarmManager.setHeartBeatAdjustEnable(needApplyByPlugType, mHeartbeatListUpdate);
                            mCurrentAdjustEnabled = needApplyByPlugType;
                        } else {
                            mAlarmManager = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
                        }
                    }

                    mPluggedType = newPlugType;
                    break;

                case ALIGN_TIMES_CHANGED:
                    if (shouldApplyAlarmAdjust(mPluggedType) && mScreenOff) {
                        int mAlignTimes = msg.arg1;
                        int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
                        int mNetworkClass = TelephonyManager.getNetworkClass(mTelephonyManager
                                .getNetworkType(dataSubId));
                        int newAlignTimes = mSprdNatTimeUtils.getNatOvertimeByNetClass(
                                mNetworkClass, dataSubId);
                        if (newAlignTimes == mAlignTimes) {
                            mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
                            log("ALIGN_TIMES_CHANGED, setAlignTimeLength = " + newAlignTimes);
                            mAlarmManager.setAlignLength(newAlignTimes);
                        }
                    }
                    break;

                default:
                    loge("unknown event: " + msg);
                    break;


            }
        }
    }

    private class PackageUpdateReceiver extends BroadcastReceiver {
        public PackageUpdateReceiver(Context context) {
            IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
            filter.addAction(Intent.ACTION_PACKAGE_ADDED);
            filter.addAction(Intent.ACTION_PACKAGE_REPLACED);
            filter.addDataScheme("package");
            PowerGuruService.this.mContext.registerReceiver(this, filter);
             // Register for events related to sdcard installation.
            IntentFilter sdFilter = new IntentFilter();
            sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE);
            sdFilter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE);
            PowerGuruService.this.mContext.registerReceiver(this, sdFilter);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            String pkgList[] = null;
            int changeType = 0;// 0-removed,1-added
            int updateReason = 0;
            log("PackageUpdateReceiver:onReceive,action = "+action);
            if (Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE.equals(action)) { // unavailable
                pkgList = intent.getStringArrayExtra(Intent.EXTRA_CHANGED_PACKAGE_LIST);
                changeType = 0;
                updateReason = PACKAGES_CHANGE_REASON_UNAVAILABLE;
            } else if (Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(action)){
                pkgList = intent.getStringArrayExtra(Intent.EXTRA_CHANGED_PACKAGE_LIST);
                changeType = 1;
                updateReason = PACKAGES_CHANGE_REASON_AVAILABLE;
            } else if (Intent.ACTION_PACKAGE_REMOVED.equals(action)){ //removed
                if (intent.getBooleanExtra(Intent.EXTRA_REPLACING, false)) {
                    // This package is being updated; don't kill its alarms.
                    log("PackageUpdateReceiver onReceive,EXTRA_REPLACING is true");
                    return;
                }
                Uri data = intent.getData();
                if (data != null) {
                    String pkg = data.getSchemeSpecificPart();
                    if (pkg != null) {
                        pkgList = new String[]{pkg};
                    }
                }
                changeType = 0;
                updateReason = PACKAGES_CHANGE_REASON_REMOVE;
            } else if (Intent.ACTION_PACKAGE_REPLACED.equals(action)){
                Uri data = intent.getData();
                if (data != null) {
                    String pkg = data.getSchemeSpecificPart();
                    if (pkg != null) {
                        pkgList = new String[]{pkg};
                    }
                }
                changeType = 2;
                updateReason = PACKAGES_CHANGE_REASON_REPLACED;
            } else if (Intent.ACTION_PACKAGE_ADDED.equals(action)){//package added
                Uri data = intent.getData();
                if (data != null) {
                    String pkg = data.getSchemeSpecificPart();
                    if (pkg != null) {
                        pkgList = new String[]{pkg};
                    }
                }
                changeType = 1;
                updateReason = PACKAGES_CHANGE_REASON_ADD;
            } else {
                log("invalid intent,return");
                return;
            }
            if (pkgList != null && (pkgList.length > 0)) {
                List<String> list = Arrays.asList(pkgList);
                log("uninstallReceiver:send msg");
                mHandler.sendMessage(mHandler.obtainMessage(UPDATE_DATA_ON_PACKAGES_CHANGED, changeType,updateReason,list));
            }
        }
    }

    private void dumpAlarm(AlarmInfo alarm) {
        if (DBG) {
            long realWhen = convertToRealTime(alarm.when, alarm.type);
            CharSequence timeStr = DateFormat.format("yyyy-MM-dd HH:mm:ss", realWhen);
            CharSequence firstTimeStr = DateFormat.format("yyyy-MM-dd HH:mm:ss", alarm.firstSetTime);
            CharSequence lastTimeStr = DateFormat.format("yyyy-MM-dd HH:mm:ss", alarm.lastSetTime);
            log("alarm info: packageName = " + alarm.packageName
                + ", Action = " + alarm.action
                + ", Component = " + alarm.componentName
                + ", type = " + alarm.type
                + ", when = " + alarm.when
                + " (" + timeStr + ")"
                + ", interval = " + alarm.repeatInterval
                + ", setCount = " + alarm.setCount
                + ", userSetCount = " + alarm.userSetCount
                + ", firstSetTime = " + alarm.firstSetTime
                + " (" + firstTimeStr + ")"
                + ", isRepeat = " + alarm.isRepeat
                + ", isFromGMS = " + alarm.isFromGms
                + ", isAvailable = " + alarm.isAvailable
                + ", lastSetTime = " + lastTimeStr
                + ", setMinsAlignCount = " + alarm.setMinsAlignCount);
        }

    }


    private boolean isAlarmMatched(AlarmInfo a1, AlarmInfo a2, boolean isRepeat) {

        if (a1 == null || a2 == null) return false;

        if (a1.packageName != null  && a1.packageName.equals(a2.packageName) &&
            ((a1.action !=null  && a1.action.equals(a2.action)) ||(a1.action == null && a2.action ==null)) &&
            ((a1.componentName !=null  && a1.componentName.equals(a2.componentName)) ||(a1.componentName == null && a2.componentName ==null)) /*&&
            (a1.type == a2.type)*/
            ) { //check packageName && action && component && type

            if (isRepeat) { //also check interval
                if ((a1.repeatInterval != 0) && (a2.repeatInterval != 0))
                    return true;
                else
                    return false;
            }

            return true;
        }

        return false;
    }

    private boolean isAlarmMatched(PowerGuruAlarmInfo a1, AlarmInfo a2, boolean isRepeat) {

        if (a1 == null || a2 == null) return false;

        if (a1.packageName != null && a1.packageName.equals(a2.packageName) &&
            ((a1.actionName !=null  && a1.actionName.equals(a2.action)) ||(a1.actionName == null && a2.action ==null)) &&
            ((a1.componentName !=null  && a1.componentName.equals(a2.componentName)) ||(a1.componentName == null && a2.componentName ==null)) /*&&
            (a1.alarmType == a2.type)*/
            ) { //check packageName && action && component && type

            if (isRepeat) { //also check interval
                if (a2.repeatInterval != 0)
                    return true;
                else
                    return false;
            }

            return true;
        }

        return false;
    }


    private boolean isInBlackAppList(AlarmInfo alarm) {
        for(String s : mBlackAppList) {
            if(s != null  && s.equals(alarm.packageName)) {

                return true;
            }
        }

        return false;
    }


    private boolean isThirdParty(AlarmInfo alarm) {
        for(String s : mCandicateWhiteList) {
            if(s != null  && s.equals(alarm.packageName)) {

                return true;
            }
        }

        return false;
    }

    private boolean isInWhiteAppList(AlarmInfo alarm) {
        for(String s : mWhiteAppList) {
            if(s != null  && s.equals(alarm.packageName)) {

                return true;
            }
        }

        /*check if in internal white app list, like CTS app*/
        for(String s2 : mInternalWhiteAppList) {
            if(s2 != null  && s2.equals(alarm.packageName)) {

                return true;
            }
        }

        return false;
    }


    private boolean isPresetHeartbeatAlarm(AlarmInfo alarm) {

        for(PowerGuruAlarmInfo a: mPresetHeartbeatList) {
            if (isAlarmMatched(a, alarm, true)) {
                return true;
            }
        }

        return false;
    }


    private boolean isSavedHeartbeatAlarm(AlarmInfo alarm) {
        for(PowerGuruAlarmInfo a: mSavedHeartbeatList) {
            if (isAlarmMatched(a, alarm, false)) {
                return true;
            }
        }

        return false;
    }


    private boolean containHeartbeatAlarmFeature(AlarmInfo alarm) {

        for(String s : mHeartbeatFetureStrings) {
            if(alarm.action != null  && alarm.action.contains(s)) {
                return true;
            }
        }

        return false;
    }

    /** need consider alarm from Gms App*/
    private boolean addToHeartbeatAlarmList(AlarmInfo alarm, boolean isRepeat, boolean isFromSaved) {

        boolean isNewAlarm = true;
        boolean isAlarmUpdate = false;
        synchronized (mHeartbeatListLock) {

            log("Heartbeat Alarm size:" + mHeartbeatAlarmList.size());

            for(AlarmInfo a : mHeartbeatAlarmList) {
               if(isAlarmMatched(a, alarm, isRepeat)) {
                    log("Already added to Heartbeat Alarm List");
                    isNewAlarm = false;

                    /*if a GMS alarm, need to update */
                    if (alarm.isFromGms) {
                        if (alarm.isAvailable != a.isAvailable) {
                            log("Alarm from GMS update in Heartbeat Alarm List");
                            a.isAvailable = alarm.isAvailable;
                            isAlarmUpdate = true;
                        }
                    }

                    break;
                }
            }

            if (isNewAlarm) {
                log("New alarm add to Heartbeat Alarm List");
                /*add to Heartbeat Alarm list and Saved Heartbeat list*/
                mHeartbeatAlarmList.add(alarm);

                if (!alarm.isFromGms && !isFromSaved) { // do not save alarm from GMS app, because it depends on environment

                    if (!isSavedHeartbeatAlarm(alarm)) {
                        mSavedHeartbeatList.add(new PowerGuruAlarmInfo(alarm.packageName, alarm.action,
                            alarm.componentName, alarm.type, alarm.isFromGms, alarm.isAvailable));

                        // to save to disk every update ???
                        mHandler.sendMessage(mHandler.obtainMessage(SAVE_STUDY_HEARTBEAT_LIST_TO_DISK));
                    }

                }
            }

            if ( isNewAlarm || isAlarmUpdate) {
                /*notify update*/
                mHeartbeatListUpdate = true;
                mHandler.sendMessage(mHandler.obtainMessage(HEARTBEAT_ALARM_LIST_UPDATE));
            }


            /* for debug */
            log("current Hearbeat Alarms:");
            for(AlarmInfo a : mHeartbeatAlarmList) {
                dumpAlarm(a);
            }

        }

        return true;

    }



    private void updateHistoryAlarm(AlarmInfo alarm, AlarmInfo newAlarm) {

        long now = System.currentTimeMillis();
        long elapsed = now - alarm.firstSetTime;
        long detectDuration = DEFAULT_ALARM_DETECT_DURATION;

        /*if userSetCount > 0 && packages name contains "clock" extend the study duration*/
        if ((alarm.userSetCount > 0) && alarm.packageName.contains("clock")) {
            detectDuration = 2 * DEFAULT_ALARM_DETECT_DURATION;
        }

        if (elapsed < detectDuration) {

            long oldTriggerTime = convertToRealTime(alarm.when, alarm.type);
            long newTriggerTime = convertToRealTime(newAlarm.when, newAlarm.type);

            /* if the new triggerTime is after the detect duration, do not care it */
            if (newTriggerTime >=  now + DEFAULT_ALARM_DETECT_DURATION) {
                log("This Action = " + alarm.action + " its trigger time is after the detect duration, don't care about it!");
                return;
            }

            /* The time of setting alarm is after the trigger time of the last setting, this like the behavior of the clock alarm */
            if ( (newAlarm.firstSetTime > oldTriggerTime + 5*1000)
                && (alarm.windowLength == 0) && (newAlarm.windowLength == 0)) {
                log("This Exact alarm: Action = " + alarm.action + " may be set by user."
                    + " newAlarm.firstSetTime=" + newAlarm.firstSetTime + " oldTriggerTime=" + oldTriggerTime
                    + " alarm.windowLength=" + alarm.windowLength);
                alarm.lastSetTime = newAlarm.firstSetTime;
                alarm.when = newAlarm.when;
                alarm.whenElapsed = newAlarm.whenElapsed;
                alarm.maxWhen = newAlarm.maxWhen;
                alarm.type = newAlarm.type;
                if (!mScreenOff) {
                    if (newTriggerTime >  now + 4*60*1000) { /* interval > 4mins may be set by user??*/
                        alarm.userSetCount++;
                    }
                    return;
                } else {
                    if (newTriggerTime >  now + 4*60*1000) {
                        return;
                    }
                }
            }

            if (0 == (newTriggerTime % (60*1000))) {
                //log("This Aciton = " + newAlarm.action + " is align to minute");
                alarm.setMinsAlignCount++;
            }

            //update information
            alarm.lastSetTime = newAlarm.firstSetTime;
            alarm.when = newAlarm.when;
            alarm.whenElapsed = newAlarm.whenElapsed;
            alarm.maxWhen = newAlarm.maxWhen;
            alarm.type = newAlarm.type;

            alarm.setCount++;

            //the count have exceed the average counts
            if ((alarm.setCount > (DEFAULT_ALARM_DETECT_DURATION/mAlarmRepeatIntervalMsLevel1))
                && (alarm.userSetCount == 0) && (!alarm.packageName.contains("clock"))
                && (alarm.setMinsAlignCount < alarm.setCount/2) && (!alarm.isClockAlarm)) {

                alarm.repeatInterval = elapsed/alarm.setCount;
                /* check if a heartbeat alarm */
                if (detectHeartbeatAlarm(alarm)) {

                    log("add this non-repeat alarm to heartbeat list! before the statistics duration is end");

                    //add to heartbeat alarm ist
                    addToHeartbeatAlarmList(alarm, alarm.isRepeat, false);

                    //remove from history alarm list
                    mHistoryAlarmList.remove(alarm);
                }
            }
            return;
        }

        //update information
        alarm.lastSetTime = newAlarm.firstSetTime;
        alarm.when = newAlarm.when;
        alarm.whenElapsed = newAlarm.whenElapsed;
        alarm.maxWhen = newAlarm.maxWhen;
        alarm.type = newAlarm.type;


        /*if most of the setcouts is setMinsAlignCount , then this may be a clock alarm*/
        if ((alarm.setMinsAlignCount >=  ((3*alarm.setCount)/4)) || (alarm.isClockAlarm)) {
            log("This Action = " + alarm.action + " may be a clock alarm!!");

            //update count and firstSetTime
            alarm.firstSetTime = now;
            alarm.setCount = 1;

            alarm.lastSetTime = now;
            alarm.userSetCount = 0;

            alarm.setMinsAlignCount = (alarm.isFirstTriggerTimeAlignToMins?1:0);

            if (alarm.setCount > (detectDuration/mAlarmRepeatIntervalMsLevel1)) {
                log("This Action = " + alarm.action + " is a clock alarm!!");
                alarm.isClockAlarm = true;
            }

            return;
        }


        /* calcu the average interval */
        alarm.repeatInterval = detectDuration/alarm.setCount;

        log("update history alarm:");
        dumpAlarm(alarm);

        /* check if a heartbeat alarm */
        if (detectHeartbeatAlarm(alarm)) {

            log("add this non-repeat alarm to heartbeat list!");

            //add to heartbeat alarm ist
            addToHeartbeatAlarmList(alarm, alarm.isRepeat, false);

            //remove from history alarm list
            mHistoryAlarmList.remove(alarm);

            return;
        }

        //update count and firstSetTime
        alarm.firstSetTime = now;
        alarm.setCount = 1;

        alarm.lastSetTime = now;
        alarm.userSetCount = 0;
        alarm.setMinsAlignCount = (alarm.isFirstTriggerTimeAlignToMins?1:0);

    }



    private void processInputAlarm(AlarmInfo alarm) {

        if (alarm == null) {
            loge("null alarm!");
            return;
        }

        /*only case about WAKEUP alarm*/
        if(alarm.type != RTC_WAKEUP && alarm.type != ELAPSED_REALTIME_WAKEUP) {
            log("not a WAKEUP alarm, return!");
            return;
        }


        log("receive input alarm:");
        dumpAlarm(alarm);


        if (isSupportBlackList()) {
            /* check if an alarm from black app list */
            if (isInBlackAppList(alarm)) {
                //contained in whitelist,just return
                if (isInWhiteAppList(alarm)) {
                    log("Alarm is form WhiteApp, is not care about!");
                    return;
                }

                log("Alarm from black app");
                processBlackAppAlarm(alarm);
                return;
            }
        }

        /*check if a GMS alarm*/
        if (isGMSAlarm(alarm)) {

            log("Alarm from GMS app");

            if (processGMSAlarm(alarm)) {
                return;
            }
        }

        /*check if a ThirdParty alarm */
        if (!isThirdParty(alarm) && !alarm.isFromGms) {
            log("Alarm is not from ThirdParty, is not care about!");
            return;
        }


        /*check if in a white list*/
        if (isInWhiteAppList(alarm)) {
            log("Alarm is form WhiteApp, is not care about!");
            return;
        }

        /*check if a preset Heartbeat alarm or a saved heartbeat alarm*/
        if (isPresetHeartbeatAlarm(alarm)) {
            log("This alarm is a preset Heartbeat alarm! add it to Heartbeat list!");

            addToHeartbeatAlarmList(alarm, true, false);
            return;
        }

        /*check if a preset Heartbeat alarm or a saved heartbeat alarm*/
        if (isSavedHeartbeatAlarm(alarm)) {
            log("This alarm is a study saved Heartbeat alarm! add it to Heartbeat list!");

            addToHeartbeatAlarmList(alarm, false, true);
            return;
        }


        if ( alarm.repeatInterval == 0 ) {
            log("This alarm is a non-repeat alarm, save it to history alarm list");

            processHistoryAlarm(alarm);
            return;
        }

        /*detec if it is a heartbeat alarm */
        if (detectHeartbeatAlarm(alarm)) {
            log("This alarm is a Heartbeat alarm! add it to Heartbeat list!");

            addToHeartbeatAlarmList(alarm, true, false);
            return;
        }

    }



    private void processHistoryAlarm(AlarmInfo alarm) {

        if (alarm == null) return;

        log("History Alarm size:" + mHistoryAlarmList.size());
        for(AlarmInfo a: mHistoryAlarmList) {
            if (isAlarmMatched(a, alarm, false)) { //check packageName && (action || component)
                log("Already saved history alarm:");
                dumpAlarm(a);
                updateHistoryAlarm(a, alarm);
                return;
            }
        }

        //check the first trigger time
        long triggerTime = convertToRealTime(alarm.when, alarm.type);
        if (0 == (triggerTime % (60*1000))) {
            //log("This Aciton = " + alarm.action + " is align to minute");
            alarm.setMinsAlignCount++;
            alarm.isFirstTriggerTimeAlignToMins = true;
        }

        //can not find, add to the history alarm list.
        log("a new history alarm:");
        dumpAlarm(alarm);
        mHistoryAlarmList.add(alarm);

        //
    }


    private void processSuspectedHeartbeatAlarm(AlarmInfo alarm) {

        log("suspected heartbeat alarm:");
        dumpAlarm(alarm);
        //currently just return
        return;

    }

    private boolean detectHeartbeatAlarm(AlarmInfo alarm) {

        if (alarm.repeatInterval > mAlarmRepeatIntervalMsLevel1) {
            log("alarm interval = " + alarm.repeatInterval + ", > " + mAlarmRepeatIntervalMsLevel1 + "(ms) return!");
            return false;
        }

        if (containHeartbeatAlarmFeature(alarm)) {
            log(" alarm action = " + alarm.action + " contain Heartbeat alarm feature, it is a heartbeat alarm!");
            return true;
        }

        if (alarm.repeatInterval > mAlarmRepeatIntervalMsLevel2) {

            processSuspectedHeartbeatAlarm(alarm);

            return false;
        }

        return true;
    }



    private void processBlackAppAlarm(AlarmInfo alarm) {
        //now add it to heartbeat list in default
        log("all the alarm from black app list are added to heartbeat list. (for T5 test result!!)");
        addToHeartbeatAlarmList(alarm, alarm.isRepeat, false);
    }



    /** GMS process ***/

    private boolean isGMSAlarm(AlarmInfo alarm) {
        for(String s : mGMSAppList) {
            if(s != null  && s.equals(alarm.packageName)) {

                alarm.isFromGms = true;

                return true;
            }
        }

        return false;
    }


    private boolean  isChinaNetwork(){
        String cmccStr1 = "46000";
        String cmccStr2 = "46002";
        String cmccStr3 = "46007";
        String cmccStr4 = "46008";
        String cmccStr5 = "46004";
        String cuccstr1 = "46001";
        String cuccstr2 = "46006";
        String cuccstr3 = "46009";
        String dxstr1 = "46003";
        String dxstr2 = "46005";
        String curOper = null ;

        if(mTelephonyManager!= null) {
            curOper =mTelephonyManager.getNetworkOperator();
            log("curOper ="+curOper);
            if (curOper!=null &&(curOper.equals(cmccStr1) || curOper.equals(cmccStr2) || curOper.equals(cmccStr3)
                    ||curOper.equals(cmccStr5)||curOper.equals(cuccstr1) ||curOper.equals(cuccstr2)||curOper.equals(dxstr1)
                    ||curOper.equals(cmccStr4)||curOper.equals(cuccstr3)||curOper.equals(dxstr2))
                    ) {
                log("now is China Network!");
                return true;
            }
        }

        return false;
    }




    private boolean processGMSAlarm(AlarmInfo alarm) {

        /**check if in china network */

        if (isChinaNetwork()) {
            if (!mHasVpn && !mWifiConnected) {
                log("in china network, and vpn and wifi are both not connected!! do not applay this GMS alarm");
                alarm.isAvailable = false;

                //add to heartbeat alarm ist
                addToHeartbeatAlarmList(alarm, false, false);

                mAllGmsAlarmDisabled = true;

                return true;
            }
        }

        mAllGmsAlarmDisabled = false;

        return false;
    }



    /** Broadcast receiver ***/

    private void registerForBroadcasts() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_SCREEN_ON);
        intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        intentFilter.addAction(ACTION_VPN_ON);
        intentFilter.addAction(ACTION_VPN_OFF);
        intentFilter.addAction(Intent.ACTION_BATTERY_CHANGED);
        intentFilter.addAction("android.intent.action.setAlarmAligntTime");

        mContext.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        handleBroadcastEvent(context, intent);
                    }
                }, intentFilter);


        new PackageUpdateReceiver(mContext);
    }

    private void handleBroadcastEvent(Context context, Intent intent) {
        String action = intent.getAction();

        if (action.equals(Intent.ACTION_SCREEN_ON)) {
            mHandler.sendMessage(mHandler.obtainMessage(SCREEN_ON));
        } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
            mHandler.sendMessage(mHandler.obtainMessage(SCREEN_OFF));
        } else if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            NetworkInfo info = (NetworkInfo) intent.getParcelableExtra(
                WifiManager.EXTRA_NETWORK_INFO);
            boolean isConnected = info.isConnected();
            mHandler.sendMessage(mHandler.obtainMessage(WIFI_NETWORK_CONNECTION_CHANGE, (isConnected ? 1 : 0), 0));
        } else if (ACTION_VPN_ON.equals(action)) {
            mHandler.sendMessage(mHandler.obtainMessage(VPN_NETWORK_CONNECTION_CHANGE, 1, 0));
        } else if (ACTION_VPN_OFF.equals(action)) {
            mHandler.sendMessage(mHandler.obtainMessage(VPN_NETWORK_CONNECTION_CHANGE, 0, 0));
        } else if (action.equals(Intent.ACTION_BATTERY_CHANGED)) {
            int pluggedType = intent.getIntExtra("plugged", 0);
            mHandler.sendMessage(mHandler.obtainMessage(CMD_BATTERY_CHANGED, pluggedType, 0));
        } else if ("android.intent.action.setAlarmAligntTime".equals(action)) {
            mHandler.removeMessages(ALIGN_TIMES_CHANGED);
            int newNatOvertime = intent.getIntExtra("aligntime", 0);
            log(action+" ,newNatOvertime : " + newNatOvertime);
            mHandler.sendMessageDelayed(mHandler.obtainMessage(ALIGN_TIMES_CHANGED, newNatOvertime, 0), mDelayTime);
        }
    }


    /************* File operation ****************************/

    private boolean writeAlarmInfoToFile(AtomicFile aFile, List<PowerGuruAlarmInfo> alarmInfoList) {

        FileOutputStream stream;
        try {
            stream = aFile.startWrite();
        } catch (IOException e) {
            Slog.w(TAG, "Failed to write state: " + e);
            return false;
        }

        try {
            XmlSerializer serializer = new FastXmlSerializer();
            serializer.setOutput(stream, "utf-8");
            serializer.startDocument(null, true);
            serializer.startTag(null, XML_ALARMINFO_FILE_TAG);

            if (alarmInfoList != null) {
                for (PowerGuruAlarmInfo powerGuruAlarmInfo : alarmInfoList) {
                    serializer.startTag(null, XML_ALARMINFO_BODY_TAG);
                    //serializer.attribute(null, "id", person.getId().toString());

                    serializer.startTag(null, XML_ALARMINFO_PKG_TAG);
                    if (powerGuruAlarmInfo.getPkg() == null ) {
                        serializer.text("null");
                    } else {
                        serializer.text(powerGuruAlarmInfo.getPkg());
                    }
                    serializer.endTag(null, XML_ALARMINFO_PKG_TAG);

                    serializer.startTag(null, XML_ALARMINFO_CPN_TAG);
                    if (powerGuruAlarmInfo.getCpnt() == null ) {
                        serializer.text("null");
                    } else {
                        serializer.text(powerGuruAlarmInfo.getCpnt());
                    }
                    serializer.endTag(null, XML_ALARMINFO_CPN_TAG);

                    serializer.startTag(null, XML_ALARMINFO_ACT_TAG);
                    if (powerGuruAlarmInfo.getAction() == null ) {
                        serializer.text("null");
                    } else {
                        serializer.text(powerGuruAlarmInfo.getAction());
                    }
                    serializer.endTag(null, XML_ALARMINFO_ACT_TAG);

                    serializer.startTag(null, XML_ALARMINFO_TYPE_TAG);
                    serializer.text(Integer.toString(powerGuruAlarmInfo.getAlarmType()));
                    serializer.endTag(null, XML_ALARMINFO_TYPE_TAG);

                    serializer.startTag(null, XML_ALARMINFO_GMS_TAG);
                    serializer.text(powerGuruAlarmInfo.getFromGMS() == true ? "true" : "false");
                    serializer.endTag(null, XML_ALARMINFO_GMS_TAG);

                    serializer.startTag(null, XML_ALARMINFO_AVAILABLE_TAG);
                    serializer.text(powerGuruAlarmInfo.getAvailable() == true ? "true" : "false");
                    serializer.endTag(null, XML_ALARMINFO_AVAILABLE_TAG);

                    serializer.endTag(null, XML_ALARMINFO_BODY_TAG);
                }
            }

            serializer.endTag(null, XML_ALARMINFO_FILE_TAG);
            serializer.endDocument();
            aFile.finishWrite(stream);
        } catch (IOException e) {
            Slog.w(TAG, "Failed to write state, restoring backup.", e);
            aFile.failWrite(stream);
            return false;
        }
        return true;
    }

    private boolean writeStudiedAlarmInfoListToFile(){
        boolean ret = false;
        synchronized (mSavedHeartbeatList){
            ret = writeAlarmInfoToFile(mStudiedHeartBeatRecordFile,mSavedHeartbeatList);
        }
        return ret;
    }

    private List<PowerGuruAlarmInfo> readAlarmInfoFromFile(AtomicFile afile){
        InputStream stream = null;
        try {
            stream = afile.openRead();
        } catch (FileNotFoundException exp){
            loge(">>>file not found,"+exp);
            return new ArrayList<PowerGuruAlarmInfo>();
        }

        List<PowerGuruAlarmInfo> retList = new ArrayList<PowerGuruAlarmInfo>();

        try {
            PowerGuruAlarmInfo alarmInfoItem = null;
            XmlPullParser pullParser = Xml.newPullParser();
            pullParser.setInput(stream, "UTF-8");
            int event = pullParser.getEventType();
            while (event != XmlPullParser.END_DOCUMENT) {
                switch (event) {
                    case XmlPullParser.START_DOCUMENT:
                        //retList = new ArrayList<PowerGuruAlarmInfo>();
                        break;

                    case XmlPullParser.START_TAG:
                        if (XML_ALARMINFO_BODY_TAG.equals(pullParser.getName())) {
                            alarmInfoItem = new PowerGuruAlarmInfo();
                        }

                        if (XML_ALARMINFO_PKG_TAG.equals(pullParser.getName())) {
                            String name = pullParser.nextText();
                            if (name.equals("null")) {
                                alarmInfoItem.setPkg(null);
                            } else {
                                alarmInfoItem.setPkg(name);
                            }
                        }

                        if (XML_ALARMINFO_CPN_TAG.equals(pullParser.getName())) {
                            String cpn = pullParser.nextText();
                            if (cpn.equals("null")) {
                                alarmInfoItem.setCpnt(null);
                            } else {
                                alarmInfoItem.setCpnt(cpn);
                            }
                        }

                        if (XML_ALARMINFO_ACT_TAG.equals(pullParser.getName())) {
                            String act = pullParser.nextText();
                            if (act.equals("null")) {
                                alarmInfoItem.setAction(null);
                            } else {
                                alarmInfoItem.setAction(act);
                            }
                        }

                        if (XML_ALARMINFO_TYPE_TAG.equals(pullParser.getName())) {
                            int type = Integer.parseInt(pullParser.nextText());
                            alarmInfoItem.setAlarmType(type);
                        }

                        if (XML_ALARMINFO_GMS_TAG.equals(pullParser.getName())) {
                            boolean fromGms = pullParser.nextText().equals("true") ? true : false;
                            alarmInfoItem.setFromGMS(fromGms);
                        }

                        if (XML_ALARMINFO_AVAILABLE_TAG.equals(pullParser.getName())) {
                            boolean isAvailable = pullParser.nextText().equals("true") ? true : false;
                            alarmInfoItem.setAvailable(isAvailable);
                        }
                        break;

                    case XmlPullParser.END_TAG:
                        if(XML_ALARMINFO_BODY_TAG.equals(pullParser.getName())) {
                            retList.add(alarmInfoItem);
                            alarmInfoItem = null;
                        }
                        break;
                }
                event = pullParser.next();
            }
         } catch (IllegalStateException e) {
                loge("Failed parsing " + e);
            } catch (NullPointerException e) {
                loge("Failed parsing " + e);
            } catch (NumberFormatException e) {
                loge("Failed parsing " + e);
            } catch (XmlPullParserException e) {
                loge("Failed parsing " + e);
            } catch (IOException e) {
                loge("Failed parsing " + e);
            } catch (IndexOutOfBoundsException e) {
                loge("Failed parsing " + e);
            } finally {
                try {
                    stream.close();
                } catch (IOException e) {
                    loge("Fail to close stream " + e);
                } catch (Exception e) {
                    loge("exception at last,e: " + e);
                }
        }
        return retList;

    }


    private boolean writeAppListToFile(AtomicFile aFile, List<String> appList) {

        FileOutputStream stream;
        try {
            stream = aFile.startWrite();
        } catch (IOException e) {
            loge("Failed to write state: " + e);
            return false;
        }

        try {
            XmlSerializer serializer = new FastXmlSerializer();
            serializer.setOutput(stream, "utf-8");
            serializer.startDocument(null, true);
            serializer.startTag(null, XML_APP_LIST_FILE_TAG);

            if (appList != null) {
                for (String pkg : appList) {
                    serializer.startTag(null, XML_APP_LIST_PKG_BODY_TAG);
                    //serializer.attribute(null, "id", person.getId().toString());

                    serializer.startTag(null, XML_APP_LIST_PKG_NAME_TAG);
                    if (pkg== null ) {
                        serializer.text("null");
                    } else {
                        serializer.text(pkg);
                    }
                    serializer.endTag(null, XML_APP_LIST_PKG_NAME_TAG);

                    serializer.endTag(null, XML_APP_LIST_PKG_BODY_TAG);
                }
            }
            serializer.endTag(null, XML_APP_LIST_FILE_TAG);
            serializer.endDocument();
            aFile.finishWrite(stream);
        } catch (IOException e) {
            loge("Failed to write state, restoring backup."+"exp:"+"\n"+e);
            aFile.failWrite(stream);
            return false;
        }
        return true;
    }

    private List<String> readStringListFromFile(AtomicFile afile){

        InputStream stream = null;
        try {
            stream = afile.openRead();
        }catch (FileNotFoundException exp){
            loge(">>>file not found,"+exp);
            return new ArrayList<String>();
        }

        List<String> retList = new ArrayList<String>();

        try {
            String pkg = null;
            XmlPullParser pullParser = Xml.newPullParser();
            pullParser.setInput(stream, "UTF-8");
            int event = pullParser.getEventType();
            while (event != XmlPullParser.END_DOCUMENT) {
                switch (event) {
                    case XmlPullParser.START_DOCUMENT:
                        //retList = new ArrayList<String>();
                        break;
                    case XmlPullParser.START_TAG:
                        if (XML_APP_LIST_PKG_BODY_TAG.equals(pullParser.getName())) {
                            //alarmInfoItem = new alarmInfoItem();
                        }
                        if (XML_APP_LIST_PKG_NAME_TAG.equals(pullParser.getName())) {
                            pkg = pullParser.nextText();
                        }

                        break;
                    case XmlPullParser.END_TAG:
                        if(XML_APP_LIST_PKG_BODY_TAG.equals(pullParser.getName())) {
                            retList.add(pkg );
                            pkg = null;
                        }
                        break;
                }
                event = pullParser.next();
            }
        } catch (IllegalStateException e) {
            loge("Failed parsing " + e);
        } catch (NullPointerException e) {
            loge("Failed parsing " + e);
        } catch (NumberFormatException e) {
            loge("Failed parsing " + e);
        } catch (XmlPullParserException e) {
            loge("Failed parsing " + e);
        } catch (IOException e) {
            loge("Failed parsing " + e);
        } catch (IndexOutOfBoundsException e) {
            loge("Failed parsing " + e);
        } finally {
            try {
                stream.close();
            } catch (IOException e) {
                dumpStringList(retList);
                loge("Failed close stream " + e);
            }
        }
        return retList;
    }


    private List<PackageInfo> getThirdPartyAppsList(int flags) {
        //the original interface to get all installed apps,
        List<PackageInfo> packgeNames = mContext.getPackageManager().getInstalledPackages(flags);
        //filter the sys partion apps,get the 3rd Apps list
        Iterator<PackageInfo> packageInfos = packgeNames.iterator();
        PackageInfo info = null;
        while(packageInfos.hasNext()) {
            info = packageInfos.next();
            if(info !=null && info.applicationInfo !=null && info.applicationInfo.sourceDir!=null
                   && info.applicationInfo.sourceDir.startsWith("/system")){
               packageInfos.remove();
            }
        }
        return packgeNames;
    }

    private List<String> getThirdPartyAppsInfoList(List<PackageInfo> pkglist){
        List<String> retList = new ArrayList<String>();
        for (int i = 0; i < pkglist.size(); i++) {
            PackageInfo pak = (PackageInfo) pkglist.get(i);
            retList.add(pak.packageName);
        }
        return retList;
    }


    /** call during init, so do not need sync*/
    private void initData() {

        mPresetHeartbeatList = readAlarmInfoFromFile(mPresetHeartBeatRecordFile);

        mSavedHeartbeatList = readAlarmInfoFromFile(mStudiedHeartBeatRecordFile);

        mWhiteAppList = readStringListFromFile(mWhiteAppFile);

        //debug
        dbgLogd("White APP LIST:");
        for (String str : mWhiteAppList){
            log(str+"\n");
        }

        mCandicateWhiteList = getThirdPartyAppsInfoList(getThirdPartyAppsList(0));

        //debug
        dbgLogd("Candicate White APP LIST:");
        for (String s : mCandicateWhiteList){
            log(s);
        }

        if (isSupportBlackList()){
            mBlackAppList = readStringListFromFile(mBlackAppFile);

            //debug
            log("Black APP LIST:");
            for (String s : mBlackAppList){
                log(s);
            }
        }


    }



    private class DelayedDiskWrite {

        private HandlerThread mDiskWriteHandlerThread;
        private Handler mDiskWriteHandler;

        /* Tracks multiple writes on the same thread */
        private int mWriteSequence = 0;

        private List<String> mAppList = null;
        private List<PowerGuruAlarmInfo> mAlarmList = null;


        DelayedDiskWrite () {

        }

        public void writeAlarmList () {

            /* Make a copy */
            mAlarmList = new ArrayList<PowerGuruAlarmInfo>();

            synchronized(mSavedHeartbeatList) {
                for (PowerGuruAlarmInfo a : mSavedHeartbeatList) {
                    mAlarmList.add(new PowerGuruAlarmInfo(a.packageName, a.actionName, a.componentName,
                        a.alarmType, a.isFromGMS, a.isAvailable));
                }
            }

            /* Do a delayed write to disk on a seperate handler thread */
            synchronized (this) {
                if (++mWriteSequence == 1) {
                    mDiskWriteHandlerThread = new HandlerThread("PowerGuruServiceWriteThread");
                    mDiskWriteHandlerThread.start();
                    mDiskWriteHandler = new Handler(mDiskWriteHandlerThread.getLooper());
                }
            }

            mDiskWriteHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onWriteAlarmListCalled();
                    }
                });
        }


        public void writeAppList () {

            /* Make a copy */
            mAppList = new ArrayList<String>();

            synchronized(mWhiteAppList) {
                for (String s : mWhiteAppList) {
                    mAppList.add(new String(s));
                }
            }

            /* Do a delayed write to disk on a seperate handler thread */
            synchronized (this) {
                if (++mWriteSequence == 1) {
                    mDiskWriteHandlerThread = new HandlerThread("PowerGuruServiceWriteThread");
                    mDiskWriteHandlerThread.start();
                    mDiskWriteHandler = new Handler(mDiskWriteHandlerThread.getLooper());
                }
            }

            mDiskWriteHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onWriteAppListCalled();
                    }
                });
        }

        private void onWriteAlarmListCalled() {

            try {
                writeAlarmInfoToFile(mStudiedHeartBeatRecordFile, mAlarmList);
            } catch (Exception e) {
                loge("exception: " + e);
            } finally {
                //Quit if no more writes sent
                synchronized (this) {
                    if (--mWriteSequence == 0) {
                        mDiskWriteHandler.getLooper().quit();
                        mDiskWriteHandler = null;
                        mDiskWriteHandlerThread = null;
                    }
                }
                mAlarmList = null;
            }
        }

        private void onWriteAppListCalled() {

            try {
                writeAppListToFile(mWhiteAppFile, mAppList);
            } catch (Exception e) {
                loge("exception: " + e);
            } finally {
                //Quit if no more writes sent
                synchronized (this) {
                    if (--mWriteSequence == 0) {
                        mDiskWriteHandler.getLooper().quit();
                        mDiskWriteHandler = null;
                        mDiskWriteHandlerThread = null;
                    }
                }
                mAppList = null;
            }
        }

    }


    private void saveWhileListToDisk() {
        if (mDelayedDiskWrite != null) {
            mDelayedDiskWrite.writeAppList();
        }
    }


    private void saveStudyHeartbeatListToDisk() {
        if (mDelayedDiskWrite != null) {
            mDelayedDiskWrite.writeAlarmList();
        }
    }


    /*
    * updateType maybe add or remove in future,now it is now use
    * param:pkgList,the result list
    * param:changeType,0->removed,1->added
    */
    private void updateDataOnPackagesChanged(List<String> pkgList,int changeType,int updateReason) {
        List<String> list = pkgList;
        log("updateDataOnPackagesChanged,changetype:"+changeType);
        //for debug begin
        if (DBG){
            log("pkgList:");
            dumpStringList(pkgList);
        }
        //for debug end
        if (0 == changeType){
            if ( list != null && list.size() > 0) {
                if (updateReason == PACKAGES_CHANGE_REASON_REMOVE){
                    synchronized(mWhiteAppList) {
                        log("update mWhiteAppList");
                        if (DBG) log("before remove,size is :"+mWhiteAppList.size());
                        removeStringListItemByPkgList(list,mWhiteAppList);
                        mHandler.sendMessage(mHandler.obtainMessage(SAVE_WHITE_LIST_TO_DISK));
                        if (DBG) log("after remove,size is :"+mWhiteAppList.size());
                    }
                }

                synchronized(mHeartbeatAlarmList) {
                    log("update mHeartbeatAlarmList");
                    if (DBG) log("before remove,size is :"+mHeartbeatAlarmList.size());
                    removeAlarmInfoListItemByPkgList(pkgList,mHeartbeatAlarmList);
                    if (DBG) log("after remove,size is :"+mHeartbeatAlarmList.size());
                }

                synchronized(mCandicateWhiteList) {
                    log("update mCandicateWhiteList");
                    if (DBG) log("before remove,size is :"+mCandicateWhiteList.size());
                    removeStringListItemByPkgList(list,mCandicateWhiteList);
                    if (DBG) log("after remove,size is :"+mCandicateWhiteList.size());
                }
                synchronized(mSavedHeartbeatList) {
                    log("update mSavedHeartbeatList");
                    if (DBG) log("before remove,size is :"+mSavedHeartbeatList.size());
                    removePGAlarmListItemByPkgList(list,mSavedHeartbeatList);
                    mHandler.sendMessage(mHandler.obtainMessage(SAVE_STUDY_HEARTBEAT_LIST_TO_DISK));
                    if (DBG) log("after remove,size is :"+mSavedHeartbeatList.size());
                }
            }
        }else if (1 == changeType){
            if (updateReason != PACKAGES_CHANGE_REASON_AVAILABLE){
                synchronized(mCandicateWhiteList) {
                    log("add pkgs to mCandicateWhiteList");
                    if (DBG) log("before add,size is :"+mCandicateWhiteList.size());
                    addStringListItemByPkgList(list,mCandicateWhiteList);
                    if (DBG) log("after add,size is :"+mCandicateWhiteList.size());
                }
            } else {
                log("update mCandicateWhiteList for external app available");
                List<String> latestPkgList = getThirdPartyAppsInfoList(getThirdPartyAppsList(0));
                log("before update,mCandicateWhiteList.size = "+mCandicateWhiteList.size());
                synchronized(mCandicateWhiteList) {
                    mCandicateWhiteList = latestPkgList;
                }
                log("after update,mCandicateWhiteList.size = "+mCandicateWhiteList.size());
            }
        } else if (2 == changeType) {
            synchronized(mHeartbeatAlarmList) {
                log("update mHeartbeatAlarmList");
                if (DBG) log("before remove,size is :"+mHeartbeatAlarmList.size());
                removeAlarmInfoListItemByPkgList(pkgList,mHeartbeatAlarmList);
                if (DBG) log("after remove,size is :"+mHeartbeatAlarmList.size());
            }
            synchronized(mSavedHeartbeatList) {
                log("update mCandicateWhiteList");
                if (DBG) log("before remove,size is :"+mSavedHeartbeatList.size());
                removePGAlarmListItemByPkgList(list,mSavedHeartbeatList);
                mHandler.sendMessage(mHandler.obtainMessage(SAVE_STUDY_HEARTBEAT_LIST_TO_DISK));
                if (DBG) log("after remove,size is :"+mSavedHeartbeatList.size());
            }
        }else{
            log("invalid changeType:"+changeType);
            return;
        }
    }

    private void notifyDataChanged(){
        log("sending broadcast,notifyDataChanged");
        Intent intent = new Intent(PowerGuru.ACTION_POWERGURU_DATA_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY | Intent.FLAG_RECEIVER_FOREGROUND);
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void removeAlarmInfoListItemByPkgList(List<String> pkgList,List<AlarmInfo> alarmInfoList){
        List<String> list = pkgList;
        List<AlarmInfo> almInfoList = alarmInfoList;

        if ( list != null && list.size() > 0
            && almInfoList != null && almInfoList.size() > 0) {
            for (int i=0;i < list.size();i++){
                for (int j=0;j < almInfoList.size();j++){
                    if (list.get(i).equals((almInfoList.get(j)).packageName)){
                        almInfoList.remove(j);
                    }
                }
            }
        }
    }

    private void removePGAlarmListItemByPkgList(List<String> pkgList,List<PowerGuruAlarmInfo> alarmInfoList){
        List<String> list = pkgList;
        List<PowerGuruAlarmInfo> almInfoList = alarmInfoList;

        if ( list != null && list.size() > 0
            && almInfoList != null && almInfoList.size() > 0) {
                for (int i=0;i < list.size();i++){
                    for (int j=0;j < almInfoList.size();j++){
                        if (list.get(i).equals((almInfoList.get(j)).packageName)){
                            almInfoList.remove(j);
                        }
                    }
                }
            }
        }

    private void removeStringListItemByPkgList(List<String> pkgList,List<String> targetList){
        List<String> list = pkgList;
        List<String> tarList = targetList;

        if ( list != null && list.size() > 0
            && tarList != null && tarList.size() > 0) {
            for (String str : list) {
                tarList.remove(str);
            }
        }
    }

    private void addStringListItemByPkgList(List<String> pkgList,List<String> targetList){
        List<String> list = pkgList;
        List<String> tarList = targetList;

        for (String str : list){
            targetList.add(str);
        }
    }

    private void updateHeartBeatList(String appname){
        synchronized( mHeartbeatListLock){
            log("updateHeartBeatList");
            log("before update,size is :"+mHeartbeatAlarmList.size());
            for (int i = 0;i < mHeartbeatAlarmList.size();i++){
                if (appname.equals(mHeartbeatAlarmList.get(i).packageName)){
                    mHeartbeatAlarmList.remove(i);
                }
            }
            log("after update,size is :"+mHeartbeatAlarmList.size());
        }
    }

    //default is true
    private boolean isSupportBlackList(){
        return (1 == SystemProperties.getInt("persist.sys.powerblackapp",1));
    }


    /**
     * Determines whether the need to apply adjust acording to mplugType
     * Power source is an AC charger.
     * public static final int BATTERY_PLUGGED_AC = 1;
     * Power source is a USB port.
     * public static final int BATTERY_PLUGGED_USB = 2;
     * Power source is wireless.
     * public static final int BATTERY_PLUGGED_WIRELESS = 4;
     *
     */
    private boolean shouldApplyAlarmAdjust(int pluggedType) {
        if (pluggedType == 1 || pluggedType == 2) {  //USB or AC, do not applay adjusting
            // Never
            return false;
        } else {
            // Default
            return true;
        }
    }


    /**
     * Convert Elapsed time to real time
     */
    private long convertToRealTime(long when, int type) {

        if(type == ELAPSED_REALTIME_WAKEUP || type == ELAPSED_REALTIME){
            when += System.currentTimeMillis() - SystemClock.elapsedRealtime();
        }
        return when;
    }




    /**
     * network connection changed, if wifi change from disconnected -> connected
     * if in chinese mobile network, then need to update the gms alarm in heartbeat list.
     */
    private void netConnectionChanged() {
        boolean hasUpdate = false;

        if (mAllGmsAlarmDisabled && (mHasVpn || mWifiConnected)) {
            log("in china network, and vpn or wifi is connected!! update the GMS alarm in Heartbeat list");

            synchronized( mHeartbeatListLock){

                for (int i = 0;i < mHeartbeatAlarmList.size();){
                    AlarmInfo a = mHeartbeatAlarmList.get(i);
                    if (a.isFromGms && !a.isAvailable){
                        mHeartbeatAlarmList.remove(i);
                        hasUpdate = true;
                    } else {
                        i++;
                    }
                }
            }

            mAllGmsAlarmDisabled = false;
        }

        if (hasUpdate) {
            mHeartbeatListUpdate = true;
            mHandler.sendMessage(mHandler.obtainMessage(HEARTBEAT_ALARM_LIST_UPDATE));
        }
    }



    /*****************external interface*************/
    public void testHello(){
        Slog.d(TAG,">>>hello this is PowerGuruManagerService<<<");
    }
    //for alarm manager
    @Override
    public boolean notifyPowerguruAlarm(int type, long when,long whenElapsed, long windowLength,
            long maxWhen, long interval, PendingIntent operation) {

        try {
            AlarmInfo alarm = new AlarmInfo(type, when, whenElapsed, windowLength,
                maxWhen, interval, operation);

            mHandler.sendMessage(mHandler.obtainMessage(NEW_ALARM, alarm));
        } catch(Exception e) {
            loge("Unkown Exception:" + e);
        }

        return true;
    }

    @Override
    public List<PowerGuruAlarmInfo> getBeatList() {
        List<PowerGuruAlarmInfo> beatList = new ArrayList<PowerGuruAlarmInfo>();
        synchronized( mHeartbeatListLock){
            for (AlarmInfo item : mHeartbeatAlarmList){
                beatList.add(new PowerGuruAlarmInfo(item.packageName, item.action,
                            item.componentName, item.type, item.isFromGms, item.isAvailable));
            }
            mHeartbeatListUpdate = false;
        }
        return beatList;
    }


    //for app
    @Override
    public List<String> getWhiteList() {
        List<String> appList = new ArrayList<String>();

        synchronized(mWhiteAppList) {
            for (String s : mWhiteAppList) {
                appList.add(new String(s));
            }
        }
        log("getWhiteList,size:"+appList.size());
        return appList;
    }

    @Override
    public boolean delWhiteAppfromList(String appname) {
        log("before delWhiteAppfromList,appname:"+appname+",cur size:"+mWhiteAppList.size());
        try {
            synchronized(mWhiteAppList) {
                Iterator<String> it = mWhiteAppList.iterator();
                while (it.hasNext()){
                    String toDel = it.next();
                    if(toDel.equals(appname)) {
                        it.remove();
                    }
                }
            }
        }catch (Exception ex){
            loge(ex.toString());
            return false;
        }
        log("after delWhiteAppfromList,size of whitelist:"+mWhiteAppList.size());
        mHandler.sendMessage(mHandler.obtainMessage(SAVE_WHITE_LIST_TO_DISK));

        return true;

    }


    @Override
    public List<String> getWhiteCandicateList() {
        List<String> appList = new ArrayList<String>();

        for(String s : mCandicateWhiteList) {
                appList.add(new String(s));
        }
        log("getWhiteCandicateList,cur size:"+appList.size());
        return appList;
    }

    @Override
    public boolean addWhiteAppfromList(String appname) {
        log("before addWhiteAppfromList,appname:"+appname+",cur size:"+mWhiteAppList.size());
        if (appname == null ) return false;

        boolean found = false;
        synchronized(mWhiteAppList) {
            for(String s : mWhiteAppList) {
                if(s != null  && s.equals(appname)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                mWhiteAppList.add(appname);
            }
        }
        updateHeartBeatList(appname);
        log("after addWhiteAppfromList,size of whitelist:"+mWhiteAppList.size());

        mHandler.sendMessage(mHandler.obtainMessage(SAVE_WHITE_LIST_TO_DISK));

        return true;
    }


    /** public API ***/

    public void setAlarm(int type, long triggerAtTime, long whenElapsed, long windowLength,
            long maxWhen, long interval, PendingIntent operation) {

        try {
            AlarmInfo alarm = new AlarmInfo(type, triggerAtTime, whenElapsed, windowLength,
                maxWhen, interval, operation);

            mHandler.sendMessage(mHandler.obtainMessage(NEW_ALARM, alarm));
        } catch(Exception e) {
            loge("Unkown Exception:" + e);
        }
    }

    public void remove(PendingIntent operation) {
    }

    //for debug
    private void dbgLogd(String msg) {
        if (DBG || getDeBugPowerGuruLog()) Slog.d(TAG,msg);
    }

    /*log funciton */
    private void log(String s) {
        if (DBG || getDeBugPowerGuruLog()) Slog.d(TAG, s);
    }

    /*log funciton */
    private void loge(String s) {
        Slog.e(TAG, s);
    }

    /*log string list*/
    private void dumpStringList(List<String> list){
        log("begin dumping String List:");
        for (String str : list){
            log(str);
        }
        log("end dumping String List:");
    }

    private boolean getDeBugPowerGuruLog() {
        boolean DEBUG_POWER_GURU = SystemProperties.getBoolean("debug.power.power_guru", false);
        return DEBUG_POWER_GURU;
    }

    private boolean isDebug(){
        return Build.TYPE.equals("eng") || Build.TYPE.equals("userdebug");
    }


}
