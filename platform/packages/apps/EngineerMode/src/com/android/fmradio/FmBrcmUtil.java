package com.android.fmradio;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Iterator;
import java.util.Map;
import com.android.fmradio.FmConstantsForBrcm.*;

import android.os.Debug;
import android.os.SystemProperties;
import android.util.Log;

/*
 * change fm audiofocus type
 */
public class FmBrcmUtil {

    public static final int FM_NOTIFY_UPDATE_STATION = 1;
    public static final int FM_NOTIFY_UPDATE_VOLUME = 2;
    public static final int FM_NOTIFY_SET_FREQ = 3;

    private static final boolean DEBUG = true;
    private static final String LOGTAG = "FmBrcmUtil";
    public static final int FM_STATE_CHANGE_FREQ = 1;
    public static final int FM_STATE_UI_DISABLE = 2; // not use
    public static final int FM_STATE_AUDIO_ROUTE = 3;
    public static final int FM_STATE_RECORD_START = 4;
    public static final int FM_STATE_RECORD_STOP = 5;
    public static final int FM_STATE_RECORD_PAUSE = 6;
    public static final int FM_STATE_RECORD_RESUME = 7;
    public static final int FM_STATE_RECORD_IDLE = 8;
    public static final int FM_STATE_HEADSET_PLUG = 9;
    public static final int FM_STATE_AIRPLANE_MODE_CHANGED = 10;
    public static final int FM_STATE_OFF = 11;
    public static final int FM_STATE_MUSIC_OFF = 12;
    public static final int FM_STATE_EXIT = 100;

    public static final int FM_NOTIFY_ERROR = 0;
    public static final int FM_NOTIFY_STATE = 1;
    public static final int FM_NOTIFY_RDS_DATA = 2;
    public static final int FM_NOTIFY_RDS_AF_STATE = 3;
    public static final int FM_NOTIFY_AF_JUMP = 4;

    public static final int FM_RDS_DATA_PTY_UPDATE = 2;
    public static final int FM_RDS_DATA_PS_UPDATE = 7;
    public static final int FM_RDS_DATA_PTYN_UPDATE = 8;
    public static final int FM_RDS_DATA_RT_UPDATE = 9;

    /* RDS status constants. */
    public final static int RDS_STATE_RDS_OFF = 0;
    public final static int RDS_STATE_RDS_ON = 1;
    public final static int RDS_STATE_RBDS_ON = 2;

    /* RDS RDBS type */
    /** This deactivates all RDS and RDBS functionality. */
    public static final int RDS_MODE_OFF = 0;
    /** This activates RDS or RDBS as appropriate. */
    public static final int RDS_MODE_DEFAULT_ON = 1;
    /** This activates RDS. */
    public static final int RDS_MODE_RDS_ON = 2;
    /** This activates RDBS. */
    public static final int RDS_MODE_RBDS_ON = 3;

    /* AF Modes. */
    /** Disables AF capability. */
    public static final int AF_MODE_OFF = 0;
    /** Enables AF capability. */
    public static final int AF_MODE_ON = 1;
    public int mRdsDataType = FM_RDS_DATA_PTY_UPDATE;

    //begin 562936 add by suyan.yang 2016.05.19
    public static int bler = -1;
    //end 562936 add by suyan.yang 2016.05.19

    private static String universeSupportKey = "universe_ui_support";
    public static final boolean UNIVERSEUI_SUPPORT = SystemProperties.getBoolean(
            universeSupportKey, false);
    public static final boolean surpport50ksearch = "1".equals(SystemProperties.get(
                "persist.surpport.50ksearch", "0"));
    public static ConcurrentHashMap<String, FMStateChangeListener> fmStateChangeListenerMaps;

    private static FmBrcmUtil util;

    private FmBrcmUtil() {

    }

    public static FmBrcmUtil getUtil() {
        if (null == util) {
            return new FmBrcmUtil();
        }
        return util;
    }

    public FMStateChangeListener registerfmStateChangeListener(
            FMStateChangeListener callback) {
        if (null == fmStateChangeListenerMaps) {
            fmStateChangeListenerMaps = new ConcurrentHashMap<String, FMStateChangeListener>();
        }
        if (null != callback) {
            return fmStateChangeListenerMaps.put(callback.toString(), callback);
        } else {
            return callback;
        }
    }

    public FMStateChangeListener unregisterfmStateChangeListener(
            FMStateChangeListener callback) {
        if (null == callback) {
            return null;
        }
        if (null == fmStateChangeListenerMaps) {
            return null;
        } else {
            if (fmStateChangeListenerMaps.containsKey(callback.toString())) {
                return fmStateChangeListenerMaps.remove(callback.toString());
            } else {
                return null;
            }
        }
    }

    public int fmSignal2Info(int type, int state, String info) {
        try {
            if (DEBUG) Log.d(LOGTAG, "fmSignal2Info type:" + type + ",state:"
                    + state + ",info:" + info);
            if (null != fmStateChangeListenerMaps
                    && fmStateChangeListenerMaps.size() >= 1) {
                Iterator<String> it = fmStateChangeListenerMaps.keySet().iterator();
                switch (type) {
                    case FM_NOTIFY_STATE:
                        while (it.hasNext()) {
                            String key = it.next();
                            fmStateChangeListenerMaps.get(key).onStateChange(state,
                                    info, key);
                        }
                        break;
                    case FM_NOTIFY_RDS_DATA:
                        while (it.hasNext()) {
                            String key = it.next();
                            if (mRdsDataType == FM_RDS_DATA_PTY_UPDATE) {
                                info = FmConstantsForBrcm.PTY_LIST[state];
                            }
                            fmStateChangeListenerMaps.get(key)
                                    .onRdsDataUpdate(mRdsDataType, info, key);
                        }
                        break;

                    case FM_NOTIFY_RDS_AF_STATE:
                        while (it.hasNext()) {
                            String key = it.next();
                            fmStateChangeListenerMaps.get(key)
                                    .onRdsAFStateUpdate(state, info, key);
                        }
                        break;
                    case FM_NOTIFY_AF_JUMP:
                        while (it.hasNext()) {
                            String key = it.next();
                            fmStateChangeListenerMaps.get(key).onAfJumpChange(state, info, key);
                        }
                        break;
                    case FM_NOTIFY_ERROR:
                        while (it.hasNext()) {
                            String key = it.next();
                            fmStateChangeListenerMaps.get(key)
                                    .onError(state, info, key);
                        }
                        break;
                    default:
                        break;
                }
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception:"+e);
        }
        return 0;
    }
    public void setRdsDataType(int rdsDataType){
        mRdsDataType = rdsDataType;
    }

    //begin 562936 add by suyan.yang 2016.05.19
    public void setRdsBler(int bler_value){
        bler=bler_value;
    }
    //end 562936 add by suyan.yang 2016.05.19

    public static String changeFreqString(String freq){
       String newFreq = freq;
       String[] arrayFreq = freq.split("\\.");
       if( arrayFreq.length>1 && arrayFreq[1]!= null && arrayFreq[1].length()<2){
           newFreq =  freq+"0";
       }
       if (DEBUG) Log.d(LOGTAG, "changeFreqString newFreq  :" + newFreq );
       return newFreq;
    }

    interface FMStateChangeListener {
        public int onError(int state, String info, String to);

        public int onStateChange(int state, String info, String to);

        public int onRdsDataUpdate(int state, String info, String to);

        public int onRdsAFStateUpdate(int state, String info, String to);
        public int onAfJumpChange(int state, String info, String to);

    }
}
