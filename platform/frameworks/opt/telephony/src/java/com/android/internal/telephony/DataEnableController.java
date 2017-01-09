
package com.android.internal.telephony;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.TelephonyIntents;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.os.Handler;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

public class DataEnableController extends ContextWrapper {

    static final String TAG = "DataEnableController";
    static DataEnableController mInstance;
    private Context mContext;
    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    private int mDefaultDataSubId;
    final Map<Integer, DataRegisterContentObserver> mDataRegisterContentObservers = new HashMap<Integer, DataRegisterContentObserver>();

    public static DataEnableController getInstance() {
        return mInstance;
    }

    public static DataEnableController init(Context context) {
        synchronized (DataEnableController.class) {
            if (mInstance == null) {
                mInstance = new DataEnableController(context);
            } else {
                Log.wtf(TAG, "init() called multiple times!  mInstance = " + mInstance);
            }
            return mInstance;
        }
    }

    public DataEnableController(Context context) {
        super(context);
        mInstance = this;
        mContext = context;

        mSubscriptionManager = SubscriptionManager.from(mContext);
        mTelephonyManager = TelephonyManager.from(mContext);
        mDefaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        mTelephonyManager.setDataEnabled(getDataEnable());
        IntentFilter filter = new IntentFilter(
                TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED);
        mContext.registerReceiver(mReceiver, filter);
        mSubscriptionManager.addOnSubscriptionsChangedListener(mOnSubscriptionsChangeListener);
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED.equals(action)) {
                selectDataCardUpdate();
            }
        }
    };

    private final SubscriptionManager.OnSubscriptionsChangedListener mOnSubscriptionsChangeListener = new SubscriptionManager.OnSubscriptionsChangedListener() {
        @Override
        public void onSubscriptionsChanged() {
            updateRegisterContentObserver();
        }
    };

    private void updateRegisterContentObserver() {
        List<SubscriptionInfo> subscriptions = mSubscriptionManager.getActiveSubscriptionInfoList();
        if (subscriptions == null) {
            return;
        }
        HashMap<Integer, DataRegisterContentObserver> cachedContentObservers = new HashMap<Integer, DataRegisterContentObserver>(
                mDataRegisterContentObservers);
        mDataRegisterContentObservers.clear();
        final int num = subscriptions.size();
        for (int i = 0; i < num; i++) {
            int subId = subscriptions.get(i).getSubscriptionId();
            // If we have a copy of this dataRegister already reuse it, otherwise
            // make a new one.
            if (cachedContentObservers.containsKey(subId)) {
                mDataRegisterContentObservers.put(subId, cachedContentObservers.remove(subId));
            } else {
                DataRegisterContentObserver dataRegister = new DataRegisterContentObserver(mContext,
                        subId);
                mDataRegisterContentObservers.put(subId, dataRegister);
            }
        }
        for (Integer key : cachedContentObservers.keySet()) {
            cachedContentObservers.get(key).unRegisterContentObserver();
        }
    }

    private boolean getDataEnable() {
        try {
            Log.d(TAG, "MOBILE_DATA=" + Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.MOBILE_DATA + SubscriptionManager.MAX_SUBSCRIPTION_ID_VALUE));
            return Settings.Global.getInt(mContext.getContentResolver(), Settings.Global.MOBILE_DATA
                    + SubscriptionManager.MAX_SUBSCRIPTION_ID_VALUE) != 0;
        } catch (SettingNotFoundException e) {
            Settings.Global.putInt(mContext.getContentResolver(),
                    Settings.Global.MOBILE_DATA + SubscriptionManager.MAX_SUBSCRIPTION_ID_VALUE, 1);
            return true;
        }
    }

    private void selectDataCardUpdate() {
        int newSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        boolean isDataEnable = getDataEnable();
        if (newSubId != mDefaultDataSubId) {
            if (SubscriptionManager.isValidSubscriptionId(newSubId)) {
                mDefaultDataSubId = newSubId;
            }
            if (isDataEnable != mTelephonyManager.getDataEnabled(newSubId)) {
                Log.d(TAG, "setDataEnabled:" + isDataEnable);
                mTelephonyManager.setDataEnabled(newSubId, isDataEnable);
            }
        }
    }

    private void update(int subId) {
        int defaultDataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        boolean isDataEnable = mTelephonyManager.getDataEnabled(subId);
        if (SubscriptionManager.isValidSubscriptionId(defaultDataSubId) && subId == defaultDataSubId) {
            Log.d(TAG, "save default data state:" + isDataEnable);
            Settings.Global.putInt(mContext.getContentResolver(),
                    Settings.Global.MOBILE_DATA + SubscriptionManager.MAX_SUBSCRIPTION_ID_VALUE,
                    isDataEnable ? 1 : 0);
        }
    }

    public class DataRegisterContentObserver {

        Context mContext;
        int mSubId;

        public DataRegisterContentObserver(Context context, int subId) {
            mContext = context;
            mSubId = subId;
            mContext.getContentResolver().registerContentObserver(
                    Settings.Global.getUriFor(Settings.Global.MOBILE_DATA + mSubId), true,
                    mMobileDataObserver);
        }

        private ContentObserver mMobileDataObserver = new ContentObserver(new Handler()) {
            @Override
            public void onChange(boolean selfChange) {
                DataEnableController.this.update(mSubId);
            }
        };

        public void unRegisterContentObserver() {
            mContext.getContentResolver().unregisterContentObserver(mMobileDataObserver);
        }
    }
}
