/*This code functions as follows:
 * 1, real-time monitoring of changes in the type of network
 * 2, to identify the type of network
 */
package com.sprd.engineermode.telephony;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.os.Handler;
import android.os.HandlerThread;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.preference.PreferenceCategory;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.preference.PreferenceActivity;
import android.provider.Settings;
import android.util.Log;
import android.telephony.ServiceState;
import android.telephony.PhoneStateListener;
import android.os.SystemProperties;
import android.provider.Settings;
import android.content.BroadcastReceiver;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

public class NetInfoSimNetworkTypeActivity extends PreferenceActivity {

    private static final String TAG = "NetInfoSimNetworkTypeActivity";
    private static final String KEY_SIM_INDEX = "simindex";
    private static final String PREF_INDEX = "PrefenceIndex";
    private static final String LTE_INDEX = "LteIndex";
    private static final String NETWORK_TYPE = "NetWorkType";
    private static final String KEY_TDD_SVLTE = "TDD_SVLTE";
    private static final String KEY_FDD_CSFB = "FDD_CSFB";
    private static final String KEY_TDD_CSFB = "TDD_CSFB";
    private static final String NETWORK_STAT_CHANGE = "com.sprd.network.NETWORK_STAT_CHANGE";

    private static final int NETWORK_UNKNOW = 0;
    private static final int NETWORK_GSM = 1;
    private static final int NETWORK_TDSCDMA = 2;
    private static final int NETWORK_WCDMA = 3;
    private static final int NETWORK_LTE = 4;

    /**
     * SPRD Add For LTE_CA 4G+ Supports.(Bug 570270)
     * TelephonyManager.NETWORK_TYPE_LTE_CA is added on SPRD_Trunk,
     * but Engineermode app is shared with Trunk and Prime,
     * so needs to be defined locally.
     */
    private static final int NETWORK_TYPE_LTE_CA = 19;

    private static final int LTE_CHANGE = 0;

    static final boolean DEBUG = true;
    private Preference[] mSIMPref;
    private Preference[] mLtePref;
    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private PhoneStateListener mPhoneStateListener;
    private PreferenceCategory mPreferenceCategory;
    private PreferenceCategory mLtePreferenceCategory;
    private PreferenceGroup mPreGroup = null;
    private ServiceState mServiceState;
    private Handler myHandler;
    private boolean mDataConnected;
    private boolean mHspaDataDistinguishable;
    private boolean isLTEReady = false;
    private boolean mAirplaneMode = false;
    private boolean isPreferenEnable = true;
    private boolean mListening = false;

    private int mDataNetType;
    private int mDataState;
    private int mSimIndex;
    private int mNetWorkType = NETWORK_UNKNOW;
    private int mPrefenceIndex;

    private int[] PrefenceName = new int[] { R.string.netinfo_server_cell,
            R.string.netinfo_neighbour_cell, R.string.netinfo_adjacent_cell_3g,
            R.string.netinfo_adjacent_cell_4g,
            R.string.netinfo_outfield_information };

    private int[] LtefereName = new int[] { R.string.netinfo_server_cell,
            R.string.netinfo_neighbour_cell, R.string.netinfo_adjacent_cell_2g,
            R.string.netinfo_adjacent_cell_3g,
            R.string.netinfo_outfield_information };

