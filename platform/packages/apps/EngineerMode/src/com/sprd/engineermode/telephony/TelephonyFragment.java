
package com.sprd.engineermode.telephony;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;
import android.preference.PreferenceFragment;
import android.preference.SwitchPreference;
import android.preference.PreferenceManager;
import android.preference.TwoStatePreference;
import android.preference.EditTextPreference;
import android.preference.CheckBoxPreference;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.Toast;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import com.sprd.engineermode.telephony.TelephonyManagerSprd.RadioCapbility;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.text.InputFilter;
import android.text.InputFilter.LengthFilter;
import android.text.method.NumberKeyListener;

import com.sprd.engineermode.debuglog.BandModeSetActivity;
import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.provider.Settings;
import android.provider.Settings.System;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import com.android.internal.telephony.TelephonyIntents;
import com.android.ims.ImsManager;

import android.os.UserHandle;

public class TelephonyFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnSharedPreferenceChangeListener {

    private static final String TAG = "TelephonyFragment";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";
    private static final String KEY_VIDEOTYPE = "video_type";
    private static final String KEY_QOSSWITCH = "qos_switch";
    private static final String KEY_AUTO_ANSWER = "auto_answer";
    private static final String KEY_NETMODE_SELECT = "network_mode";
    private static final String KEY_Image_SWITCH = "imageswitch";
    private static final String KEY_GPRS_SET = "gprs_set";
    private static final String KEY_SPRD_ILOG = "sprd_ilog";
    private static final String KEY_BACKGROUNDSEARCH = "lte_backgroundsearch";
    private static final String KEY_TIMER = "lte_timer";
    private static final String KEY_RSRP = "lte_rsrp";

    //add-
    private static final String ACTION_SIM_STATE_CHANGED = "android.intent.action.SIM_STATE_CHANGED";
    //-add
    /*
     * SPRD: modify 20140621 Spreadtrum of 325713 EngineerMode, telephony-LTE modem add Clear Prior
     * Information@{
     */
    private static final String KEY_CLEAR_PRIOR_INFORMATION = "clear_prior_information";
    private static final String KEY_SIM_INDEX = "simindex";

    private static final String LOCKED_BAND = "locked_band";
    private static final String MODEM_TYPE = "modemtype";
    private static final String W_PREFER_PLUS = "w_prefer_plus";
    private static final String ISDUALMODE = "dual_mode";
    private static final String NET_MODE = "net_mode";
    private static final String G_LOCKED_BAND = "g_locked_band";
    private static final String ISSUPPORTLTE = "lte_support";
    private static final String LTE_LOCKED_BAND = "lte_locked_band";

    private static final String KEY_NETINFO_STATI = "netinfo_statistics";
    private static final String KEY_FAST_DORMANCY = "fastdormancy";
    private static final String KEY_FAST_DORMANCY_VALUE = "fastdormancy_value";
    private static final String KEY_DATA_SERVICE_PREFERRED = "dataservicepreferred";
    private static final String KEY_VT_OPTION_PREFERRED = "VT_option";
    private static final String KEY_PLSENSOR_OPTION = "PLsensor_option";

    private static final String KEY_SIMLOCK_TYPE = "simlock_type";
    private static final String KEY_SIMLOCK_NETWORK = "networkpersonalization";
    private static final String KEY_SIMLOCK_NETWORK_SUBSET = "networksubsetpersonalization";
    private static final String KEY_SIMLOCK_SERVICE = "serviceproviderpersonalization";
    private static final String KEY_SIMLOCK_CORPORATE = "corporatepersonalization";
    private static final String KEY_SIMLOCK_SIM = "simpersonalization";
    private static final String KEY_LTE_MODE_SET = "lte_set";
    private static final String KEY_LTE_MODEM_SET = "lte_modem_set";
    private static final String KEY_LTE_IMPEDE_DATA = "lte_impede_data";
    private static final String KEY_SIM_TRACE = "sim_trace";
    private static final String KEY_USB_ACTIVE = "switch_for_usb_active";
    private static final String KEY_DNS_FILTER = "dns_filter";
    private static final String KEY_VOLTE_SETTING = "volte_setting";
    private static final String KEY_AUTO_AUTH = "auto_auth";
    private static final String KEY_MODE_SWITCH = "mode_switch";
    public  static final String KEY_PHONEID = "key_phoneid";
    public  static final String KEY_THERMAL_SWITCH = "thermal_switch";
    public  static final String KEY_THERMAL_PA_SWITCH = "thermal_pa_switch";
    public  static final String KEY_DATA_SWITCH = "data_switch";
    public  static final String KEY_VOLTE_SWITCH = "volte_switch";
    public  static final String KEY_CSFB_GSM = "csfb2gsm_delay";
    public  static final String KEY_IMS_SWITCH = "ims_switch";
    public  static final String KEY_SMS_SWITCH = "sms_over_ip_switch";
    private static final String KEY_LOAD_APN = "load_apn";
    private static final String KEY_SINGLE_DUAL_SIM_MODE_SWITCH = "single_dual_sim_mode_switch";
    private static final String KEY_DDR_SWITCH = "ddr_switch";
    //SPRD: 474686 Feature for Uplmn
    public static final String KEY_UPLMN_SWITCH = "uplmn_switch";
    //private static final String KEY_SESSION_TIMER="session_timer";
    //private static final String KEY_FORCE_MT_SESSION_TIMER="force_mt_session_timer";
    private static final String KEY_MOS_SWITCH="mos_switch";
    private static final String KEY_NET_INFO_RECORD="net_info_record";

    private static final int SET_CFU = 1;
    private static final int OPEN_AUTO_ANSWER = 2;
    private static final int CLOSE_AUTO_ANSWER = 3;
    private static final int TD_BAND_QUERY = 4;
    // private static final int LTE_BAND_QUERY = 19;
    private static final int W_BAND_QUERY = 5;
    private static final int G_BAND_QUERY = 6;
    // private static final int GET_FAST_DORMANCY = 7;
    private static final int SET_FAST_DORMANCY = 8;
    private static final int SET_VIDEOTYPE = 9;
    private static final int GET_VIDEOTYPE = 10;
    private static final int OPEN_DATA_SERVICES_PRE = 11;
    private static final int CLOSE_DATA_SERVICES_PRE = 12;

    private static final int BACKGROUNDSEARCH = 13;
    private static final int LTESET = 14;
    private static final int RSRPSET = 15;
    // private static final int CPINFO = 18;

    private static final int GET_WPREFER_PLUS_STATUS = 16;
    private static final int SET_WPREFER_PLUS_STATUS = 17;
    private static final int CPINFO = 18;
    private static final int LTE_BAND_QUERY = 19;
    private static final int LTE_GET_DATA_IMPEDE = 20;
    private static final int LTE_SET_DATA_IMPEDE = 21;
    private static final int GET_SIM_TRACE_STATUS = 22;
    private static final int OPEN_SIM_TRACE = 23;
    private static final int CLOSE_SIM_TRACE = 24;
    private static final int SET_MODE_SWITCH = 25;
    private static final int GET_AUTO_ANSWER = 26;
    private static final int SET_SINGLE_DUAL_MODE_SWITCH = 27;
    //begin 550429 add by suyan.yang 2016.05.10
    private static final int GET_SMS_OVER_IP_STATUS = 28;
    private static final int SET_SMS_OVER_IP_STATUS = 29;
    //end 550429 add by suyan.yang 2016.05.10
//    private static final int GET_SESSION_TIMER = 30;
//    private static final int SET_SESSION_TIMER = 31;
//    private static final int GET_FORCE_MT_SESSION_TIMER = 32;
//    private static final int SET_FORCE_MT_SESSION_TIMER = 33;
    /*
     * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
     * add NXP list and MOS switch @{
     */
    private static final int GET_NXP_STATUS=34;
    private static final int GET_MOS_STATUS=35;
    private static final int SET_MOS_STATUS=36;
    private static final int SET_NXP_STATUS=37;
    /* @} */

    private static final int FAST_DORMANCY_OPEN = 1;
    private static final int FAST_DORMANCY_CLOSE = 0;

    private static final String CFU_CONTROL = "persist.sys.callforwarding";
    private static final String VALUEKEY_VIDEO_TYPE = "debug.videophone.videotype";
    private static final String DDR_SWITCH_STATUS = "persist.sys.ddr.status";
    private static final String SWITCH_FOR_USB_ACTIVE = "switch_for_usb_active";

    private SwitchPreference mAutoAnswer;
    private SwitchPreference mQosSwitch;
    private SwitchPreference mSprdIlogSwitch;
    private ListPreference mSupplementaryServiceQuery;
    private ListPreference mVideoType;
    private EMSwitchPreference mFastdormancy;
    private Preference mNetModeSelect;
    private Preference mImageSwitchPreference;
    private EMSwitchPreference mWpreferplus;
    private EMSwitchPreference mDataServicePreferred;
    private PreferenceScreen mBandSelect;
    private Preference mBandSelectSim[];
    private Preference mGPRSSet;
    private EMSwitchPreference mVTOptionPreference;
    private EMSwitchPreference mLTEDateImpede;
    private TwoStatePreference mSimTrace;
    private Preference mNetinfoStatistics;
    private TwoStatePreference mUsbActive;
    private TwoStatePreference mDNSFilter;
    private TwoStatePreference mAutoAuth;
    private Preference mVolteSetting;
    private Preference mNVItemList;
    private ListPreference mModeSwitch;
    private ListPreference mSingleDualSIMModeSwitch;
    private EMSwitchPreference mDataSwitch;
    private EMSwitchPreference mVolteSwitch;
    private EMSwitchPreference mImsSwitch;
    private EMSwitchPreference mSmsOverIpSwitch;
    private EMSwitchPreference mLoadAPNSwitch;
    private EMSwitchPreference mDdrSwitch;
    private EMSwitchPreference mThermal,mThermalPa;
    private EMSwitchPreference mMosSwitch;
    private PreferenceScreen mNxp;
    private String[] status;
    private int nxp_index;
    //SPRD: 474686 Feature for Uplmn
    private EMSwitchPreference mUplmnSwitch;

    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    private Preference mCsfb2gsm;
    private Preference mNetInfoRecord;

