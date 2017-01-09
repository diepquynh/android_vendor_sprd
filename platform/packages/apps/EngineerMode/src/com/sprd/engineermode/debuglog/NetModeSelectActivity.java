package com.sprd.engineermode.debuglog;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;
import android.provider.Settings;
import android.content.Context;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

public class NetModeSelectActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "NetModeSelectActivity";
    static final String PROPERTY_MULTI_SIM_CONFIG = "persist.radio.multisim.config";
    private static final String KEY = "sim";

    private static final int SET_NETMODE = 1;
    private static final int GET_NETMODE = 2;

    private int mPhoneCount;
    private int mLastValue;
    private ListPreference[] mListPreference;
    private SharedPreferences mSharePref;
    private NMODEHandler mNMODEHandler;
    private Context mContext;
    private NetModeSelectHelper[] mSelectHelper = null;
    private SubscriptionManager mSubscriptionManager;
    private Handler mUiThread = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case SET_NETMODE: {
                int sim = msg.arg1;
                int isSuccess = msg.arg2;

                if (isSuccess == 1) {
                    if (mSelectHelper[sim].getModemType() == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                        mListPreference[sim].setSummary(mListPreference[sim]
                                .getEntry());
                        finish();
                        Toast.makeText(NetModeSelectActivity.this,
                                "success,modem reset...", Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        mListPreference[sim].setSummary(mListPreference[sim]
                                .getEntry());
                        Toast.makeText(NetModeSelectActivity.this, "success",
                                Toast.LENGTH_SHORT).show();
                    }
                } else {
                    String backupString = (String) msg.obj;
                    mListPreference[sim].setValue(backupString);
                    mListPreference[sim].setSummary(mListPreference[sim]
                            .getEntry());
                    SharedPreferences.Editor edit = mSharePref.edit();
                    edit.putString(KEY + sim, backupString);
                    edit.commit();
                    Toast.makeText(NetModeSelectActivity.this, "fail",
                            Toast.LENGTH_SHORT).show();
                }

            }
                break;
            case GET_NETMODE: {
                int sim = msg.arg1;
                mLastValue = msg.arg2;
                mListPreference[sim].setValue(String.valueOf(mLastValue));
                mListPreference[sim]
                        .setSummary(mListPreference[sim].getEntry());
            }
                break;
            }
        }
    };

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mNMODEHandler = new NMODEHandler(ht.getLooper());

        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        Log.d(TAG, "mPhoneCount is " + mPhoneCount);

        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        PreferenceGroup preGroup = getPreferenceScreen();
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager.from(NetModeSelectActivity.this);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mListPreference = new ListPreference[mPhoneCount];
        mSelectHelper = new NetModeSelectHelper[mPhoneCount];

        for (int i = 0; i < mPhoneCount; i++) {
            String key = KEY + i;
            mListPreference[i] = new ListPreference(NetModeSelectActivity.this);
            mListPreference[i].setEnabled(false);
            mListPreference[i].setTitle(key);
            mListPreference[i].setKey(key);
            mListPreference[i]
                    .setOnPreferenceChangeListener(NetModeSelectActivity.this);
            preGroup.addPreference(mListPreference[i]);
            mListPreference[i].setEntries(R.array.network_mode_choices_gsm);
            mListPreference[i].setEntryValues(R.array.network_mode_gsm_values);
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < mPhoneCount; i++) {
                    mSelectHelper[i] = NetModeSelectHelper
                            .getNetModeSelectHelper(i,mContext);
                }

                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        TelephonyManager tm = null;
                        for (int i = 0; i < mPhoneCount; i++) {
                            tm = TelephonyManager
                                    .from(NetModeSelectActivity.this);
                            if (mSelectHelper[i] != null) {
                                if (tm != null
                                        && tm.getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                                    mListPreference[i].setEnabled(true);
                                } else {
                                    mListPreference[i].setEnabled(false);
                                    continue;
                                }

                                if (mSelectHelper[i].getModemType() == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                                    mListPreference[i]
                                            .setEntries(R.array.network_mode_choices_td);
                                    mListPreference[i]
                                            .setEntryValues(R.array.network_mode_td_values);
                                } else if (mSelectHelper[i].getModemType() == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                                    mListPreference[i]
                                            .setEntries(R.array.network_mode_choices_wcdma);
                                    mListPreference[i]
                                            .setEntryValues(R.array.network_mode_wcdma_values);
                                } else {
                                    mListPreference[i]
                                            .setEntries(R.array.network_mode_choices_gsm);
                                    mListPreference[i]
                                            .setEntryValues(R.array.network_mode_gsm_values);
                                }

                                if (mListPreference[i].isEnabled()) {
                                    Log.d(TAG, "sim" + i + "is enable");
                                    Message netMode = mNMODEHandler
                                            .obtainMessage(GET_NETMODE, i, 0);
                                    mNMODEHandler.sendMessage(netMode);
                                }
                            }
                        }
                    }
                });
            }
        }).start();
    }

    @Override
    protected void onDestroy() {
        if (mNMODEHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mNMODEHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        Log.d(TAG, "onPreferenceChange and the key is " + key);
        for (int i = 0; i < mPhoneCount; i++) {
            if (key.equals(KEY + i)) {
                String re = newValue.toString();
                Message mNetMode = mNMODEHandler.obtainMessage(SET_NETMODE, i,
                        0, re);
                mNMODEHandler.sendMessage(mNetMode);
            }
        }
        return true;
    }

    /*
     * Modify 324468 & 342378 by sprd When Setting Net Mode in TD/W,
     * EngineerMode need to set Setting Provider after writting NV or sending AT
     * Success
     */
    private boolean refreshSettingProvider(int sim, int value) {
        boolean hasRefreshed = false;
        int subId = slotIdToSubId(sim);
        switch (value) {
            case NetModeSelectHelper.NETMODE_GSMONLY:
                Settings.Global
                        .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId), 1);
                try {
                    int newvalue = Settings.Global
                            .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                    subId));
                    if (newvalue == 1) {
                        hasRefreshed = true;
                    } else {
                        hasRefreshed = false;
                    }
                    Log.d(TAG, "NetMode Set GSM Only and Refresh Setting Provider " + newvalue);
                } catch (Settings.SettingNotFoundException e) {
                    Log.d(TAG, "catch exception");
                    hasRefreshed = false;
                }
                break;
        case NetModeSelectHelper.NETMODE_TDONLY:
            Settings.Global
                    .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                            subId), 2);
            try {
                int newvalue = Settings.Global
                        .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId));
                if (newvalue == 2) {
                    hasRefreshed = true;
                } else {
                    hasRefreshed = false;
                }
                Log.d(TAG, "NetMode Set TD Only and Refresh Setting Provider " + newvalue);
            } catch (Settings.SettingNotFoundException e) {
                Log.d(TAG, "catch exception");
                hasRefreshed = false;
            }
            break;
        case NetModeSelectHelper.NETMODE_TDPREFER:
            Settings.Global
                    .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                            subId), 0);
            try {
                int newvalue = Settings.Global
                        .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId));
                if (newvalue == 0) {
                    hasRefreshed = true;
                } else {
                    hasRefreshed = false;
                }
                Log.d(TAG, "NetMode Set TD Prefered and Refresh Setting Provider " + newvalue);
            } catch (Settings.SettingNotFoundException e) {
                Log.d(TAG, "catch exception");
                hasRefreshed = false;
            }
            break;
        case NetModeSelectHelper.NETMODE_WCDMAONLY:
            Settings.Global
                    .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                            subId), 2);
            try {
                int newvalue = Settings.Global
                        .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId));
                if (newvalue == 2) {
                    hasRefreshed = true;
                } else {
                    hasRefreshed = false;
                }
                Log.d(TAG, "NetMode Set WCDMA Only and Refresh Setting Provider " + newvalue);
            } catch (Settings.SettingNotFoundException e) {
                Log.d(TAG, "catch exception");
                hasRefreshed = false;
            }
            break;
        case NetModeSelectHelper.NETMODE_WCDMAPREFER:
            Settings.Global
                    .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                            subId), 0);
            try {
                int newvalue = Settings.Global
                        .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId));
                if (newvalue == 0) {
                    hasRefreshed = true;
                } else {
                    hasRefreshed = false;
                }
                Log.d(TAG, "NetMode Set WCDMA Prefered and Refresh Setting Provider " + newvalue);
            } catch (Settings.SettingNotFoundException e) {
                Log.d(TAG, "catch exception");
                hasRefreshed = false;
            }
            break;
        case NetModeSelectHelper.NETMODE_AUTO:
            Settings.Global
                    .putInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                            subId), 3);
            try {
                int newvalue = Settings.Global
                        .getInt(getContentResolver(), getSetting(Settings.Global.PREFERRED_NETWORK_MODE,
                                subId));
                if (newvalue == 3) {
                    hasRefreshed = true;
                } else {
                    hasRefreshed = false;
                }
                Log.d(TAG, "NetMode Set Auto and Refresh Setting Provider " + newvalue);
            } catch (Settings.SettingNotFoundException e) {
                Log.d(TAG, "catch exception");
                hasRefreshed = false;
            }
            break;
        default:
            break;
        }
        return hasRefreshed;
    }

    public int slotIdToSubId(int phoneId) {
        int subId;
        SubscriptionInfo mSubscriptionInfo = mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
        if (mSubscriptionInfo != null) {
            subId = mSubscriptionInfo.getSubscriptionId();
        } else {
            subId = SubscriptionManager.getDefaultSubscriptionId();
        }
        return subId;
     }

    public String getSetting(String defaultSetting, int phoneId) {
        if (isMultiSimEnabledEx()) {
            return defaultSetting + phoneId;
        } else {
            return defaultSetting;
        }
    }

    private  boolean isMultiSimEnabledEx() {
        String multiSimConfig = SystemProperties.get(PROPERTY_MULTI_SIM_CONFIG);
        return (multiSimConfig.equals("dsds") || multiSimConfig.equals("dsda") || multiSimConfig
                .equals("tsts"));
    }

    class NMODEHandler extends Handler {

        public NMODEHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case SET_NETMODE: {
                    int sim = (int) msg.arg1;
                    String valueStr = (String) msg.obj;
                    int value = Integer.valueOf(valueStr.trim());

                    if (mSelectHelper[sim].setNetMode(value)) {
                        // Modify 324468 by sprd
                        if (refreshSettingProvider(sim, value)) {
                            if (value == NetModeSelectHelper.NETMODE_GSMONLY && (mLastValue == NetModeSelectHelper.NETMODE_TDONLY || mLastValue == NetModeSelectHelper.NETMODE_TDPREFER)
                                    || value == NetModeSelectHelper.NETMODE_TDONLY
                                    || value == NetModeSelectHelper.NETMODE_TDPREFER) {
                                IATUtils.sendATCmd(engconstents.ENG_AT_RESET,
                                        "atchannel" + sim);
                            }
                            mLastValue = value;
                            mUiThread.sendMessage(mUiThread.obtainMessage(SET_NETMODE,
                                    sim, 1, null));
                        } else {
                            mUiThread.sendMessage(mUiThread.obtainMessage(SET_NETMODE,
                                    sim, 0, String.valueOf(mLastValue)));
                        }
                    } else {
                        mUiThread.sendMessage(mUiThread.obtainMessage(SET_NETMODE,
                                sim, 0, String.valueOf(mLastValue)));
                    }
                    break;
                }
            case GET_NETMODE: {
                int sim = (int) msg.arg1;
                if (mSelectHelper[sim] != null) {
                    int netMode = mSelectHelper[sim].getNetMode();
                    if (netMode != NetModeSelectHelper.NETMODE_INVALID) {

                        mUiThread.sendMessage(mUiThread.obtainMessage(
                                GET_NETMODE, sim, netMode));
                    }
                    Log.d(TAG, "GET_NETMODE  sim =" + sim + "netMode ="
                            + netMode);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

class NetModeSelectHelper {
    private final static String TAG = "NetModeSelectHelper";
    private int mSimIndex = -1;
    private int mModemType = -1;
    private Context mContext = null;

    public final static int NETMODE_INVALID = -1;
    public final static int NETMODE_AUTO = 0;
    public final static int NETMODE_GSMONLY = 1;
    public final static int NETMODE_WCDMAONLY = 2;
    public final static int NETMODE_WCDMAPREFER = 3;
    public final static int NETMODE_TDONLY = 4;
    public final static int NETMODE_TDPREFER = 5;

    // public final static int NETMODE_GSMPREFER = 6;

    public static NetModeSelectHelper getNetModeSelectHelper(int simIndex,Context context) {
        NetModeSelectHelper helper = new NetModeSelectHelper(simIndex,context);

        if (helper.mSimIndex == -1 || helper.mModemType == -1) {
            return null;
        }

        return helper;
    }

    public int getModemType() {
        return mModemType;
    }

    public int getNetMode() {
        int netMode = NETMODE_INVALID;
        if (mModemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
            String netmodeNv1 = IATUtils.sendATCmd(engconstents.ENG_GET_SNVM
                    + "2118", "atchannel" + mSimIndex);
            String netmodeNv2 = IATUtils.sendATCmd(engconstents.ENG_GET_SNVM
                    + "2124", "atchannel" + mSimIndex);
            String netmodeNv3 = IATUtils.sendATCmd(engconstents.ENG_GET_SNVM
                    + "2138", "atchannel" + mSimIndex);

            if (!netmodeNv1.contains(IATUtils.AT_OK)
                    || !netmodeNv2.contains(IATUtils.AT_OK)
                    || !netmodeNv3.contains(IATUtils.AT_OK)) {
                netMode = NETMODE_INVALID;
            } else if (netmodeNv1.contains("00")) {
                netMode = NETMODE_GSMONLY;
            } else if (netmodeNv1.contains("01")) {
                netMode = NETMODE_TDONLY;
            } else if (netmodeNv1.contains("02") && netmodeNv2.contains("01")) {
                netMode = NETMODE_TDPREFER;
                // } else if (netmodeNv1.contains("02") &&
                // netmodeNv2.contains("00")) {
                // netMode = NETMODE_GSMPREFER;
            } else {
                netMode = NETMODE_INVALID;
            }
        } else {
            String atRsp = IATUtils.sendATCmd(engconstents.ENG_AT_NETMODE1,
                    "atchannel" + mSimIndex);

            if (atRsp.contains(IATUtils.AT_OK)) {
                String[] str = atRsp.split("\\:");
                String[] str1 = str[1].split("\\,");
                String result = str1[0].trim();
                String subResult = str1[1].trim();
                if (result.contains("2")) {
                    if (mModemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                        netMode = NETMODE_WCDMAPREFER;
                    } else if (mModemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                        netMode = NETMODE_TDPREFER;
                    } else {
                        netMode = NETMODE_GSMONLY;
                    }
                } else if (result.contains("13")) {
                    netMode = NETMODE_GSMONLY;
                } else if (result.contains("14")) {
                    netMode = NETMODE_WCDMAONLY;
                } else if (result.contains("15")) {
                    netMode = NETMODE_TDONLY;
                }
            }
        }

        return netMode;
    }

    public boolean setNetMode(int netMode) {
        if (checkNetMode(netMode)) {
            String atCmd = null;
            switch (netMode) {
            case NETMODE_AUTO:
                atCmd = engconstents.ENG_AT_NETMODE + "2,0,2,4";
                break;
            case NETMODE_GSMONLY:
                if (mModemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                    if (IATUtils.sendATCmd(
                            engconstents.ENG_SET_SNVM + "2118,00",
                            "atchannel" + mSimIndex).contains(IATUtils.AT_OK)
                            && IATUtils.sendATCmd(
                                    engconstents.ENG_SET_SNVM + "2124,00",
                                    "atchannel" + mSimIndex).contains(
                                    IATUtils.AT_OK)
                            && IATUtils.sendATCmd(
                                    engconstents.ENG_SET_SNVM + "2138,02",
                                    "atchannel" + mSimIndex).contains(
                                    IATUtils.AT_OK)) {
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    atCmd = engconstents.ENG_AT_NETMODE + "13,3,2,4";
                }
                break;
            case NETMODE_WCDMAONLY:
                atCmd = engconstents.ENG_AT_NETMODE + "14,3,2,4";
                break;
            case NETMODE_WCDMAPREFER:
                atCmd = engconstents.ENG_AT_NETMODE + "2,2,2,4";
                break;
            case NETMODE_TDONLY:
                if (IATUtils.sendATCmd(engconstents.ENG_SET_SNVM + "2118,01",
                        "atchannel" + mSimIndex).contains(IATUtils.AT_OK)
                        && IATUtils.sendATCmd(
                                engconstents.ENG_SET_SNVM + "2124,01",
                                "atchannel" + mSimIndex).contains(
                                IATUtils.AT_OK)
                        && IATUtils.sendATCmd(
                                engconstents.ENG_SET_SNVM + "2138,02",
                                "atchannel" + mSimIndex).contains(
                                IATUtils.AT_OK)) {
                    return true;
                } else {
                    return false;
                }
            case NETMODE_TDPREFER:
                if (IATUtils.sendATCmd(engconstents.ENG_SET_SNVM + "2118,02",
                        "atchannel" + mSimIndex).contains(IATUtils.AT_OK)
                        && IATUtils.sendATCmd(
                                engconstents.ENG_SET_SNVM + "2124,01",
                                "atchannel" + mSimIndex).contains(
                                IATUtils.AT_OK)
                        && IATUtils.sendATCmd(
                                engconstents.ENG_SET_SNVM + "2138,01",
                                "atchannel" + mSimIndex).contains(
                                IATUtils.AT_OK)) {
                    return true;
                } else {
                    return false;
                }
            }
            String atRsp = IATUtils.sendATCmd(atCmd, "atchannel" + mSimIndex);

            if (atRsp.contains(IATUtils.AT_OK)) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    private NetModeSelectHelper(int simIndex, Context context) {
        mSimIndex = simIndex;
        mContext = context;
        TelephonyManagerSprd tm = new TelephonyManagerSprd(context);
        int previousSetPhoneId = tm.getPrimaryCard();
        Log.d(TAG,"Main sim is: " + previousSetPhoneId);
        if (TelephonyManagerSprd.getModemType() == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
            if (mSimIndex == previousSetPhoneId) {
                mModemType = TelephonyManagerSprd.MODEM_TYPE_TDSCDMA;
            } else {
                mModemType = TelephonyManagerSprd.MODEM_TYPE_GSM;
            }
        } else if (TelephonyManagerSprd.getModemType() == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
            if (mSimIndex == previousSetPhoneId) {
                mModemType = TelephonyManagerSprd.MODEM_TYPE_WCDMA;
            } else {
                mModemType = TelephonyManagerSprd.MODEM_TYPE_GSM;
            }
        } else {
            mModemType = TelephonyManagerSprd.MODEM_TYPE_GSM;
        }
        Log.d(TAG, "Modem type is: " + mModemType);
    }

    private boolean checkNetMode(int netMode) {
        if ((mModemType == TelephonyManagerSprd.MODEM_TYPE_GSM)
                && ((netMode == NETMODE_AUTO) || (netMode == NETMODE_GSMONLY))) {
            return true;
        } else if ((mModemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA)
                && ((netMode == NETMODE_AUTO) || (netMode == NETMODE_GSMONLY)
                        || (netMode == NETMODE_WCDMAONLY) || (netMode == NETMODE_WCDMAPREFER))) {
            return true;
        } else if ((mModemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA)
                && ((netMode == NETMODE_AUTO) || (netMode == NETMODE_GSMONLY)
                        || (netMode == NETMODE_TDONLY) || (netMode == NETMODE_TDPREFER))) {
            return true;
        } else {
            return false;
        }
    }

}
