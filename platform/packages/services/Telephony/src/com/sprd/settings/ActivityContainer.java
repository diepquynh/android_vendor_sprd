package com.sprd.settings;

import android.app.Activity;
import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.TelephonyIntents;

import java.util.concurrent.CopyOnWriteArrayList;

public class ActivityContainer extends Application {
    private static final String LOG_TAG = "ActivityContainer";
    private CopyOnWriteArrayList<Activity> mActivityList = new CopyOnWriteArrayList<Activity>();
    private ArrayMap<Activity, Integer> mActivityArrayMap = new ArrayMap<Activity, Integer>();
    private static ActivityContainer sInstance;
    private Application mApplication;
    private TelephonyManager mTeleMgr;
    private static final int NO_DECIDE_BY_PHONEID = -1;
    private boolean mIsFirstListen = true;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(LOG_TAG, "mIsFirstListen : " + mIsFirstListen);
            if ((TelephonyIntents.ACTION_SIM_STATE_CHANGED).equals(action) && !mIsFirstListen) {
                String state = intent
                        .getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                Log.d(LOG_TAG, "state : " + state);
                if ((IccCardConstants.INTENT_VALUE_ICC_ABSENT).equals(state)) {
                    Log.d(LOG_TAG, "exit");
                    exit();
                }
            }
            mIsFirstListen = false;
        }
    };

    private ActivityContainer() {
    }

    public synchronized static ActivityContainer getInstance() {
        if (sInstance == null) {
            sInstance = new ActivityContainer();
        }
        return sInstance;
    }

    public void setApplication(Application application) {
        if (mApplication == null) {
            Log.d(LOG_TAG, "set Application");
            mApplication = application;
            IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
            if (mApplication != null) {
                mTeleMgr = getTeleMgr();
                mApplication.registerReceiver(mReceiver, intentFilter);
            }
        }
    }

    public void addActivity(Activity activity, int phoneId) {
        if (!mActivityList.contains(activity)) {
            Log.d(LOG_TAG, "add activity:" + activity + " phoneId: " + phoneId);
            mActivityArrayMap.put(activity, phoneId);
            mActivityList.add(activity);
        }
    }

    public void removeActivity(Activity activity) {
        if (mActivityList.contains(activity)) {
            Log.d(LOG_TAG, "remove activity:" + activity);
            mActivityArrayMap.remove(activity);
            mActivityList.remove(activity);
            if (mActivityList.size() == 0) {
                Log.d(LOG_TAG, "mActivityList.size() =0 ");
                if (mApplication != null) {
                    mApplication.unregisterReceiver(mReceiver);
                }
                sInstance = null;
            }
        }
    }

    public void exit() {
        Log.d(LOG_TAG, "exit()");
        for (Activity activity : mActivityList) {
            if (activity != null && mActivityArrayMap != null) {
                Log.d(LOG_TAG, "mActivityArrayMap :" + mActivityArrayMap.toString());
                int phoneId = mActivityArrayMap.get(activity) == null ?
                        NO_DECIDE_BY_PHONEID : mActivityArrayMap.get(activity);
                if (isDoneExitByPhoneId(phoneId)) {
                    Log.d(LOG_TAG, "finish activity : " + activity.toString());
                    mActivityArrayMap.remove(activity);
                    mActivityList.remove(activity);
                    activity.finish();
                }
            }
        }
        if (mActivityList.size() == 0) {
            Log.d(LOG_TAG, "mActivityList.size() =0 ");
            if (mApplication != null) {
                mApplication.unregisterReceiver(mReceiver);
            }
            sInstance = null;
        }
    }

    private boolean isDoneExitByPhoneId(int phoneId) {
        if (phoneId == NO_DECIDE_BY_PHONEID) {
            return true;
        } else {
            if (mTeleMgr == null && mApplication != null) {
                mTeleMgr = getTeleMgr();
            }
            int simState = mTeleMgr.getSimState(phoneId);
            return (simState == TelephonyManager.SIM_STATE_ABSENT);
        }
    }

    private TelephonyManager getTeleMgr() {
        return (TelephonyManager) mApplication.
                    getSystemService(Context.TELEPHONY_SERVICE);
    }
}