    private Handler mUiThread = new Handler();
    private TELHandler mTELHandler;
    private Context mContext;
    private int mPhoneCount;
    private int[] mModemType;
    private int mSIM;
    private String mServerName = "atchannel0";
    private String mIntentAction;
    private String mBandCapbility;
    private String mLockBand;
    private String mLteLockBand;
    private ProgressDialog mProgressDialog;
    private TelephonyManager[] mTelephonyManager;
    private TelephonyManagerSprd mTelephonyManagerSprd;
    private boolean mIsDualMode = false;
    private String mNetMode;
    private boolean isSupportLTE = !(TelephonyManagerSprd.RadioCapbility.NONE
            .equals(TelephonyManagerSprd.getRadioCapbility()));
    private boolean isSupportCSFB = TelephonyManagerSprd.RadioCapbility.TDD_CSFB
            .equals(TelephonyManagerSprd.getRadioCapbility())
            || TelephonyManagerSprd.RadioCapbility.FDD_CSFB.equals(TelephonyManagerSprd
                    .getRadioCapbility())
            || TelephonyManagerSprd.RadioCapbility.CSFB.equals(TelephonyManagerSprd
                    .getRadioCapbility());
    private TelephonyManagerSprd.RadioCapbility nowModeName = TelephonyManagerSprd
            .getRadioCapbility();
    private String nowSingelDualSIMModeName = SystemProperties.get("persist.msms.phone_count");
    private boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
    private boolean disableNetmode = SystemProperties.get("persist.radio.network.unable").equals("true");
    private String mStrTmp;
    private int mMesarg;
    private String mATCmd;
    private static final String DORM_TIME = "Dormancy time";
    // private static String mDromTime;

    private EditTextPreference mBackgroundSearch;
    private EditTextPreference mLTE;
    private EditTextPreference mRSRP;
    //private EditTextPreference mSessionTimer,mForceMtSessionTimer;
    private Preference mCPINFO;
    private SharedPreferences mSharePref;
    private String strInput;
    private String mATResponse;
    private String mSetNum;
    private int mPhoneId = 0;
    private boolean Isenable = false;
    private int mCardCount = 0;
    private boolean mSupportVT = false;
    private boolean[] mIsCardExit;
    private RadioCapbility mCurrentRadioCapbility;
    private boolean mVolteChecked=false;
    private boolean mSmsChecked=false;
    private final int NXP_COUNT=27;
    private CheckBoxPreference[] mNxpList;

    private ThermalInterface mThermalInter = new ThermalInterface();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mTELHandler = new TELHandler(ht.getLooper());

        addPreferencesFromResource(R.xml.pref_telephonytab);

        PreferenceScreen prefSet = getPreferenceScreen();

        mContext = getActivity();
        preferences = mContext.getSharedPreferences("fd", mContext.MODE_PRIVATE);
        editor = preferences.edit();
        mAutoAnswer = (SwitchPreference) prefSet
                .findPreference(KEY_AUTO_ANSWER);
        mAutoAnswer.setOnPreferenceChangeListener(this);
        mQosSwitch = (SwitchPreference) prefSet.findPreference(KEY_QOSSWITCH);
        mQosSwitch.setOnPreferenceChangeListener(this);
        mBandSelect = (PreferenceScreen) findPreference("band_select");
        mGPRSSet = (Preference) findPreference(KEY_GPRS_SET);
        mGPRSSet.setEnabled(false);

        mSimTrace = (TwoStatePreference) findPreference(KEY_SIM_TRACE);
        mSimTrace.setOnPreferenceChangeListener(this);
        mUsbActive = (TwoStatePreference) findPreference(KEY_USB_ACTIVE);
        mUsbActive.setOnPreferenceChangeListener(this);

        mSprdIlogSwitch = (SwitchPreference) findPreference(KEY_SPRD_ILOG);
        mSprdIlogSwitch.setEnabled(false);
        mSprdIlogSwitch.setOnPreferenceChangeListener(this);
        mFastdormancy = (EMSwitchPreference) findPreference(KEY_FAST_DORMANCY);
        mFastdormancy.setOnPreferenceChangeListener(this);
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);
        mNetModeSelect = (Preference) findPreference(KEY_NETMODE_SELECT);
        mImageSwitchPreference = (Preference) findPreference(KEY_Image_SWITCH);

        mDataServicePreferred = (EMSwitchPreference) findPreference(KEY_DATA_SERVICE_PREFERRED);
        mDataServicePreferred.setOnPreferenceChangeListener(this);

        mVTOptionPreference = (EMSwitchPreference) findPreference(KEY_VT_OPTION_PREFERRED);
        mVTOptionPreference.setOnPreferenceChangeListener(this);
        mLTEDateImpede = (EMSwitchPreference) findPreference(KEY_LTE_IMPEDE_DATA);
        mLTEDateImpede.setOnPreferenceChangeListener(this);
        mNetinfoStatistics = (Preference) findPreference(KEY_NETINFO_STATI);
        mDNSFilter = (TwoStatePreference) findPreference(KEY_DNS_FILTER);
        mDNSFilter.setOnPreferenceChangeListener(this);
        mAutoAuth = (TwoStatePreference) findPreference(KEY_AUTO_AUTH);
        mModeSwitch = (ListPreference) findPreference(KEY_MODE_SWITCH);
        if ((!isSupportCSFB) || isUser) {
            mModeSwitch.setEnabled(false);
            mModeSwitch.setSummary(R.string.feature_not_support);
        }
        mSingleDualSIMModeSwitch = (ListPreference) findPreference(KEY_SINGLE_DUAL_SIM_MODE_SWITCH);
        if ("1".equals(SystemProperties.get("persist.sys.autoauth.enable"))) {
            mAutoAuth.setChecked(true);
        } else {
            mAutoAuth.setChecked(false);
        }
        mAutoAuth.setOnPreferenceChangeListener(this);
        mNVItemList = (Preference) findPreference("nv_item_list");
        mNVItemList.setEnabled(true);
        mVolteSetting = (Preference) findPreference(KEY_VOLTE_SETTING);
        mVolteSwitch = (EMSwitchPreference) findPreference(KEY_VOLTE_SWITCH);
        mVolteSwitch.setOnPreferenceChangeListener(this);
        mImsSwitch = (EMSwitchPreference) findPreference(KEY_IMS_SWITCH);
        mImsSwitch.setOnPreferenceChangeListener(this);
        //begin 550429 add by suyan.yang 2016.05.10
        mSmsOverIpSwitch = (EMSwitchPreference) findPreference(KEY_SMS_SWITCH);
        mSmsOverIpSwitch.setOnPreferenceChangeListener(this);
        //end 550429 add by suyan.yang 2016.05.10
        /* SPRD: 474686 Feature for Uplmn @{ */
        mUplmnSwitch = (EMSwitchPreference) findPreference(KEY_UPLMN_SWITCH);
        mUplmnSwitch.setOnPreferenceChangeListener(this);
        /* @} */
        if (SystemProperties.get("persist.sys.volte.enable").equals("true")) {
            mVolteSetting.setEnabled(true);
        } else {
            mVolteSetting.setEnabled(false);
        }

        if (!isSupportLTE) {
            mVolteSwitch.setEnabled(false);
            mImsSwitch.setEnabled(false);
            mVolteSwitch.setSummary(R.string.feature_not_support);
            mImsSwitch.setSummary(R.string.feature_not_support);
            mVolteSetting.setSummary(R.string.feature_not_support);
        } else {
            mVolteSwitch.setEnabled(true);
            mImsSwitch.setEnabled(true);
        }

        mDataSwitch = (EMSwitchPreference) findPreference(KEY_DATA_SWITCH);
        mDataSwitch.setOnPreferenceChangeListener(this);
        if (SystemProperties.get("persist.sys.data.restore", "false").equals("true")) {
            mDataSwitch.setChecked(true);
        } else {
            mDataSwitch.setChecked(false);
        }
        /*
         * SPRD: modify Spreadtrum of 506992 EngineerMode, add DDR switch to control the ddr power
         * down @{
         */
        mDdrSwitch = (EMSwitchPreference) findPreference(KEY_DDR_SWITCH);
        mDdrSwitch.setOnPreferenceChangeListener(this);
        /* @} */
        // Set SystemProperties.get("persist.sys.support.vt").equals("true")
        // will reasonable
        // But VT default is Enable in all apps, so set this to ture
        if (SystemProperties.get("persist.sys.support.vt").isEmpty()) {
            mVTOptionPreference.setChecked(true);
        } else {
            mVTOptionPreference.setChecked(SystemProperties.get(
                    "persist.sys.support.vt").equals("true"));
        }

        mPhoneCount = TelephonyManager.from(mContext).getPhoneCount();
        mModemType = new int[mPhoneCount];
        mBandSelectSim = new Preference[mPhoneCount];
        mIsCardExit = new boolean[mPhoneCount];

        mTelephonyManagerSprd = new TelephonyManagerSprd(mContext);
        mTelephonyManager = new TelephonyManager[mPhoneCount];
        getCardExitState();
        for (int i = 0; i < mPhoneCount; i++) {
            mModemType[i] = TelephonyManagerSprd.getModemType();
            mBandSelectSim[i] = new Preference(mContext);
            if (mModemType[i] == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mNetModeSelect.setSummary(R.string.input_cmcc_card);
                Log.d(TAG, "modem type is TDSCDMA");
            } else if (mModemType[i] == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mNetModeSelect.setSummary(R.string.input_cucc_card);
                Log.d(TAG, "modem type is WCDMA");
            }
            mBandSelectSim[i].setTitle("SIM" + i);
            mBandSelectSim[i].setKey("SIM" + i);
            mBandSelect.addPreference(mBandSelectSim[i]);
        }

        if (TelephonyManagerSprd.getModemType() == TelephonyManagerSprd.MODEM_TYPE_GSM) {
            Log.d(TAG,
                    "Modem Type is GSM, remove net mode select and vt option");
            mNetModeSelect.setEnabled(false);
            mNetModeSelect.setSummary(R.string.feature_not_support);
            mNetinfoStatistics.setEnabled(false);
            mNetinfoStatistics.setSummary(R.string.feature_not_support);
        }
        if (disableNetmode) {
            mNetModeSelect.setEnabled(false);
            mNetModeSelect.setSummary(R.string.feature_not_support);
        }

        // If RegionConfig is exists, show mImageSwitchPreference;else do not show
        if (!isAppInstalled(mContext, "com.android.regionconfig")) {
            getPreferenceScreen().removePreference(mImageSwitchPreference);
        }

        /*
         * SPRD: modify 20140701 Spreadtrum of 328867 EngineerMode,there is no "NetworkMode" options
         * in Telephony @{ if (TelephonyManager.getDefault(0).getModemType() ==
         * TelephonyManagerSprd.MODEM_TYPE_LTE) { mNetModeSelect.setEnabled(false);
         * prefSet.removePreference(mNetModeSelect); }
         * @}
         */
        // sharkl does not support VT
        mCurrentRadioCapbility = TelephonyManagerSprd.getRadioCapbility();
        if (mCurrentRadioCapbility == TelephonyManagerSprd.RadioCapbility.TDD_CSFB || isSupportCSFB) {
            mVTOptionPreference.setEnabled(false);
            prefSet.removePreference(mVTOptionPreference);
        }

        registerReceiver();

        if (!mGPRSSet.isEnabled()) {
            mGPRSSet.setSummary(R.string.input_card_to_test);
        }

        mWpreferplus = (EMSwitchPreference) findPreference(W_PREFER_PLUS);
        mWpreferplus.setOnPreferenceChangeListener(this);
        if (TelephonyManagerSprd.getModemType() != TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
            mWpreferplus.setEnabled(false);
            mWpreferplus.setSummary(R.string.feature_not_support);
            mWpreferplus = null;
        }

        mVideoType = (ListPreference) findPreference(KEY_VIDEOTYPE);

        mSharePref = PreferenceManager.getDefaultSharedPreferences(mContext);
        mSharePref.registerOnSharedPreferenceChangeListener(this);

        if (isSupportLTE) {
            mBackgroundSearch = (EditTextPreference) prefSet
                    .findPreference(KEY_BACKGROUNDSEARCH);
            String background = mSharePref
                    .getString(KEY_BACKGROUNDSEARCH, null);
            if (background == null) {
                mBackgroundSearch.setSummary(getResources().getString(
                        R.string.input));
            } else {
                mBackgroundSearch.setSummary(background);
            }
            mBackgroundSearch.setText(null);
            mBackgroundSearch.setOnPreferenceChangeListener(this);

            mLTE = (EditTextPreference) prefSet.findPreference(KEY_TIMER);
            String timer = mSharePref.getString(KEY_TIMER, null);
            if (timer == null) {
                mLTE.setSummary(R.string.input);
            } else {
                mLTE.setSummary(timer);
            }
            mLTE.setText(null);
            mLTE.setOnPreferenceChangeListener(this);

            mRSRP = (EditTextPreference) prefSet.findPreference(KEY_RSRP);
            String rsrp = mSharePref.getString(KEY_RSRP, null);
            if (rsrp == null) {
                mRSRP.setSummary(R.string.input);
            } else {
                mRSRP.setSummary(rsrp);
            }
            mRSRP.setText(null);
            mRSRP.setOnPreferenceChangeListener(this);
            /*
             * SPRD: modify 20140621 Spreadtrum of 325713 EngineerMode, telephony-LTE modem add
             * Clear Prior Information@{
             */
            mCPINFO = (Preference) prefSet
                    .findPreference(KEY_CLEAR_PRIOR_INFORMATION);
            Log.d(TAG,
                    "ril.service.l.enable is "
                            + SystemProperties.get("ril.service.l.enable", ""));
            // When not insert 4G card, grey
            if (!"1".equals(SystemProperties.get("ril.service.l.enable", ""))) {
                mCPINFO.setEnabled(false);
            }
            mCPINFO.setOnPreferenceClickListener(new OnPreferenceClickListener() {
                @Override
                public boolean onPreferenceClick(Preference preference) {
                    Message m = mTELHandler.obtainMessage(CPINFO);
                    mTELHandler.sendMessage(m);
                    return false;
                }
            });
            /* @} */
        } else {
            /*
             * SPRD: modify 20140701 Spreadtrum of 328867 EngineerMode,there is no "NetworkMode"
             * options in Telephony @{ Preference lteSet = (Preference)
             * prefSet.findPreference(KEY_LTE_MODE_SET); prefSet.removePreference(lteSet);
             * ----328867 @}
             */
            mLTEDateImpede.setEnabled(false);
            mLTEDateImpede.setSummary(R.string.feature_not_support);
            Preference lteModemSet = (Preference) prefSet
                    .findPreference(KEY_LTE_MODEM_SET);
            prefSet.removePreference(lteModemSet);
        }
        mCsfb2gsm = (Preference) findPreference(KEY_CSFB_GSM);
        if (!isSupportLTE) {
            mCsfb2gsm.setEnabled(false);
            mCsfb2gsm.setSummary(R.string.feature_not_support);
        }
        mLoadAPNSwitch = (EMSwitchPreference) findPreference(KEY_LOAD_APN);
        mLoadAPNSwitch.setOnPreferenceChangeListener(this);
        mThermal = (EMSwitchPreference) findPreference(KEY_THERMAL_SWITCH);
        mThermal.setOnPreferenceChangeListener(this);