    // Detect whether the phone supports 4G network
    private boolean isSupportSVLTE = TelephonyManagerSprd.RadioCapbility.TDD_SVLTE.equals(TelephonyManagerSprd.getRadioCapbility());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        SharedPreferences pref = PreferenceManager
                .getDefaultSharedPreferences(this);
        NetTypeDisplay();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Intent intent = this.getIntent();
        mSimIndex = intent.getIntExtra(KEY_SIM_INDEX, -1);
        if (mSimIndex != -1) {
            setTitle("SIM" + mSimIndex);
        } else {
            finish();
        }
        Log.d(TAG, "onResume SimIndex=" + mSimIndex);
        mServiceState = new ServiceState();
        mTelephonyManager = (TelephonyManager) TelephonyManager
                .from(NetInfoSimNetworkTypeActivity.this);
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(NetInfoSimNetworkTypeActivity.this);
        mPhoneStateListener = getPhoneStateListener(mSimIndex);
        listenForPhoneState(true);
        mDataConnected = false;
        mDataNetType = TelephonyManager.NETWORK_TYPE_UNKNOWN;
        mDataState = TelephonyManager.DATA_DISCONNECTED;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG,"onDestroy");
        listenForPhoneState(false);
    }

    void listenForPhoneState(boolean start) {
        if (start) {
            if (!mListening) {
                if (mPhoneStateListener != null) {
                    Log.d(TAG,"start listen");
                    mTelephonyManager
                            .listen(mPhoneStateListener,
                                    PhoneStateListener.LISTEN_SERVICE_STATE
                                            | PhoneStateListener.LISTEN_DATA_CONNECTION_STATE
                                            | PhoneStateListener.LISTEN_DATA_ACTIVITY);
                    mListening = true;
                }
            }
        } else {
            if (mPhoneStateListener != null) {
                Log.d(TAG,"stop listen");
                mTelephonyManager.listen(mPhoneStateListener,
                        PhoneStateListener.LISTEN_NONE);
                mListening = false;
            }
        }
    }

    // The first interface display function
    // if it has 4G network,it will display "Lte",or it will not display
    private void NetTypeDisplay() {
        mPreferenceCategory = new PreferenceCategory(this);
        mPreGroup.addPreference(mPreferenceCategory);
        mSIMPref = new Preference[5];
        for (int i = 0; i < 5; i++) {
            mSIMPref[i] = new Preference(this);
            mSIMPref[i].setTitle(PrefenceName[i]);
            mSIMPref[i].setKey(PREF_INDEX + i);
            mPreGroup.addPreference(mSIMPref[i]);
        }
    }

    // phoneId get subid
    public int slotIdToSubId(int phoneId) {
       int subId;
       SubscriptionInfo mSubscriptionInfo = mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
       if (mSubscriptionInfo == null) {
           Log.d(TAG,"mSubscriptionInfo is null");
           subId = SubscriptionManager.getDefaultSubscriptionId();
       } else {
           subId = mSubscriptionInfo.getSubscriptionId();
       }
       return subId;
    }

    private PhoneStateListener getPhoneStateListener(final int phoneId) {
        int subId = slotIdToSubId(phoneId);
        PhoneStateListener phoneStateListener = new PhoneStateListener(subId) {
            @Override
            public void onServiceStateChanged(ServiceState state) {
                if (DEBUG) {
                    Log.d(TAG, "onServiceStateChanged on phoneId" + phoneId
                            + "state=" + state.getState());
                }
                mServiceState = state;
                mDataNetType = mServiceState.getNetworkType();
                Log.d(TAG, "mDataNetType[" + phoneId + "]=" + mDataNetType);
                updateDataNetType(phoneId);
            }

            @Override
            public void onDataConnectionStateChanged(int state, int networkType) {
                if (DEBUG) {
                    Log.d(TAG, "onDataConnectionStateChanged: state=" + state
                            + " type=" + networkType + " phoneId=" + phoneId);
                }
                if (!isSupportSVLTE) {
                    if (networkType == TelephonyManager.NETWORK_TYPE_UNKNOWN) {
                        mDataNetType = mServiceState.getNetworkType();
                    } else {
                        mDataNetType = networkType;
                    }
                } else {
                    mDataNetType = networkType;
                }
                updateDataNetType(phoneId);
            }

        };
        return phoneStateListener;
    }

    private final void updateDataNetType(int phoneId) {

        /* SPRD: modify by bug426493 @{ */
        int netType = TelephonyManager.NETWORK_TYPE_UNKNOWN;
        int[] subId = SubscriptionManager.getSubId(phoneId);
        if (subId != null) {
            if (DEBUG) {
                Log.d(TAG, "updateDataNetType phoneId = " + phoneId + ", subId = " + subId[0]);
            }
            if (SubscriptionManager.isValidSubscriptionId(subId[0])) {
               netType = mTelephonyManager.getVoiceNetworkType(subId[0]);
            }

        }
        if (DEBUG) {
            Log.d(TAG, "netType = " + netType + ", typeName = " + mTelephonyManager
                    .getNetworkTypeName(netType));
        }
        //switch (mDataNetType) {
        switch(netType) {
        /* @} */
        case TelephonyManager.NETWORK_TYPE_EDGE:
        case TelephonyManager.NETWORK_TYPE_GPRS:
        case TelephonyManager.NETWORK_TYPE_CDMA:
        case TelephonyManager.NETWORK_TYPE_1xRTT:
        case TelephonyManager.NETWORK_TYPE_IDEN:
        case TelephonyManager.NETWORK_TYPE_GSM:
            mPreferenceCategory.setTitle("GSM");
            mSIMPref[2].setTitle(R.string.netinfo_adjacent_cell_3g);
            mSIMPref[3].setTitle(R.string.netinfo_adjacent_cell_4g);
            mNetWorkType = NETWORK_GSM;
            Log.d(TAG, "Networktype is GSM");
            for (int i = 0; i < 5; i++) {
                mSIMPref[i].setEnabled(true);
            }
            break;
        case TelephonyManager.NETWORK_TYPE_UMTS:
        case TelephonyManager.NETWORK_TYPE_HSDPA:
        case TelephonyManager.NETWORK_TYPE_HSUPA:
        case TelephonyManager.NETWORK_TYPE_HSPA:
        case TelephonyManager.NETWORK_TYPE_HSPAP:
        case TelephonyManager.NETWORK_TYPE_EHRPD:
        case TelephonyManager.NETWORK_TYPE_EVDO_0:
        case TelephonyManager.NETWORK_TYPE_EVDO_A:
        case TelephonyManager.NETWORK_TYPE_EVDO_B:
            mNetWorkType = check3GNetWork(mSimIndex);
            if (mNetWorkType == NETWORK_TDSCDMA) {
                mPreferenceCategory.setTitle("TD-SCDMA");
                mSIMPref[2].setTitle(R.string.netinfo_adjacent_cell_2g);
                mSIMPref[3].setTitle(R.string.netinfo_adjacent_cell_4g);
                Log.d(TAG, "Networktype is TDSCDMA");
            }
            if (mNetWorkType == NETWORK_WCDMA) {
                mPreferenceCategory.setTitle("WCDMA");
                mSIMPref[2].setTitle(R.string.netinfo_adjacent_cell_2g);
                mSIMPref[3].setTitle(R.string.netinfo_adjacent_cell_4g);
                Log.d(TAG, "Networktype is WCDMA");
            }
            for (int i = 0; i < 5; i++) {
                mSIMPref[i].setEnabled(true);
            }
            break;
        case TelephonyManager.NETWORK_TYPE_LTE:
        case NETWORK_TYPE_LTE_CA: // SRPD Add to Support LTE_CA.
            for (int i = 0; i < 5; i++) {
                mSIMPref[i].setEnabled(true);
            }
            mPreferenceCategory.setTitle("LTE");
            mNetWorkType = NETWORK_LTE;
            mSIMPref[2].setTitle(R.string.netinfo_adjacent_cell_2g);
            mSIMPref[3].setTitle(R.string.netinfo_adjacent_cell_3g);
            Log.d(TAG, "Networktype is LTE");
            break;
        default:
            mNetWorkType = NETWORK_UNKNOW;
            mPreferenceCategory.setTitle(R.string.unknow_server);
            Log.d(TAG, "modem type is UnKnow");
            for (int i = 0; i < 5; i++) {
                mSIMPref[i].setEnabled(false);
            }
            break;
        }
        sendNetWorkStatBroadcast(mNetWorkType,phoneId);
    }

 // Identify the 3G network type (Mobile and China Unicom)
    private int check3GNetWork(int mSimIndex) {
        String mServerName = "atchannel" + mSimIndex;
        String mATcmd = engconstents.ENG_AT_COPS;
        String mStrTmp = IATUtils.sendATCmd(mATcmd, mServerName);
        if (mStrTmp.contains(IATUtils.AT_OK)) {
            Log.d(TAG, mStrTmp);
            String[] strs = mStrTmp.split("\n");
            Log.d(TAG, strs[0]);
            if (strs[0].contains("46000") || strs[0].contains("46004") || strs[0].contains("00100")) {
                return NETWORK_TDSCDMA;
            } else {
                return NETWORK_WCDMA;
            }
        }
        return NETWORK_WCDMA;
    }

    private void sendNetWorkStatBroadcast(int netWorkType, int phoneId) {
        Log.d(TAG, "sendNetWorkStatBroadcast : " + netWorkType);
        Intent intent = new Intent(NETWORK_STAT_CHANGE);
        intent.putExtra(NETWORK_TYPE, netWorkType);
        intent.putExtra(KEY_SIM_INDEX, phoneId);
        sendBroadcast(intent);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();
        mPrefenceIndex = Integer.valueOf(key.substring(13, 14));
        int mSubId = slotIdToSubId(mSimIndex);
        Log.d(TAG, "PrefIndex: " + mPrefenceIndex + KEY_SIM_INDEX + ": "
                + mSimIndex + "SubId: " + mSubId);
        Bundle data = new Bundle();
        data.putInt("PrefenceIndex", mPrefenceIndex);
        data.putInt(KEY_SIM_INDEX, mSimIndex);
        data.putInt("SubId", mSubId);
        switch (mNetWorkType) {
        case NETWORK_GSM:
            Intent intent1 = new Intent("android.intent.action.GSMSHOW");
            intent1.putExtras(data);
            startActivity(intent1);
            break;
        case NETWORK_TDSCDMA:
            Intent intent2 = new Intent("android.intent.action.TDSCDMASHOW");
            intent2.putExtras(data);
            startActivity(intent2);
            break;
        case NETWORK_WCDMA:
            Intent intent3 = new Intent("android.intent.action.WCDMASHOW");
            intent3.putExtras(data);
            startActivity(intent3);
            break;
        case NETWORK_LTE:
            Intent intent4 = new Intent("android.intent.action.LTESHOW");
            intent4.putExtras(data);
            startActivity(intent4);
            break;
        default:
            break;
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }
}
