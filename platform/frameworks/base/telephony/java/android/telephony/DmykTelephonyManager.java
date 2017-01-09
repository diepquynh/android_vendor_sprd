
package com.dmyk.android.telephony;

import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import com.android.internal.telephony.DctConstants;
import com.android.internal.telephony.IPhoneSubInfo;
import com.android.internal.telephony.IPhoneSubInfoEx;
import com.android.internal.telephony.ITelephony;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.ITelephonyEx;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyProperties;
import android.os.Message;
import android.os.Looper;
import android.app.ActivityThread;
import android.content.Context;
import android.content.Intent;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.database.Cursor;

import com.android.internal.R;

public class DmykTelephonyManager extends DmykAbsTelephonyManager {

    static final String TAG = "DmykTelephonyManager";
    private SubscriptionManager mSubscriptionManager;
    /** Watches for changes to the APN db. */
    private ApnChangeObserver mApnObserver;
    private Handler mHandler;
    private static final int EVENT_APN_CHANGE = 1;
    private static final String APN_URI = "content://telephony/carriers/";
    /** Watches for changes of VoLTE State. */
    private final BroadcastReceiver mVoLTEConfigReceiver = new VoLTEConfigReceiver();
    /** Watches for changes of Enhanced 4GLTE Switch State. */
    private ContentObserver mEnhancedLTEObserver;
    private ContentObserver mVoLTESettingObserver;
    private ContentObserver mVoLTESettingObserver1;
    private boolean mVolteEnable;
    private boolean mIsSettingEnhancedLTEByByUser = false;
    private boolean mIsSettingEnhancedLTEBySDK = false;