//        mSessionTimer=(EditTextPreference)findPreference(KEY_SESSION_TIMER);
//        mSessionTimer.setOnPreferenceChangeListener(this);
//        //begin bug572397 add by suyan.yang 20160620
//        mForceMtSessionTimer=(EditTextPreference)findPreference(KEY_FORCE_MT_SESSION_TIMER);
//        mForceMtSessionTimer.setOnPreferenceChangeListener(this);
//        //end bug572397 add by suyan.yang 20160620
        mNetInfoRecord=(Preference)findPreference(KEY_NET_INFO_RECORD);

        /*
         * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
         * add NXP list and MOS switch @{
         */
         mNxp=(PreferenceScreen)findPreference("nxp_list");
         mNxpList=new CheckBoxPreference[NXP_COUNT];
         for(int i=0;i<NXP_COUNT;i++){
             mNxpList[i]=new CheckBoxPreference(mContext);
             mNxpList[i].setTitle(this.getResources().getStringArray(R.array.nxp_list)[i]);
             mNxp.addPreference(mNxpList[i]);
             mNxp.setKey("nxp"+i);
             mNxpList[i].setOnPreferenceClickListener(clickListenser);
         }
         mNxp.setOnPreferenceClickListener(new OnPreferenceClickListener() {
             @Override
             public boolean onPreferenceClick(Preference preference) {
                 if(mNxp!=null){
                     Message simTraceSta = mTELHandler
                             .obtainMessage(GET_NXP_STATUS);
                     mTELHandler.sendMessage(simTraceSta);
                 }
                 return true;
             }
         });

         mMosSwitch = (EMSwitchPreference) findPreference(KEY_MOS_SWITCH);
         mMosSwitch.setOnPreferenceChangeListener(this);
         if(isSupportLTE){
             mMosSwitch.setEnabled(true);
         }else{
             mMosSwitch.setEnabled(false);
             mMosSwitch.setSummary(R.string.feature_not_support);
             mNxp.setSummary(mContext.getString(R.string.feature_not_support));
             mNxp.setEnabled(false);
         }
         /* @} */
         mThermalPa = (EMSwitchPreference) findPreference(KEY_THERMAL_PA_SWITCH);
         mThermalPa.setOnPreferenceChangeListener(this);
    }

    private void getCardExitState() {
        for (int i = 0; i < mPhoneCount; i++) {
            mTelephonyManager[i] = (TelephonyManager) TelephonyManager
                    .from(mContext);
            if (mTelephonyManager[i] != null
                    && mTelephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                mIsCardExit[i] = true;
            } else {
                mIsCardExit[i] = false;
            }
            if (mIsCardExit[i]) {
                mGPRSSet.setEnabled(true);
                mPhoneId = i;
                mCardCount++;
            }
            Log.d(TAG, "mIsCardExit[" + i + "] = " + mIsCardExit[i]
                    + " mCardCount = " + mCardCount);
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    private BroadcastReceiver mMobileReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            String action = intent.getAction();
            Log.d(TAG, "action = " + action);
            if (action.startsWith(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                if (! disableNetmode) updateModeType();
            } else if (action.startsWith(ACTION_SIM_STATE_CHANGED)) {
                getCardExitState();
                setCardExitState();
            }
            /* @} */
        }
    };

    private void setCardExitState() {
        boolean isAirplane = Settings.Global.getInt(
                mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) == 1;
        for (int i = 0; i < mPhoneCount; i++) {
            if (mIsCardExit[i]) {
                if (isAirplane || !TelephonyManagerSprd.isSimStandby(i, mContext)) {
                    mBandSelectSim[i].setSummary(mContext.getString(R.string.open_card_warn));
                    mBandSelectSim[i].setEnabled(false);
                    if (BandModeSetActivity.BandModeSetActivityInstance != null) {
                            BandModeSetActivity.BandModeSetActivityInstance.finish();
                    }
                } else {
                    mBandSelectSim[i].setSummary(null);
                    mBandSelectSim[i].setEnabled(true);
                }
            } else {
                mBandSelectSim[i].setSummary(mContext.getString(R.string.input_sim__warn));
                mBandSelectSim[i].setEnabled(false);
                if (BandModeSetActivity.BandModeSetActivityInstance != null) {
                        BandModeSetActivity.BandModeSetActivityInstance.finish();
                    }
            }
        }
    }

    private void updateThermalStatus() {
        if (mThermal != null) {
            boolean en = true;
            en = mThermalInter.isThermalEnabled();
            mThermal.setChecked(en);
        }
    }

     private void updateThermalPaStatus() {
        if (mThermalPa != null) {
            boolean en = true;
            en = mThermalInter.isThermalPaEnabled();
            Log.d(TAG, "mThermalInter.isThermalPaEnabled() " + en);
            mThermalPa.setChecked(en);
        }
    }
    private void updateModeType() {
        int primaryCard = mTelephonyManagerSprd.getPrimaryCard();
        boolean isStandby = TelephonyManagerSprd.isSimStandby(primaryCard, mContext);
        boolean isAirplane = Settings.Global.getInt(
                mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) == 1;
        Log.d(TAG, "isSupportLTE = " + isSupportLTE + " isStandby = "
                + isStandby + " isAirplane = " + isAirplane);
        boolean isEnable = false;
        CharSequence summary = null;
        if (isSupportLTE) {
            if (TelephonyManager.from(mContext).getPhoneCount() == 1) {
                if (mCardCount != 0 ) {
                    if (!isStandby || isAirplane) {
                        isEnable = false;
                        summary = mContext.getString(R.string.open_card_warn);
                    } else {
                        isEnable = true;
                        summary = null;
                    }
                } else {
                    isEnable = false;
                    summary = mContext.getString(R.string.input_sim__warn);
                }
            } else {
                if (mCardCount == 0) {
                    isEnable = false;
                    summary = mContext.getString(R.string.input_sim__warn);
                } else {
                    if (!isStandby || isAirplane) {
                        isEnable = false;
                        summary = mContext.getString(R.string.open_card_warn);
                    } else {
                        isEnable = true;
                        summary = null;
                    }
                }
            }
            mNetModeSelect.setEnabled(isEnable);
            mNetModeSelect.setSummary(summary);

        }
        setCardExitState();
    }

    public void registerReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction(ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mMobileReceiver, filter);
    }

    public void unregisterReceiver() {
        try {
            mContext.unregisterReceiver(mMobileReceiver);
        } catch (IllegalArgumentException iea) {
            // Ignored.
        }
    }

    private int changeValueToIndex(TelephonyManagerSprd.RadioCapbility PrefKey) {
        int valueIndex = 0;
        if ((TelephonyManagerSprd.RadioCapbility.TDD_CSFB).equals(PrefKey)) {
            valueIndex = 0;
        } else if ((TelephonyManagerSprd.RadioCapbility.FDD_CSFB).equals(PrefKey)) {
            valueIndex = 1;
        } else if ((TelephonyManagerSprd.RadioCapbility.CSFB).equals(PrefKey)) {
            valueIndex = 2;
        }
        return valueIndex;
    }

    private int changeSingleDualSIMModeValueToIndex(String PrefKey) {
        int valueIndex = 0;
        if (("1").equals(PrefKey)) {
            valueIndex = 0;
        } else if (("2").equals(PrefKey)) {
            valueIndex = 1;
        }
        return valueIndex;
    }

    private void setModeSwitch(int value) {
        switch (value) {
            case 0:
                  SystemProperties.set("persist.radio.modem.config", "TL_TD_G,G");
                  SystemProperties.set("persist.radio.modem.workmode", "7,10");
                break;
            case 1:
                  SystemProperties.set("persist.radio.modem.config", "TL_LF_W_G,G");
                  SystemProperties.set("persist.radio.modem.workmode", "6,10");
                break;
            case 2:
                  SystemProperties.set("persist.radio.modem.config", "TL_LF_TD_W_G,G");
                  SystemProperties.set("persist.radio.modem.workmode", "9,10");
                break;
            default:
                break;
        }
    }

    private void setSingeDualSIMModeSwitch(int value) {
        switch (value) {
            case 0:
                SystemProperties.set("persist.msms.phone_count", "1"); // set phone_count to 1
                SystemProperties.set("persist.msms.phone_default", "0"); // set phone_default to 0
                SystemProperties.set("persist.radio.multisim.config", "ssss"); // set multisim
                                                                               // config to single
                                                                               // sim single standby
                break;
            case 1:
                SystemProperties.set("persist.msms.phone_count", "2"); // set phone_count to 1
                SystemProperties.set("persist.msms.phone_default", "0"); // set phone_default to 0
                SystemProperties.set("persist.radio.multisim.config", "dsds"); // set multisim
                                                                               // config to dual sim
                                                                               // dual standby
                break;
            default:
                break;
        }
    }

    @Override
    public void onStart() {

        if (mAutoAnswer != null) {
            Message autoAnswerState = mTELHandler
                    .obtainMessage(GET_AUTO_ANSWER);
            mTELHandler.sendMessage(autoAnswerState);
        }
        /*
         * Modify 355060 sync CFU and supplementary_service_query which in TTCNActivity etc
         */
        if (SystemProperties.get("persist.radio.fd.disable", "").equals("0")) {
            mFastdormancy.setSummary(mContext.getString(R.string.Default_value));
            mFastdormancy.setChecked(true);
            editor.putString("fd_open", "true");
            editor.commit();
        } else if (SystemProperties.get("persist.radio.fd.disable", "").equals(
                "1")) {
            if (preferences.getString("fd_open", "").equals("true")) {
                mFastdormancy.setSummary(preferences.getString(DORM_TIME,
                        "null") + "s");
                mFastdormancy.setChecked(true);
            }
        }

        String cfuValue = mSharePref.getString("supplementary_service_query", SystemProperties.get(CFU_CONTROL, "0"));
        
        if (cfuValue.equals("0")) {
            mSupplementaryServiceQuery.setSummary("Default");
            mSupplementaryServiceQuery.setValue("0");
        } else if (cfuValue.equals("1")) {
            mSupplementaryServiceQuery.setSummary("Always Query");
            mSupplementaryServiceQuery.setValue("1");
        } else if (cfuValue.equals("2")) {
            mSupplementaryServiceQuery.setSummary("Always Not Query");
            mSupplementaryServiceQuery.setValue("2");
        }
        /*
         * Modify 355060 sync CFU and supplementary_service_query which in TTCNActivity etc
         */
        if (mSupportVT) {
            mVideoType.setSummary(mVideoType.getEntry());
        }

        if (mWpreferplus != null) {
            mTELHandler.sendEmptyMessage(GET_WPREFER_PLUS_STATUS);
        }

        if (mSimTrace != null) {
            Message simTraceSta = mTELHandler
                    .obtainMessage(GET_SIM_TRACE_STATUS);
            mTELHandler.sendMessage(simTraceSta);
        }
        if (mUsbActive != null) {
            int usbActiveSta = Settings.Global.getInt(mContext.getContentResolver(),
                    SWITCH_FOR_USB_ACTIVE, 0);
            Log.d(TAG, "Usb active status is: " + usbActiveSta);
            if (usbActiveSta == 1) {
                mUsbActive.setChecked(true);
            } else {
                mUsbActive.setChecked(false);
            }
        }
        if (isSupportLTE) {
            mTELHandler.sendEmptyMessage(LTE_GET_DATA_IMPEDE);
        }
        if (mModeSwitch != null && mModeSwitch.isEnabled()) {
            mModeSwitch.setValueIndex(changeValueToIndex(nowModeName));
            mModeSwitch.setSummary(mModeSwitch.getEntry());
        }
        if (mSingleDualSIMModeSwitch != null) {
            mSingleDualSIMModeSwitch
                    .setValueIndex(changeSingleDualSIMModeValueToIndex(nowSingelDualSIMModeName));
            mSingleDualSIMModeSwitch.setSummary(mSingleDualSIMModeSwitch.getEntry());
        }
        if (mDNSFilter != null) {
            if (SystemProperties.getBoolean("persist.sys.engineermode.dns", false)) {
                mDNSFilter.setChecked(true);
            } else {
                mDNSFilter.setChecked(false);
            }
        }
        if (mDdrSwitch != null) {
            boolean mDdrStatus = SystemProperties.getBoolean(DDR_SWITCH_STATUS, false);
            Log.d(TAG, "ddr current status is: " + mDdrStatus);
            mDdrSwitch.setChecked(mDdrStatus);
        }
        Log.d(TAG, "onStart");
        if (! disableNetmode) updateModeType();
        updateThermalStatus();
        updateThermalPaStatus();
        /*
         * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
         * add NXP list and MOS switch @{
         */
        if (isSupportLTE && mMosSwitch != null){
            Message simTraceSta = mTELHandler
               .obtainMessage(GET_MOS_STATUS);
            mTELHandler.sendMessage(simTraceSta);
        }
        /*  @} */
        super.onStart();
    }

    @Override
    public void onDestroy() {
        if (mTELHandler != null) {
            mTELHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        unregisterReceiver();
        super.onDestroy();
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery
                    .getEntry());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mTELHandler.obtainMessage(SET_CFU, re);
            mTELHandler.sendMessage(mSupplService);
        } else if (key.equals(KEY_VIDEOTYPE)) {
            mVideoType.setSummary(mVideoType.getEntry());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mTELHandler
                    .obtainMessage(SET_VIDEOTYPE, re);
            mTELHandler.sendMessage(mSupplService);
        } else if (key.equals(KEY_MODE_SWITCH)) {
            final String re = sharedPreferences.getString(key, "");
            if (Integer.parseInt(re) != changeValueToIndex(nowModeName)) {
                if (isAdded()) {
                    AlertDialog alertDialog = new AlertDialog.Builder(getActivity())
                            .setTitle(getString(R.string.mode_switch))
                            .setMessage(getString(R.string.mode_switch_waring))
                            .setPositiveButton(getString(R.string.alertdialog_ok),
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog,
                                                int which) {
                                            Message mSetMode = mTELHandler
                                                    .obtainMessage(SET_MODE_SWITCH, re);
                                            mTELHandler.sendMessage(mSetMode);
                                        }
                                    })
                            .setNegativeButton(R.string.alertdialog_cancel,
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog,
                                                int which) {
                                            mUiThread.post(new Runnable() {
                                                @Override
                                                public void run() {
                                                    mModeSwitch
                                                            .setValueIndex(changeValueToIndex(nowModeName));
                                                }
                                            });
                                        }
                                    }).create();
                    alertDialog.show();
                }
            }
        } else if (key.equals(KEY_SINGLE_DUAL_SIM_MODE_SWITCH)) {
            final String re = sharedPreferences.getString(key, "");
            if (Integer.parseInt(re) != changeSingleDualSIMModeValueToIndex(nowSingelDualSIMModeName)) {
                if (isAdded()) {
                    AlertDialog alertDialog = new AlertDialog.Builder(getActivity())
                            .setTitle(getString(R.string.single_dual_sim_mode_switch))
                            .setMessage(getString(R.string.mode_switch_waring))
                            .setPositiveButton(getString(R.string.alertdialog_ok),
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog,
                                                int which) {
                                            Message mSetMode = mTELHandler
                                                    .obtainMessage(SET_SINGLE_DUAL_MODE_SWITCH, re);
                                            mTELHandler.sendMessage(mSetMode);
                                        }
                                    })
                            .setNegativeButton(R.string.alertdialog_cancel,
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog,
                                                int which) {
                                            mUiThread.post(new Runnable() {
                                                @Override
                                                public void run() {
                                                    mSingleDualSIMModeSwitch
                                                            .setValueIndex(changeSingleDualSIMModeValueToIndex(nowSingelDualSIMModeName));
                                                }
                                            });
                                        }
                                    }).create();
                    // Make sure the dialog is always dismissed by pressing buttons.
                    alertDialog.setCancelable(false);
                    alertDialog.show();
                }
            }
        }

    }

    @Override
    public void onResume()
    {
        super.onResume();
        if (mVolteSwitch != null && mVolteSwitch.isEnabled()) {
            if (SystemProperties.get("persist.sys.volte.enable", "false")
                    .equals("true")) {
                mVolteSwitch.setChecked(true);
                mVolteSwitch.setSummary("Open");
            } else {
                mVolteSwitch.setChecked(false);
                mVolteSwitch.setSummary("Close");
            }
        }

        if (mImsSwitch != null && mImsSwitch.isEnabled()) {
            if (SystemProperties.get("persist.sys.ims.visibility", "false").equals("true")) {
                mImsSwitch.setChecked(true);
                mImsSwitch.setSummary("Open");
            } else {
                mImsSwitch.setChecked(false);
                mImsSwitch.setSummary("Close");
            }
        }

      //begin 550429 add by suyan.yang 2016.05.10
        if(mSmsOverIpSwitch != null && mSmsOverIpSwitch.isEnabled()){
            mTELHandler.sendEmptyMessage(GET_SMS_OVER_IP_STATUS);
        }
      //end 550429 add by suyan.yang 2016.05.10

        String sKeyBack = preferences.getString("key_back", "false");
        // determine whether the back key is pressed
        if (sKeyBack.equals("true")) {
            mFastdormancy.setChecked(false);
            editor.putString("key_back", "false");
            editor.commit();
        } else {
            String sDefault = preferences.getString("default", "");
            // determine whether choose the default value,"true" means choose
            // the default value,"false" means choose not
            if (sDefault.equals("true")) {
                SystemProperties.set("persist.radio.fd.disable", "0");
                editor.putString("fd_open", "true");
                editor.putString("default", "");
                editor.commit();
                mFastdormancy.setSummary(mContext
                        .getString(R.string.Default_value));
            } else if (sDefault.equals("false")) {
                editor.putString("default", "");
                editor.commit();
                int value;
                // get the self definite value
                String definite_value = preferences.getString("definite_value",
                        "");
                if (definite_value != null && definite_value.length() != 0) {
                    value = Integer.valueOf(definite_value);

                    if (value > 65535 || value <= 0) {
                        Toast.makeText(mContext,
                                "Please input between 1~65535",
                                Toast.LENGTH_SHORT).show();
                        mFastdormancy.setChecked(false);
                        return;
                    }
                    SystemProperties.set("persist.radio.fd.disable", "1");
                    Message fastDormancy = mTELHandler.obtainMessage(
                            SET_FAST_DORMANCY, FAST_DORMANCY_OPEN, 0,
                            definite_value);
                    mTELHandler.sendMessage(fastDormancy);
                    // editor.putString("DORM_TIME",definite_value);
                    editor.putString("fd_open", "true");
                    editor.commit();
                } else {
                    Toast.makeText(mContext, "Please input between 1~65535",
                            Toast.LENGTH_SHORT).show();
                    mFastdormancy.setChecked(false);
                    return;
                }
            }
        }
        if (mLoadAPNSwitch != null) {
            mLoadAPNSwitch.setChecked(SystemProperties.getBoolean(
                    "persist.sys.loaded.apn", false));
        }
/*        if(mSessionTimer != null){
            Message getTimer=mTELHandler
                    .obtainMessage(GET_SESSION_TIMER);
            mTELHandler.sendMessage(getTimer);
        }
        //begin bug572397 add by suyan.yang 20160620
        if(mForceMtSessionTimer != null){
            Message getTimer=mTELHandler
                    .obtainMessage(GET_FORCE_MT_SESSION_TIMER);
            mTELHandler.sendMessage(getTimer);
        }*/
        //end bug572397 add by suyan.yang 20160620
        mNetInfoRecord.setEnabled(false);
        for (int i = 0; i < mPhoneCount; i++) {
            if (mTelephonyManager[i] !=null && mTelephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                mNetInfoRecord.setEnabled(true);
                break;
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mAutoAnswer) {
            if (!mAutoAnswer.isChecked()) {
                Message openAutoAnswer = mTELHandler
                        .obtainMessage(OPEN_AUTO_ANSWER);
                mTELHandler.sendMessage(openAutoAnswer);
            } else {
                Message closeAutoAnswer = mTELHandler
                        .obtainMessage(CLOSE_AUTO_ANSWER);
                mTELHandler.sendMessage(closeAutoAnswer);
            }
        } else if (pref == mSimTrace) {
            if (!mSimTrace.isChecked()) {
                Message openSimTrace = mTELHandler
                        .obtainMessage(OPEN_SIM_TRACE);
                mTELHandler.sendMessage(openSimTrace);
            } else {
                Message closeSimTrace = mTELHandler
                        .obtainMessage(CLOSE_SIM_TRACE);
                mTELHandler.sendMessage(closeSimTrace);
            }
        } else if (pref == mWpreferplus) {
            if (mWpreferplus.isChecked()) {
                Message msg = mTELHandler.obtainMessage(
                        SET_WPREFER_PLUS_STATUS, 0, 0, null);
                mTELHandler.sendMessage(msg);
            } else {
                Message msg = mTELHandler.obtainMessage(
                        SET_WPREFER_PLUS_STATUS, 1, 0, null);
                mTELHandler.sendMessage(msg);
            }
        } else if (pref == mDNSFilter) {
            if (mDNSFilter.isChecked()) {
                SystemProperties.set("ctl.start", "disable_dns");
                SystemProperties.set("persist.sys.engineermode.dns", "0");
                Log.d(TAG, "start disable_dns");
            } else {
                SystemProperties.set("ctl.start", "enable_dns");
                SystemProperties.set("persist.sys.engineermode.dns", "1");
                Log.d(TAG, "start enable_dns");
            }
        } else if (pref == mAutoAuth) {
            if (mAutoAuth.isChecked()) {
                SystemProperties.set("persist.sys.autoauth.enable", "0");
                Log.d(TAG, "disable autoauth");
            } else {
                SystemProperties.set("persist.sys.autoauth.enable", "1");
                Log.d(TAG, "enable autoauth");
            }
        } else if (pref == mQosSwitch) {
            if (mQosSwitch.isChecked()) {
                SystemProperties.set("persist.sys.qosstate", "0");
                Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
            } else {
                SystemProperties.set("persist.sys.qosstate", "1");
                Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
            }
        } else if (pref == mUsbActive) {
            if (mUsbActive.isChecked()) {
                Settings.Global.putInt(mContext.getContentResolver(),
                        SWITCH_FOR_USB_ACTIVE, 0);
                Toast.makeText(mContext, "Close Success", Toast.LENGTH_SHORT).show();
            } else {
                Settings.Global.putInt(mContext.getContentResolver(),
                        SWITCH_FOR_USB_ACTIVE, 1);
                Toast.makeText(mContext, "Open Success", Toast.LENGTH_SHORT).show();
            }
        } else if (pref == mSprdIlogSwitch) {
            // Log.setIlogEnable(!mSprdIlogSwitch.isChecked());
            Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
        } else if (pref == mFastdormancy) {
            if (mFastdormancy.isChecked()) {
                SystemProperties.set("persist.radio.fd.disable", "1");
                Message fastDormancy = mTELHandler.obtainMessage(
                        SET_FAST_DORMANCY, FAST_DORMANCY_CLOSE, 0, null);
                mTELHandler.sendMessage(fastDormancy);
                editor.putString("fd_open", "false");
                editor.commit();
            } else {
                String value;
                if (mFastdormancy.getSummary() == null
                        || 0 == mFastdormancy.getSummary().toString()
                                .compareTo("65535")) {
                    value = "1~65535(s)";
                } else {
                    value = mFastdormancy.getSummary().toString();

                }
                // save the summary value
                editor.putString("summary", value);
                editor.commit();
                Intent intent = new Intent("android.intent.action.FDActivity");
                mContext.startActivity(intent);

            }
        } else if (pref == mDataServicePreferred) {
            if (!mDataServicePreferred.isChecked()) {
                mTELHandler.sendEmptyMessage(OPEN_DATA_SERVICES_PRE);
            } else {
                mTELHandler.sendEmptyMessage(CLOSE_DATA_SERVICES_PRE);
            }
        } else if (pref == mVTOptionPreference) {
            // boolean mIsVideotalkOn = mVTOptionPreference.isChecked();
            // Log.d(TAG, "mVTOptionPreference isChecked(): " +
            // mVTOptionPreference.isChecked());
            final PowerManager pm = (PowerManager) mContext
                    .getSystemService(Context.POWER_SERVICE);
            String message = this.getResources().getString(
                    R.string.vtoption_on_message);
            if (mVTOptionPreference.isChecked()) {
                message = this.getResources().getString(
                        R.string.vtoption_off_message);
            }

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setTitle(this.getResources().getString(R.string.confirm));
            builder.setMessage(message);
            builder.setNegativeButton(this.getResources().getString(R.string.vt_dialog_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            mVTOptionPreference.setChecked(!mVTOptionPreference
                                    .isChecked());
                            Log.d(TAG,
                                    "setNegativeButton, mVTOptionPreference: "
                                            + mVTOptionPreference.isChecked());
                        }
                    });
            builder.setPositiveButton(this.getResources().getString(R.string.vt_dialog_ok),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            SystemProperties.set("persist.sys.support.vt",
                                    mVTOptionPreference.isChecked() ? "true"
                                            : "false");
                            Log.d(TAG, "setPositiveButton,VT open: "
                                    + mVTOptionPreference.isChecked());
                            pm.reboot(null);
                        }
                    });
            builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
                @Override
                public void onCancel(DialogInterface arg0) {
                    // TODO Auto-generated method stub
                    mVTOptionPreference.setChecked(!mVTOptionPreference
                            .isChecked());
                    Log.d(TAG, "Cancel Button, mVTOptionPreference: "
                            + mVTOptionPreference.isChecked());
                }
            });
            builder.show();
        }else if (pref == mBackgroundSearch) {
            Message m = mTELHandler.obtainMessage(BACKGROUNDSEARCH, newValue);
            mTELHandler.sendMessage(m);
            return false;
        } else if (pref == mLTE) {
            Message m = mTELHandler.obtainMessage(LTESET, newValue);
            mTELHandler.sendMessage(m);
            return false;
        } else if (pref == mRSRP) {
            Message m = mTELHandler.obtainMessage(RSRPSET, newValue);
            mTELHandler.sendMessage(m);
            return false;
        } else if (pref == mLTEDateImpede) {
            int set_flag = mLTEDateImpede.isChecked() ? 0 : 1;
            Message msg = mTELHandler.obtainMessage(LTE_SET_DATA_IMPEDE,
                    set_flag, 0);
            mTELHandler.sendMessage(msg);
        } else if (pref == mDataSwitch) {
            if (mDataSwitch.isChecked()) {
                mDataSwitch.setChecked(false);
                SystemProperties.set("persist.sys.data.restore", "false");
            } else {
                mDataSwitch.setChecked(true);
                SystemProperties.set("persist.sys.data.restore", "true");
            }
        } else if (pref == mVolteSwitch) {
            if (mVolteSwitch.isChecked()) {
                mVolteSwitch.setChecked(false);
                mVolteChecked = false;
                mVolteSwitch.setSummary("Close");
                AlertDialogShow(KEY_VOLTE_SWITCH);
            } else {
                mVolteSwitch.setChecked(true);
                mVolteChecked = true;
                mVolteSwitch.setSummary("Open");
                AlertDialogShow(KEY_VOLTE_SWITCH);
            }
        } else if (pref == mImsSwitch) {
            if (mImsSwitch.isChecked()) {
                mImsSwitch.setChecked(false);
                SystemProperties.set("persist.sys.ims.visibility", "false");
                mImsSwitch.setSummary("Close");
            } else {
                mImsSwitch.setChecked(true);
                SystemProperties.set("persist.sys.ims.visibility", "true");
                mImsSwitch.setSummary("Open");
            }
        } else if (pref == mSmsOverIpSwitch) {
          //begin 550429 add by suyan.yang 2016.05.10
            if (mSmsOverIpSwitch.isChecked()) {
                mSmsOverIpSwitch.setChecked(false);
                mSmsChecked = false;
            } else {
                mSmsOverIpSwitch.setChecked(true);
                mSmsChecked = true;
            }
            AlertDialogShow(KEY_SMS_SWITCH);
          //end 550429 add by suyan.yang 2016.05.10
        } else if (pref == mLoadAPNSwitch) {
            if (mLoadAPNSwitch.isChecked()) {
                SystemProperties.set("persist.sys.loaded.apn", "false");
            } else {
                SystemProperties.set("persist.sys.loaded.apn", "true");
            }
        } else if (pref == mDdrSwitch) {
            if (mDdrSwitch.isChecked()) {
                SystemProperties.set(DDR_SWITCH_STATUS, "0");
                Log.d(TAG, "close ddr switch");
            } else {
                SystemProperties.set(DDR_SWITCH_STATUS, "1");
                Log.d(TAG, "open ddr switch");
            }
        } else if (pref == mThermal) {
            if (mThermal.isChecked()) {
                mThermalInter.thermalEnabled(false);
            } else {
                mThermalInter.thermalEnabled(true);
            }
            updateThermalStatus();
        }
        /* SPRD: Feature for Uplmn @{ */
        else if(pref == mUplmnSwitch){
            if (mUplmnSwitch.isChecked()) {
                mUplmnSwitch.setChecked(false);
                Log.d(TAG, "set persist.sys.uplmn.enable false" );
                SystemProperties.set("persist.sys.uplmn.enable", "false");
            } else {
                mUplmnSwitch.setChecked(true);
                Log.d(TAG, "set persist.sys.uplmn.enable true" );
                SystemProperties.set("persist.sys.uplmn.enable", "true");
            }
        }
        /* @} */
/*        else if (pref == mSessionTimer){
             if("".equals(newValue.toString())){
                 Toast.makeText(mContext, "warning:empty value",
                         Toast.LENGTH_SHORT).show();
                 return false;
             }
            Message m = mTELHandler.obtainMessage(SET_SESSION_TIMER, newValue);
            mTELHandler.sendMessage(m);
        }else if (pref == mForceMtSessionTimer){
            //begin bug572397 add by suyan.yang 20160620
            if ("".equals(newValue.toString())) {
                Toast.makeText(mContext, "warning:empty value",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
            Message m = mTELHandler.obtainMessage(SET_FORCE_MT_SESSION_TIMER, newValue);
            mTELHandler.sendMessage(m);
            //end bug572397 add by suyan.yang 20160620
        }*/else if (pref == mMosSwitch) {
            /*
             * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
             * add NXP list and MOS switch @{
             */
            int set_flag = mMosSwitch.isChecked() ? 0 : 1;
            Log.d(TAG,"set_flag is "+set_flag);
            Message msg = mTELHandler.obtainMessage(SET_MOS_STATUS,
                set_flag, 0);
            mTELHandler.sendMessage(msg);
            /* @} */
        } else if (pref == mThermalPa) {
            if (mThermalPa.isChecked()) {
                mThermalInter.thermalPaEnabled(false);
            } else {
                mThermalInter.thermalPaEnabled(true);
            }
            updateThermalPaStatus();
         } else {
            return false;
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        // public boolean onPreferenceClick(Preference pref){
        String key = preference.getKey();
        int sim = 0;
        int messType = 0;
        Log.d(TAG, "key is " + key);
        if (key == null) {
            return false;
        }
        for (int i = 0; i < mPhoneCount; i++) {
            if (key.equals(mBandSelectSim[i].getKey())) {
                Log.d(TAG, "onPreferenceTreeClick() sim:" + sim);
                Intent intent = new Intent("com.sprd.engineermode.action.BANDMODESET");
                intent.putExtra(KEY_PHONEID, i);
                startActivity(intent);
                break;
            }
        }
        /* SPRD: fix bug346409 Engnieer mode crash @{ */
        if (key.equals("net_info")) {
            Intent intent = new Intent(
                    "android.intent.action.SIMSELECT_NETINFO");
            mContext.startActivity(intent);
        }
        /* @} */
        if (key.equals("gprs_set")) {
            boolean bContinue = false;
            for (int i = 0; i < mPhoneCount; i++) {
                mTelephonyManager[i] = (TelephonyManager) TelephonyManager
                        .from(mContext);
                if (mTelephonyManager[i] != null
                        && mTelephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                    bContinue = true;
                    break;
                }
            }
            if (bContinue) {
                Intent intent = new Intent("android.intent.action.GPRSSET");
                mContext.startActivity(intent);
            } else {
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mContext, getString(R.string.sim_invalid),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
        /*
         * SPRD: modify 20140701 Spreadtrum of 328867 EngineerMode,there is no "NetworkMode" options
         * in Telephony @{
         */
        if (key.equals(KEY_NETMODE_SELECT)) {
            if (isSupportLTE) {
                AlertDialog alertDialog = new AlertDialog.Builder(mContext)
                        .setTitle(getString(R.string.network_mode))
                        .setMessage(
                                getString(R.string.switch_mode_warn_message))
                        .setPositiveButton(getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        Intent intent = new Intent(
                                                "android.intent.action.LTEActivity");
                                        intent.putExtra(KEY_SIM_INDEX, mPhoneId);
                                        mContext.startActivity(intent);
                                    }
                                })
                        .setNegativeButton(R.string.alertdialog_cancel,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                    }
                                }).create();
                alertDialog.show();
            } else {
                Intent intent = new Intent(
                        "android.intent.action.NetModeSelection");
                intent.putExtra(KEY_SIM_INDEX, 0);
                mContext.startActivity(intent);
            }
        }
        /* modify 20140701 Spreadtrum of 328867@} */
        if (key.equals(KEY_NETINFO_STATI)) {
            if (mPhoneCount > 1) {
                Intent intent = new Intent(
                        "android.intent.action.SIMSELECT_NETSTATI");
                mContext.startActivity(intent);
            } else if (mPhoneCount == 1) {
                Intent intent = new Intent("android.intent.action.NETINFOSTATI");
                intent.putExtra(KEY_SIM_INDEX, 0);
                mContext.startActivity(intent);
            }
        } else if (key.equals(KEY_SIMLOCK_NETWORK)
                || KEY_SIMLOCK_NETWORK_SUBSET.equals(key)
                || KEY_SIMLOCK_SERVICE.equals(key)
                || KEY_SIMLOCK_CORPORATE.equals(key)
                || KEY_SIMLOCK_SIM.equals(key)) {
            Intent intent = new Intent(getActivity(), SimLockOpActivity.class);
            intent.putExtra(KEY_SIMLOCK_TYPE, key);
            startActivity(intent);
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    class TELHandler extends Handler {

        public TELHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String gRespLock = null;
            switch (msg.what) {
                case SET_WPREFER_PLUS_STATUS:
                    mStrTmp = IATUtils.sendATCmd(
                            engconstents.ENG_SET_WPREFER_SWITCH + msg.arg1,
                            "atchannel0");
                    mMesarg = msg.arg1;
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mMesarg == 1) {
                                    mWpreferplus.setChecked(true);
                                } else {
                                    mWpreferplus.setChecked(false);
                                }
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mMesarg == 1) {
                                    mWpreferplus.setChecked(false);
                                } else {
                                    mWpreferplus.setChecked(true);
                                }
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case GET_SIM_TRACE_STATUS:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_GET_SIM_TRACE,
                            "atchannel0");
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)
                            && mStrTmp.contains("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(false);
                            }
                        });
                    }
                    break;
                case OPEN_SIM_TRACE:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SIM_TRACE
                            + "1", "atchannel0");
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(false);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case CLOSE_SIM_TRACE:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SIM_TRACE
                            + "0", "atchannel0");
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSimTrace.setChecked(true);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case SET_MODE_SWITCH:
                    String modeStr = (String) msg.obj;
                    setModeSwitch(Integer.parseInt(modeStr));
                    PowerManager pm = (PowerManager) mContext
                            .getSystemService(Context.POWER_SERVICE);
                    pm.reboot("setModeSwitch");
                    break;
                case SET_SINGLE_DUAL_MODE_SWITCH:
                    String singleDualSIMModeStr = (String) msg.obj;

                    /*
                     * Reset phoneId values of all SIM information when switch single-dual SIM mode
                     * to make sure applications can get correct amount of active SIM information
                     * after reboot.
                     */
                    ContentValues phoneIdValue = new ContentValues(1);
                    phoneIdValue.put(SubscriptionManager.SIM_SLOT_INDEX,
                            SubscriptionManager.INVALID_SIM_SLOT_INDEX);
                    mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI,
                            phoneIdValue, null, null);

                    setSingeDualSIMModeSwitch(Integer.parseInt(singleDualSIMModeStr));
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Thread.sleep(2000);
                            } catch (InterruptedException e) {

                            }
                            PowerManager powerManager = (PowerManager) mContext
                                    .getSystemService(Context.POWER_SERVICE);
                            powerManager.reboot(null);
                        }
                    });
                    break;
                case GET_WPREFER_PLUS_STATUS:
                    mStrTmp = IATUtils.sendATCmd(
                            engconstents.ENG_GET_WPREFER_SWITCH, "atchannel0");
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)
                            && mStrTmp.contains("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mWpreferplus.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mWpreferplus.setChecked(false);
                            }
                        });
                    }
                    break;

                case SET_CFU: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(CFU_CONTROL, valueStr);
                    break;
                }
                case SET_VIDEOTYPE: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(VALUEKEY_VIDEO_TYPE, valueStr);
                }
                    break;
                case OPEN_AUTO_ANSWER: {
                    mATCmd = engconstents.ENG_AUTO_ANSWER + 1;
                    Log.d(TAG, "OPEN_AUTO_ANSWER");
                    mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    if (mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoAnswer.setChecked(true);
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoAnswer.setChecked(false);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                }
                case GET_AUTO_ANSWER: {
                    mATCmd = engconstents.ENG_GET_AUTO_ANSWER;
                    Log.d(TAG, "GET_AUTO_ANSWER_STATE");
                    mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, mATCmd + ": " + mStrTmp);
                    if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                        String[] str0 = mStrTmp.split("\n");
                        if (str0[0].contains("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mAutoAnswer.setChecked(true);
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mAutoAnswer.setChecked(false);
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoAnswer.setEnabled(false);
                                mAutoAnswer.setSummary(R.string.feature_abnormal);
                                mAutoAnswer.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case CLOSE_AUTO_ANSWER: {
                    mATCmd = engconstents.ENG_AUTO_ANSWER + 0;
                    Log.d(TAG, "CLOSE_AUTO_ANSWER");
                    mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    if (mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoAnswer.setChecked(false);
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoAnswer.setChecked(true);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                }
                case SET_FAST_DORMANCY:
                    final String setNum = (String) msg.obj;
                    String cmd = engconstents.ENG_AT_SPSETFDY + msg.arg1;
                    final int arg1 = msg.arg1;

                    if (msg.obj != null) {
                        cmd += "," + setNum;
                    }
                    mStrTmp = IATUtils.sendATCmd(cmd, "atchannel0");
                    if (mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                                if (arg1 == 1)
                                    mFastdormancy.setSummary(setNum + "s");
                                else
                                    mFastdormancy.setSummary(setNum);
                                // mDromTime = setNum;
                                editor.putString(DORM_TIME, setNum);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case OPEN_DATA_SERVICES_PRE:
                    mATCmd = engconstents.ENG_DATA_SERVICES_PRE + 1;
                    Log.d(TAG, "OPEN_DATA_SERVICES_PRE");
                    mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    if (mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataServicePreferred.setChecked(true);
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataServicePreferred.setChecked(false);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case CLOSE_DATA_SERVICES_PRE:
                    mATCmd = engconstents.ENG_DATA_SERVICES_PRE + 0;
                    Log.d(TAG, "CLOSE_DATA_SERVICES_PRE");
                    mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    if (mStrTmp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataServicePreferred.setChecked(false);
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDataServicePreferred.setChecked(true);
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();
                            }
                        });
                    }
                    break;
                case BACKGROUNDSEARCH:
                    mSetNum = msg.obj.toString();
                    if ((mSetNum.length() == 0) || (Integer.parseInt(mSetNum) < 1)
                            || (Integer.parseInt(mSetNum) > 3600)) {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                Toast.makeText(mContext,
                                        "number between 1 and 3600",
                                        Toast.LENGTH_SHORT).show();
                            }

                        });
                        return;
                    }
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_LTEBGTIMER
                            + "0," + mSetNum + "," + 0, "atchannel0");
                    if (mATResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mBackgroundSearch.setSummary(mSetNum.replaceAll(
                                        "^(0+)", ""));
                                SharedPreferences pref = PreferenceManager
                                        .getDefaultSharedPreferences(mContext);
                                Editor editor = pref.edit();
                                editor.putString(KEY_BACKGROUNDSEARCH, mSetNum);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();

                            }
                        });
                    }
                    break;
                case LTESET:
                    mSetNum = msg.obj.toString();
                    if ((mSetNum.length() == 0) || (Integer.parseInt(mSetNum) < 1)
                            || (Integer.parseInt(mSetNum) > 3600)) {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                Toast.makeText(mContext,
                                        "number between 1 and 3600",
                                        Toast.LENGTH_SHORT).show();
                            }

                        });
                        return;
                    }
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_LTEBGTIMER
                            + "1," + mSetNum, "atchannel0");
                    if (mATResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                                mLTE.setSummary(mSetNum.replaceAll("^(0+)", ""));
                                SharedPreferences pref = PreferenceManager
                                        .getDefaultSharedPreferences(mContext);
                                Editor editor = pref.edit();
                                editor.putString(KEY_TIMER, mSetNum);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();

                            }
                        });
                    }
                    break;
                case RSRPSET:
                    mSetNum = msg.obj.toString();
                    if ((mSetNum.length() == 0) || (Integer.parseInt(mSetNum) < 0)
                            || (Integer.parseInt(mSetNum) > 97)) {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                Toast.makeText(mContext, "number between 0 and 97",
                                        Toast.LENGTH_SHORT).show();
                            }

                        });
                        return;
                    }
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_LTESETRSRP
                            + mSetNum, "atchannel0");
                    if (mATResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Success",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                if (Integer.parseInt(mSetNum) == 0) {
                                    mRSRP.setSummary("0");
                                } else {
                                    mRSRP.setSummary(mSetNum
                                            .replaceAll("^(0+)", ""));
                                }
                                SharedPreferences pref = PreferenceManager
                                        .getDefaultSharedPreferences(mContext);
                                Editor editor = pref.edit();
                                editor.putString(KEY_RSRP, mSetNum);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT)
                                        .show();

                            }
                        });
                    }
                    break;
                case LTE_GET_DATA_IMPEDE:
                    mATCmd = engconstents.ENG_AT_GET_LTE_DATA_IMPEDE;
                    Log.d(TAG, "LTE_GET_DATA_IMPEDE" + "  " + "mATCmd: " + mATCmd);
                    mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "LTE_GET_DATA_IMPEDE" + "  " + "mATResponse: "
                            + mATResponse);
                    if (mATResponse.contains(IATUtils.AT_OK)) {
                        String[] result = mATResponse.split("\n");
                        String[] result1 = result[0].split(":");
                        String atResponse = result1[1].trim().substring(2);
                        int value = Integer.parseInt(atResponse, 16);
                        Log.d(TAG, "atResponse: " + atResponse + "value: " + value);
                        if ((value & 0x01) > 0) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLTEDateImpede.setChecked(true);
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLTEDateImpede.setChecked(false);
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLTEDateImpede.setEnabled(false);
                            }
                        });
                    }
                    break;
            case LTE_SET_DATA_IMPEDE:
                int set_flag = msg.arg1;
                mATCmd = engconstents.ENG_AT_SET_LTE_DATA_IMPEDE;
                Log.d(TAG, "LTE_SET_DATA_IMPEDE" + "  " + "mATCmd: " + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd + set_flag + ",1",
                        "atchannel0");
                Log.d(TAG, "LTE_SET_DATA_IMPEDE" + "  " + "mATResponse: "
                        + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Failed",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            /*
             * SPRD: wp modify 20140621 Spreadtrum of 325713 EngineerMode,
             * telephony-LTE modem add Clear Prior Information@{
             */
            case CPINFO:
                mATCmd = engconstents.ENG_AT_CLEAR_PRIOR;
                Log.d(TAG, "CLEAR_PRIOR_INFORMATION" + "  " + "mATCmd: "
                        + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel");
                Log.d(TAG, "CLEAR_PRIOR_INFORMATION" + "  " + "mATResponse: "
                        + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Clear Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Clear Failed",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                /* @} */

              //begin 550429 add by suyan.yang 2016.05.10
            case GET_SMS_OVER_IP_STATUS:
                mStrTmp = IATUtils.sendATCmd(
                        engconstents.ENG_GET_SMS_OVER_IP_SWITCH, "atchannel0");
                Log.d(TAG, "GET_SMS_OVER_IP_STATUS" + "  " + "mStrTmp: "
                        + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    if (mStrTmp.contains("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSmsOverIpSwitch.setChecked(true);
                            }
                        });
                     }else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSmsOverIpSwitch.setChecked(false);
                            }
                        });
                    }
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mSmsOverIpSwitch.setEnabled(false);
                            mSmsOverIpSwitch.setSummary(R.string.feature_not_support);
                        }
                    });
                }
                break;
            case SET_SMS_OVER_IP_STATUS:
                int set_value = msg.arg1;
                mATCmd = engconstents.ENG_SET_SMS_OVER_IP_SWITCH;
                Log.d(TAG, "SET_SMS_OVER_IP_STATUS" + "  " + "mATCmd: " + mATCmd + set_value);
                mATResponse = IATUtils.sendATCmd(mATCmd + set_value,
                        "atchannel0");
                Log.d(TAG, "SET_SMS_OVER_IP_STATUS" + "  " + "mATResponse: "
                        + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Success",
                                    Toast.LENGTH_SHORT).show();
                            PowerManager pm = (PowerManager) mContext
                                    .getSystemService(Context.POWER_SERVICE);
                            pm.reboot("setSmsOverIpSwitch");
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "Failed",
                                    Toast.LENGTH_SHORT).show();
                            mSmsOverIpSwitch.setChecked(mSmsChecked ? false : true);
                        }
                    });
                }
                break;
              //end 550429 add by suyan.yang 2016.05.10