    public DmykTelephonyManager(Context context) {
        super(context);
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mHandler = new DmykHandler(context.getMainLooper());
        mApnObserver = new ApnChangeObserver();
        context.getContentResolver().registerContentObserver(
                Uri.parse(APN_URI + "preferapn"), true, mApnObserver);

        mVolteEnable = isVolteEnabledByPlatform();
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_VOLTE_STATE_SETTING);
        intentFilter.addAction(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_DONE);
        context.registerReceiver(mVoLTEConfigReceiver, intentFilter);
        Log.d(TAG, "register receiver");
        mEnhancedLTEObserver = new ContentObserver(null) {
            public void onChange(boolean selfChange) {
                onVoLTESettingChangeByUser();
            }
        };
        mVoLTESettingObserver = new ContentObserver(null) {
            public void onChange(boolean selfChange, Uri uri) {
                Log.d(TAG, "mVoLTESettingObserver uri = " + uri.toString());
                onVoLTESettingChange();
            }
        };
        mVoLTESettingObserver1 = new ContentObserver(null) {
            public void onChange(boolean selfChange, Uri uri) {
                Log.d(TAG, "mVoLTESettingObserver1 uri = " + uri.toString());
                onVoLTESettingChange1();
            }
        };
        context.getContentResolver().registerContentObserver(Settings.Global.getUriFor(
                android.provider.Settings.Global.ENHANCED_4G_MODE_ENABLED),
                true, mEnhancedLTEObserver);
        context.getContentResolver().registerContentObserver(Settings.System.getUriFor(
                android.provider.Settings.System.VOLTE_DMYK_STATE_0),
                true, mVoLTESettingObserver);
        context.getContentResolver().registerContentObserver(Settings.System.getUriFor(
                android.provider.Settings.System.VOLTE_DMYK_STATE_1),
                true, mVoLTESettingObserver1);
    }

    /** Network type is unknown */
    public static final int NETWORK_TYPE_UNKNOWN = 0;
    /** Current network is GPRS */
    public static final int NETWORK_TYPE_GPRS = 1;
    /** Current network is EDGE */
    public static final int NETWORK_TYPE_EDGE = 2;
    /** Current network is UMTS */
    public static final int NETWORK_TYPE_UMTS = 3;
    /** Current network is CDMA: Either IS95A or IS95B */
    public static final int NETWORK_TYPE_CDMA = 4;
    /** Current network is EVDO revision 0 */
    public static final int NETWORK_TYPE_EVDO_0 = 5;
    /** Current network is EVDO revision A */
    public static final int NETWORK_TYPE_EVDO_A = 6;
    /** Current network is 1xRTT */
    public static final int NETWORK_TYPE_1xRTT = 7;
    /** Current network is HSDPA */
    public static final int NETWORK_TYPE_HSDPA = 8;
    /** Current network is HSUPA */
    public static final int NETWORK_TYPE_HSUPA = 9;
    /** Current network is HSPA */
    public static final int NETWORK_TYPE_HSPA = 10;
    /** Current network is iDen */
    public static final int NETWORK_TYPE_IDEN = 11;
    /** Current network is EVDO revision B */
    public static final int NETWORK_TYPE_EVDO_B = 12;
    /** Current network is LTE */
    public static final int NETWORK_TYPE_LTE = 13;
    /** Current network is eHRPD */
    public static final int NETWORK_TYPE_EHRPD = 14;
    /** Current network is HSPA+ */
    public static final int NETWORK_TYPE_HSPAP = 15;
    /** Current network is GSM */
    public static final int NETWORK_TYPE_GSM = 16;
    /** Current network is TD_SCDMA */
    public static final int NETWORK_TYPE_TD_SCDMA = 17;
    /** Current network is IWLAN */
    public static final int NETWORK_TYPE_IWLAN = 18;
    /**
     * SPRD Add For LTE_CA. Current network is LTE_CA
     */
    public static final int NETWORK_TYPE_LTE_CA = 19;

    @Override
    public int getPhoneCount() {
        int phoneCount = 1;
        switch (getMultiSimConfiguration()) {
            case UNKNOWN:
                phoneCount = 1;
                break;
            case DSDS:
            case DSDA:
                phoneCount = PhoneConstants.MAX_PHONE_COUNT_DUAL_SIM;
                break;
            case TSTS:
                phoneCount = PhoneConstants.MAX_PHONE_COUNT_TRI_SIM;
                break;
        }
        return phoneCount;
    }

    @Override
    public String getGsmDeviceId(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }

        try {
            IPhoneSubInfo info = getSubscriberInfo();
            if (info == null)
                return null;
            return info.getDeviceIdForPhone(phoneId, mContext.getOpPackageName());
        } catch (RemoteException ex) {
            return null;
        } catch (NullPointerException ex) {
            return null;
        }
    }

    @Override
    public String getCdmaDeviceId() {
        try {
            ITelephony telephony = getITelephony();
            if (telephony == null)
                return null;
            return telephony.getDeviceId(mContext.getOpPackageName());
        } catch (RemoteException ex) {
            return null;
        } catch (NullPointerException ex) {
            return null;
        }
    }

    @Override
    public String getSubscriberId(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int subId = getSubId(phoneId);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return null;
        }
        try {
            IPhoneSubInfo info = getSubscriberInfo();
            if (info == null)
                return null;
            return info.getSubscriberIdForSubscriber(subId, mContext.getOpPackageName());
        } catch (RemoteException ex) {
            return null;
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
            return null;
        }
    }

    @Override
    public String getIccId(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int subId = getSubId(phoneId);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return null;
        }
        SubscriptionInfo sir = mSubscriptionManager.getActiveSubscriptionInfo(subId);
        if (sir != null) {
            return sir.getIccId();
        }

        return null;
    }

    @Override
    public int getDataState(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int subId = getSubId(phoneId);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return DATA_DISCONNECTED;
        }
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony == null)
                return DATA_DISCONNECTED;
            return telephony.getDataState(subId);
        } catch (RemoteException ex) {
            // the phone process is restarting.
            return DATA_DISCONNECTED;
        } catch (NullPointerException ex) {
            return DATA_DISCONNECTED;
        }
    }

    @Override
    public int getSimState(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        return SubscriptionManager.getSimStateForSlotIdx(phoneId);
    }

    @Override
    public int getNetworkType(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int subId = getSubId(phoneId);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return NETWORK_TYPE_UNKNOWN;
        }
        try {
            ITelephony telephony = getITelephony();
            if (telephony != null) {
                return telephony.getNetworkTypeForSubscriber(subId, getOpPackageName());
            } else {
                // This can happen when the ITelephony interface is not up yet.
                return NETWORK_TYPE_UNKNOWN;
            }
        } catch (RemoteException ex) {
            // This shouldn't happen in the normal case
            return NETWORK_TYPE_UNKNOWN;
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
            return NETWORK_TYPE_UNKNOWN;
        }
    }

    @Override
    public String getDeviceSoftwareVersion() {
        return SystemProperties.get("ro.build.display.id", "");
    }

    @Override
    public int getDeviceType() {
        return DEVICE_TYPE_CELLPHONE;
    }

    @Override
    public int getMasterPhoneId() {
        return SubscriptionManager.getPhoneId(SubscriptionManager.getDefaultDataSubscriptionId());
    }

    @Override
    public boolean isInternationalNetworkRoaming(int phoneId) {
        int subId = getSubId(phoneId);
        if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            return false;
        }
        try{
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return telephony.isInternationalNetworkRoaming(subId);
            } else {
                // This can happen when the ITelephony interface is not up yet.
                return false;
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
            return false;
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
            return false;
        }
    }

    public boolean isVolteEnabledByPlatform() {
        try{
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return telephony.isVolteEnabledByPlatform();
            } else {
                // This can happen when the ITelephony interface is not up yet.
                return false;
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
            return false;
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
            return false;
        }
    }

    public boolean isEnhanced4gLteModeSettingEnabledByUser() {
        try{
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                return telephony.isEnhanced4gLteModeSettingEnabledByUser();
            } else {
                // This can happen when the ITelephony interface is not up yet.
                return false;
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
            return false;
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
            return false;
        }
    }

    public void setEnhanced4gLteModeSetting(boolean enabled) {
        try{
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                telephony.setEnhanced4gLteModeSetting(enabled);
            } else {
                // This can happen when the ITelephony interface is not up yet.
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
        }
    }

    @Override
    public int getVoLTEState(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            Log.d(TAG, "phoneId is invalid  return state unknow");
            return VOLTE_STATE_UNKNOWN;
        }
        mVolteEnable = isVolteEnabledByPlatform();
        Log.d(TAG, "mVolteEnable = " + mVolteEnable);
        if (mVolteEnable && getMasterPhoneId() != SubscriptionManager.INVALID_SIM_SLOT_INDEX) {
            if (phoneId == 0) {
                return android.provider.Settings.System.getInt(
                        mContext.getContentResolver(),
                        android.provider.Settings.System.VOLTE_DMYK_STATE_0,
                        VOLTE_STATE_OFF);
            } else if (phoneId == 1) {
                return android.provider.Settings.System.getInt(
                        mContext.getContentResolver(),
                        android.provider.Settings.System.VOLTE_DMYK_STATE_1,
                        VOLTE_STATE_OFF);
            }
        }
        return VOLTE_STATE_UNKNOWN;
    }

    @Override
    public Uri getAPNContentUri(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int preferedId = -1;
        String mccmnc = getTelephonyProperty(phoneId,
                TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, "");
        Log.d(TAG, "mccmnc = " + mccmnc);
        if (mccmnc.isEmpty()) {
            return null;
        }
        int subId = getSubId(phoneId);
        Uri uri = Uri.parse(APN_URI);
        if (subId != SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
            final String orderBy = "_id";
            final String where = "numeric=\""
                + mccmnc
                + "\" AND NOT (type='ia' AND (apn=\"\" OR apn IS NULL))";

            Cursor cursor = mContext.getContentResolver().query(Uri.parse(APN_URI + "preferapn/subId/" + subId), new String[] {
                    "_id"}, where, null,orderBy);
            if (cursor != null) {
                if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                    preferedId = cursor.getInt(0);
                    Log.d(TAG, "preferedId = " + preferedId + ", sub id = " + subId);
                    if (preferedId != -1) {
                        uri =  Uri.parse(APN_URI + preferedId);
                    }
                }
                cursor.close();
            }
        }

        return uri;
    }


    @Override
    public int getSlotId(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        return phoneId;
    }

    @Override
    public int getCellId(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int cellId = -1;
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                int simState = getSimState(phoneId);
                if (simState != SIM_STATE_ABSENT && simState != SIM_STATE_UNKNOWN) {
                    Bundle bundle = telephony.getCellLocationForPhone(phoneId);
                    if (bundle != null) {
                        cellId = bundle.getInt("cid", -1);
                        return cellId;
                    }
                }
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
        }
        return cellId;
    }

    @Override
    public int getLac(int phoneId) {
        if (phoneId > getPhoneCount() - 1) {
            throw new IllegalArgumentException("phoneId is invalid" + phoneId);
        }
        int lac = -1;
        try {
            ITelephonyEx telephony = getITelephonyEx();
            if (telephony != null) {
                int simState = getSimState(phoneId);
                if (simState != SIM_STATE_ABSENT && simState != SIM_STATE_UNKNOWN) {
                    Bundle bundle = telephony.getCellLocationForPhone(phoneId);
                    if (bundle != null) {
                        lac = bundle.getInt("lac", -1);
                        return lac;
                    }
                }
            }
        } catch(RemoteException ex) {
            // This shouldn't happen in the normal case
        } catch (NullPointerException ex) {
            // This could happen before phone restarts due to crashing
        }
        return lac;
    }

    private enum MultiSimVariants {
        DSDS, DSDA, TSTS, UNKNOWN
    };

    /**
     * Returns the multi SIM variant Returns DSDS for Dual SIM Dual Standby Returns DSDA for Dual
     * SIM Dual Active Returns TSTS for Triple SIM Triple Standby Returns UNKNOWN for others
     */
    private MultiSimVariants getMultiSimConfiguration() {
        String mSimConfig = SystemProperties.get(TelephonyProperties.PROPERTY_MULTI_SIM_CONFIG);
        if (mSimConfig.equals("dsds")) {
            return MultiSimVariants.DSDS;
        } else if (mSimConfig.equals("dsda")) {
            return MultiSimVariants.DSDA;
        } else if (mSimConfig.equals("tsts")) {
            return MultiSimVariants.TSTS;
        } else {
            return MultiSimVariants.UNKNOWN;
        }
    }

    private IPhoneSubInfo getSubscriberInfo() {
        // get it each time because that process crashes a lot
        return IPhoneSubInfo.Stub.asInterface(ServiceManager.getService("iphonesubinfo"));
    }

    private ITelephony getITelephony() {
        return ITelephony.Stub.asInterface(ServiceManager.getService(Context.TELEPHONY_SERVICE));
    }

    private IPhoneSubInfoEx getSubscriberInfoEx() {
        // get it each time because that process crashes a lot
        return IPhoneSubInfoEx.Stub.asInterface(ServiceManager.getService("iphonesubinfoEx"));
    }

    private ITelephonyEx getITelephonyEx() {
        return ITelephonyEx.Stub.asInterface(ServiceManager.getService("phone_ex"));
    }

    /**
     * Gets the telephony property.
     */
    private static String getTelephonyProperty(int phoneId, String property, String defaultVal) {
        String propVal = null;
        String prop = SystemProperties.get(property);
        if ((prop != null) && (prop.length() > 0)) {
            String values[] = prop.split(",");
            if ((phoneId >= 0) && (phoneId < values.length) && (values[phoneId] != null)) {
                propVal = values[phoneId];
            }
        }
        return propVal == null ? defaultVal : propVal;
    }

    private String getOpPackageName() {
        // For legacy reasons the TelephonyManager has API for getting
        // a static instance with no context set preventing us from
        // getting the op package name. As a workaround we do a best
        // effort and get the context from the current activity thread.
        if (mContext != null) {
            return mContext.getOpPackageName();
        }
        return ActivityThread.currentOpPackageName();
    }

    private class DmykHandler extends Handler{
        public DmykHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "handleMessage = " + msg);
        }
    }
    private class ApnChangeObserver extends ContentObserver {
        public ApnChangeObserver () {
            super(mHandler);
        }

        public void onChange(boolean selfChange) {
            Uri uri =  Uri.parse(APN_URI);
            int phoneId = mSubscriptionManager.getDefaultDataPhoneId();
            if (SubscriptionManager.isValidPhoneId(phoneId)) {
                uri = getAPNContentUri(phoneId);
                Log.d(TAG, "uri " + uri );
            }
            Intent intent = new Intent(ACTION_APN_STATE_CHANGE);
            intent.setData(uri);
            mContext.sendBroadcast(intent);
        }
    }

    /**
     * Put value in Settings when VoLTE state changed .
     */
    private void putVoLTEState() {
        boolean isEnhanced4gLteMode = isEnhanced4gLteModeSettingEnabledByUser();
        int masterPhoneId = getMasterPhoneId();
        if (masterPhoneId == 0) {
            android.provider.Settings.System.putInt(
                    mContext.getContentResolver(),
                    android.provider.Settings.System.VOLTE_DMYK_STATE_0,
                    isEnhanced4gLteMode ? VOLTE_STATE_ON
                            : VOLTE_STATE_OFF);
        } else if (masterPhoneId == 1) {
            android.provider.Settings.System.putInt(
                    mContext.getContentResolver(),
                    android.provider.Settings.System.VOLTE_DMYK_STATE_1,
                    isEnhanced4gLteMode ? VOLTE_STATE_ON
                            : VOLTE_STATE_OFF);
        }
    }

    private void onVoLTESettingChangeByUser() {
        Log.d(TAG, "onVoLTESettingChangeByUser mIsSettingEnhancedLTEBySDK = " +
                mIsSettingEnhancedLTEBySDK + "mIsSettingEnhancedLTEByByUser = " +
                mIsSettingEnhancedLTEByByUser);
        if (mIsSettingEnhancedLTEBySDK == true) {
            mIsSettingEnhancedLTEBySDK = false;
        } else {
            mIsSettingEnhancedLTEByByUser = true;
        }
        putVoLTEState();
    }

    private void onVoLTESettingChange() {
        Log.d(TAG, "onVoLTESettingChange mIsSettingEnhancedLTEBySDK = " +
                mIsSettingEnhancedLTEBySDK + "mIsSettingEnhancedLTEByByUser = " +
                mIsSettingEnhancedLTEByByUser);
        if (getMasterPhoneId() == 0) {
            if (mIsSettingEnhancedLTEByByUser == false) {
                boolean enabled = android.provider.Settings.System.getInt(
                        mContext.getContentResolver(),
                        android.provider.Settings.System.VOLTE_DMYK_STATE_0,
                        VOLTE_STATE_OFF) == 1;
                mIsSettingEnhancedLTEBySDK = true;
                setEnhanced4gLteModeSetting(enabled);
            }
            Intent intent = new Intent(ACTION_VOLTE_STATE_CHANGE);
            intent.putExtra(EXTRA_SIM_PHONEID, 0);
            mContext.sendBroadcast(intent);
            mIsSettingEnhancedLTEByByUser = false;
        }
    }

    private void onVoLTESettingChange1() {
        Log.d(TAG, "onVoLTESettingChange1 mIsSettingEnhancedLTEBySDK = " +
                mIsSettingEnhancedLTEBySDK + "mIsSettingEnhancedLTEByByUser = " +
                mIsSettingEnhancedLTEByByUser);
        if (getMasterPhoneId() == 1) {
            if (mIsSettingEnhancedLTEByByUser == false) {
                boolean enabled = android.provider.Settings.System.getInt(
                        mContext.getContentResolver(),
                        android.provider.Settings.System.VOLTE_DMYK_STATE_1,
                        VOLTE_STATE_UNKNOWN) == 1;
                mIsSettingEnhancedLTEBySDK = true;
                setEnhanced4gLteModeSetting(enabled);
            }
            Intent intent = new Intent(ACTION_VOLTE_STATE_CHANGE);
            intent.putExtra(EXTRA_SIM_PHONEID, 1);
            mContext.sendBroadcast(intent);
            mIsSettingEnhancedLTEByByUser = false;
        }
    }

    /**
     * Put value in Settings when VoLTE state changed .
     */
    private class VoLTEConfigReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "receive action = " + action);
            if (ACTION_VOLTE_STATE_SETTING.equals(action)) {
                intent.setClassName("com.android.phone",
                        "com.android.phone.MobileNetworkSettings");
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivity(intent);
            } else if (TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_DONE.equals(action)) {
                int phoneId = getMasterPhoneId();
                int state = isEnhanced4gLteModeSettingEnabledByUser() ? 1 : 0;
                Log.d(TAG, "phoneId = " + phoneId + "state = " + state);
                if (getVoLTEState(phoneId) != state) {
                    mIsSettingEnhancedLTEByByUser = true;
                    putVoLTEState();
               }
            }
        }
    }

    private int getSubId(int phoneId){
        SubscriptionManager subManager = SubscriptionManager.from(mContext);
        List<SubscriptionInfo> availableSubInfoList = subManager.getActiveSubscriptionInfoList();
        if (availableSubInfoList == null) {
            return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }
        for(int i = 0 ;i < availableSubInfoList.size();i++){
            if(availableSubInfoList.get(i).getSimSlotIndex() == phoneId){
                return availableSubInfoList.get(i).getSubscriptionId();
            }
        }
        return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    }
}