/*            case GET_SESSION_TIMER:
                mATCmd = engconstents.ENG_GET_SESSION_TIMER;
                Log.d(TAG, "GET_SESSION_TIMER" + "  " + "mATCmd: " + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd,
                        "atchannel0");
                Log.d(TAG, "GET_SESSION_TIMER" + "  " + "mATResponse: "
                         + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    String[] str=mATResponse.split("\"");
                    if(str.length>1){
                        final String time=str[1];
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSessionTimer.setSummary(time+"s");
                            }
                        });
                    }
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG,"GET_SESSION_TIMER Failed");
                            mSessionTimer.setSummary(mContext.getString(R.string.feature_not_support));
                            mSessionTimer.setEnabled(false);
                        }
                    });
                }
                break;
            case SET_SESSION_TIMER:
                final int time = Integer.valueOf(msg.obj.toString());
                mATCmd = engconstents.ENG_SET_SESSION_TIMER+"\""+time+"\"";
                Log.d(TAG, "SET_SESSION_TIMER" + "  " + "mATCmd: " + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd,
                        "atchannel0");
                Log.d(TAG, "SET_SESSION_TIMER" + "  " + "mATResponse: "
                        + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mSessionTimer.setSummary(time+"s");
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "SET_SESSION_TIMER Failed",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            //begin bug572397 add by suyan.yang 20160620
            case GET_FORCE_MT_SESSION_TIMER:
                mATCmd = engconstents.ENG_GET_FORCE_MT_SESSION_TIMER;
                Log.d(TAG, "GET_FORCE_MT_SESSION_TIMER" + "  " + "mATCmd: " + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd,
                        "atchannel0");
                Log.d(TAG, "GET_FORCE_MT_SESSION_TIMER" + "  " + "mATResponse: "
                         + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    String[] str=mATResponse.split("\"");
                    if(str.length>1){
                        final String[] strr=str[1].split(";");
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mForceMtSessionTimer.setSummary(strr[0]+"s");
                            }
                        });
                    }
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG,"GET_FORCE_MT_SESSION_TIMER Failed");
                            mForceMtSessionTimer.setSummary(mContext.getString(R.string.feature_not_support));
                            mForceMtSessionTimer.setEnabled(false);
                        }
                    });
                }
                break;
            case SET_FORCE_MT_SESSION_TIMER:
                final int force_mt_time = Integer.valueOf(msg.obj.toString());
                mATCmd = engconstents.ENG_SET_FORCE_MT_SESSION_TIMER+"\""+force_mt_time+"\"";
                Log.d(TAG, "SET_FORCE_MT_SESSION_TIMER" + "  " + "mATCmd: " + mATCmd);
                mATResponse = IATUtils.sendATCmd(mATCmd,
                        "atchannel0");
                Log.d(TAG, "SET_FORCE_MT_SESSION_TIMER" + "  " + "mATResponse: "
                        + mATResponse);
                if (mATResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mForceMtSessionTimer.setSummary(force_mt_time+"s");
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(mContext, "SET_FORCE_MT_SESSION_TIMER Failed",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;*/
            //begin bug572397 add by suyan.yang 20160620

              /*
                 * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
                 * @{
              */
              case GET_NXP_STATUS:
                   mATCmd = engconstents.ENG_AT_GET_NXP;
                   Log.d(TAG, "GET_NXP_STATUS" + " " + "mATCmd: " + mATCmd);
                   mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel1");
                   Log.d(TAG, "GET_NXP_STATUS" + " " + "mATResponse: "
                           + mATResponse);
                   if (mATResponse.contains(IATUtils.AT_OK)) {

                       status = analysisResult(mATResponse);
                       Log.d(TAG, "status size is" + status.length);

                       mUiThread.post(new Runnable() {
                           @Override
                           public void run() {
                               for (int i = 0; i < NXP_COUNT; i++) {
                                   if (status[i + 1].contains("1")) {
                                       mNxpList[i].setChecked(true);
                                   } else {
                                       mNxpList[i].setChecked(false);
                                   }
                               }
                           }
                       });
                   } else {
                       mUiThread.post(new Runnable() {
                          @Override
                           public void run() {
                              Toast.makeText(mContext, "get nxp status failed",
                                   Toast.LENGTH_SHORT).show();
                           }
                       });
                   }
              break;
              case GET_MOS_STATUS:
                   mATCmd = engconstents.ENG_AT_GET_MOS_STATUS;
                   Log.d(TAG, "GET_MOS_STATUS" + " " + "mATCmd: " + mATCmd);
                   mATResponse = IATUtils.sendATCmd(mATCmd, "atchannel1");
                   Log.d(TAG, "GET_MOS_STATUS" + " " + "mATResponse: "
                           + mATResponse);
                           if (mATResponse.contains(IATUtils.AT_OK)) {
                               String str[] = mATResponse.split("\n");
                               final String s = str[0].substring(str[0].length() - 2);
                               Log.d(TAG, s);
                               mUiThread.post(new Runnable() {
                                   @Override
                                   public void run() {
                                       if (s.contains("1")) {
                                           mMosSwitch.setChecked(true);
                                           mMosSwitch
                                           .setSummary(R.string.mos_test_mode);
                                       } else {
                                           mMosSwitch.setChecked(false);
                                           mMosSwitch
                                           .setSummary(R.string.not_mos_test_mode);
                                       }
                                   }
                               });
                           } else {
                               mUiThread.post(new Runnable() {
                                   @Override
                                   public void run() {
                                       Log.d(TAG,"GET_MOS_STATUS Failed");
                                       mMosSwitch.setSummary(mContext.getString(R.string.feature_not_support));
                                       mMosSwitch.setEnabled(false);
                                   }
                               });
                           }
               break;
               case SET_MOS_STATUS:
                   set_flag = msg.arg1;
                   mATCmd = engconstents.ENG_AT_SET_MOS_STATUS;
                   Log.d(TAG, "SET_MOS_STATUS" + " " + "mATCmd: " + mATCmd);
                   mATResponse = IATUtils.sendATCmd(mATCmd + "," + set_flag,
                           "atchannel1");
                   Log.d(TAG, "SET_MOS_STATUS" + " " + "mATResponse: "
                           + mATResponse);
                   final int mode=set_flag;
                   if (mATResponse.contains(IATUtils.AT_OK)) {
                       mUiThread.post(new Runnable() {
                           @Override
                           public void run() {
                               Toast.makeText(mContext, "set mos status success",
                                       Toast.LENGTH_SHORT).show();
                               if(0==mode){
                                   mMosSwitch.setSummary(getString(R.string.not_mos_test_mode));
                               }else if(1==mode){
                                   mMosSwitch.setSummary(getString(R.string.mos_test_mode));
                               }
                           }
                       });
                   } else {
                       mUiThread.post(new Runnable() {
                           @Override
                           public void run() {
                               Toast.makeText(mContext, "set mos status failed",
                                       Toast.LENGTH_SHORT).show();
                               mMosSwitch.setChecked(mMosSwitch.isChecked() ? false : true);
                           }
                       });
                   }
               break;
               case SET_NXP_STATUS:
                   nxp_index = msg.arg1;
                   mATCmd = engconstents.ENG_AT_SET_NXP_STATUS;
                   Log.d(TAG, "SET_NXP_STATUS" + " " + "mATCmd: " + mATCmd);
                   int setValue = mNxpList[nxp_index - 1].isChecked() ? 1 : 0;
                   mATResponse = IATUtils.sendATCmd(mATCmd + "," + nxp_index + ","
                           + setValue, "atchannel1");
                   Log.d(TAG, "SET_NXP_STATUS" + " " + "mATResponse: "
                           + mATResponse);
                   if (mATResponse.contains(IATUtils.AT_OK)) {
                       if (setValue == 1) {
                           mUiThread.post(new Runnable() {
                               @Override
                               public void run() {
                                   mNxpList[nxp_index - 1].setChecked(true);
                               }
                           });
                       } else {
                           mUiThread.post(new Runnable() {
                               @Override
                               public void run() {
                                   mNxpList[nxp_index - 1].setChecked(false);
                               }
                           });
                       }
                   } else {
                       mUiThread.post(new Runnable() {
                           @Override
                           public void run() {
                               Toast.makeText(mContext, "set mos status failed",
                                       Toast.LENGTH_SHORT).show();
                           }
                       });
                   }
               break;
               /* @} */
            default:
                break;
            }
        }
    }

    /*
     * SPRD: modify 20160707 Spreadtrum of 578851 EngineerMode,
     * add analysisResult function to analysis the return value of "AT+SPTEST=30,0" @{
     */
     private String[] analysisResult(String result){
         String[] str=result.split("\n");
         return str[0].split(",");
     }
     /* @}*/

     private OnPreferenceClickListener clickListenser= new OnPreferenceClickListener() {
         @Override
         public boolean onPreferenceClick(Preference preference) {
             Log.d(TAG,"clickListenser");
             for(int i=0;i<NXP_COUNT;i++){
                 if(preference==mNxpList[i]){
                     Message msg = mTELHandler.obtainMessage(SET_NXP_STATUS,
                             i+1, 0);
                     mTELHandler.sendMessage(msg);
                     break;
                 }
             }
             return false;
         }
    };

    private void showProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                mProgressDialog = ProgressDialog.show(mContext, "Query...",
                        "Please wait...", true, false);
            }
        });
    }

    private void dismissProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                }
            }
        });
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        // outState.putString(DORM_TIME, mDromTime);
    }

    public void AlertDialogShow(final String key) {
        AlertDialog alertDialog = new AlertDialog.Builder(getActivity())
                .setMessage(getString(R.string.volte_switch_waring))
                .setCancelable(false)
                .setPositiveButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                              //begin 550429 modify by suyan.yang 2016.05.10
                                if(key.equals(KEY_VOLTE_SWITCH)){
                                    if (mVolteChecked) {
                                        SystemProperties.set(
                                                "persist.sys.volte.enable",
                                                "true");
                                    } else {
                                        SystemProperties.set(
                                                "persist.sys.volte.enable",
                                                "false");
                                    }
                                    PowerManager pm = (PowerManager) mContext
                                          .getSystemService(Context.POWER_SERVICE);
                                    pm.reboot("setVolteSwitch");
                                }else if(key.equals(KEY_SMS_SWITCH)){
                                    int set_flag = mSmsChecked ? 1 : 0;
                                    Message msg = mTELHandler.obtainMessage(SET_SMS_OVER_IP_STATUS,
                                            set_flag, 0);
                                    mTELHandler.sendMessage(msg);
                                }
                              //end 550429 modify by suyan.yang 2016.05.10
                            }
                        })
                .setNegativeButton(R.string.alertdialog_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                mUiThread.post(new Runnable() {
                                    @Override
                                    public void run() {
                                      //begin 550429 modify by suyan.yang 2016.05.10
                                        if(key.equals(KEY_VOLTE_SWITCH)){
                                            if (mVolteChecked) {
                                                mVolteSwitch.setChecked(false);
                                                mVolteChecked = false;
                                                mVolteSwitch.setSummary("Close");
                                            } else {
                                                mVolteSwitch.setChecked(true);
                                                mVolteChecked = true;
                                                mVolteSwitch.setSummary("Open");
                                            }
                                        }else if(key.equals(KEY_SMS_SWITCH)){
                                            mSmsOverIpSwitch.setChecked(mSmsChecked ? false : true);
                                        }
                                      //begin 550429 modify by suyan.yang 2016.05.10
                                    }
                                });
                            }
                        }).create();
        alertDialog.show();
    }

    private boolean isAppInstalled(Context context, String packagename) {
        PackageInfo packageInfo;
        try {
            packageInfo = context.getPackageManager().getPackageInfo(packagename, 0);
        } catch (NameNotFoundException e) {
            packageInfo = null;
            e.printStackTrace();
        }
        if (packageInfo == null) {
            return false;
        } else {
            return true;
        }
    }
}
