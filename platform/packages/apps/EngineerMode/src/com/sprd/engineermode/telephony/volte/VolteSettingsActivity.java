
package com.sprd.engineermode.telephony.volte;

import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceCategory;
import android.preference.TwoStatePreference;
import android.preference.CheckBoxPreference;
import com.sprd.engineermode.EMSwitchPreference;
import android.preference.EditTextPreference;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.content.SharedPreferences;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.util.Log;
import android.widget.Toast;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.content.Context;
import android.os.Message;
import android.telephony.TelephonyManager;
import android.content.DialogInterface;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.widget.EditText;
import android.app.AlertDialog;
import android.view.View;
import android.content.DialogInterface;
import android.os.SystemProperties;
import com.sprd.engineermode.R;
import android.os.PowerManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class VolteSettingsActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener {

    private static final String TAG = "VolteSettingsActivity";
    private static final String KEY_CUSTOM_IMPI_SETTING = "volte_impi_impu_sub_setting";
    private static final String KEY_IP_SETTING = "volte_ip_setting";
    private static final String KEY_MUT_LENGTH = "volte_mut_length";
    private static final String KEY_SUBSCRIBE_SETTING = "volte_subscribe_setting_switch";
    private static final String KEY_SIGCOMP_SETTING = "volte_sigcomp_setting_switch";
    private static final String KEY_SIGCOMP_ALGORITHM = "volte_sigcomp_algorithm";
    private static final String KEY_EXPIRES_TIMER_SETTING = "volte_expires_time_setting";
    private static final String KEY_VOICE_CODE_WAY = "volte_voice_code_way";
    private static final String KEY_VOICE_CODE_SPEED = "volte_voice_code_speed";
    /* sharepref save key AMR_WB Speed and AMR_NB Speed @{ */
    private static final String KEY_VOICE_NB_CODE_SPEED = "volte_voice_nb_code_speed";
    private static final String KEY_VOICE_WB_CODE_SPEED = "volte_voice_nb_code_speed";
    /* }@ */
    private static final String KEY_PRECONDITION_SETTING = "volte_precondition_setting_switch";
    private static final String KEY_TQOS_TIMER_SETTING = "volte_tqos_time_setting";
    private static final String KEY_TCALL_TIMER_SETTING = "volte_tcall_time_setting";
    private static final String KEY_TREG_TIMER_SETTING = "volte_treg_time_setting";

    private static final String KEY_IMPI_SETTING = "impi_setting";
    private static final String KEY_IMPU_SETTING = "impu_setting";
    private static final String KEY_DOMAIN_SETTING = "domain_setting";

    private static final String KEY_PCSCF_SET = "pcscf_set";
    private static final String KEY_SESSION_TIMER="session_timer";
    private static final String KEY_FORCE_MT_SESSION_TIMER="force_mt_session_timer";

    private static final int MSG_SET_IMPI = 0;
    private static final int MSG_SET_IMPU = 1;
    private static final int MSG_SET_DOMAIN = 2;
    private static final int MSG_GET_IP_STATE = 3;
    private static final int MSG_SET_IP_STATE = 4;
    private static final int MSG_GET_MUT_LENGTH = 5;
    private static final int MSG_SET_MUT_LENGTH = 6;
    private static final int MSG_GET_SUBSCRIB_STATE = 7;
    private static final int MSG_SET_SUBSCRIB_STATE = 8;
    private static final int MSG_GET_SIGCOMP_STATE = 9;
    private static final int MSG_SET_SIGCOMP_STATE = 10;
    private static final int MSG_SET_SIGCOMP_INDEX = 11;
    private static final int MSG_GET_EXPIRES_TIME = 12;
    private static final int MSG_SET_EXPIRES_TIME = 13;
    private static final int MSG_GET_VOICE_CODE_WAY = 14;
    private static final int MSG_SET_VOICE_CODE_WAY = 15;
    private static final int MSG_SET_VOICE_CODE_SPEED = 16;
    private static final int MSG_GET_PRECONDITION_STATE = 17;
    private static final int MSG_SET_PRECONDITION_STATE = 18;
    private static final int MSG_GET_TQOS_TIMER = 19;
    private static final int MSG_SET_TQOS_TIMER = 20;
    private static final int MSG_GET_TCALL_TIMER = 21;
    private static final int MSG_SET_TCALL_TIMER = 22;
    private static final int MSG_GET_TREG_TIMER = 23;
    private static final int MSG_SET_TREG_TIMER = 24;
    private static final int MSG_GET_AUTHTYPE = 25;
    private static final int MSG_SET_AUTHTYPE = 26;
    private static final int MSG_GET_VOICE_CODE_TYPE = 27;
    private static final int MSG_SET_VOICE_CODE_TYPE = 28;
    private static final int MSG_GET_MIN_BANDWIDTH = 29;
    private static final int MSG_SET_MIN_BANDWIDTH = 30;
    private static final int MSG_GET_MAX_BANDWIDTH = 31;
    private static final int MSG_SET_MAX_BANDWIDTH = 32;
    private static final int MSG_GET_MIN_BITRATE = 33;
    private static final int MSG_SET_MIN_BITRATE = 34;
    private static final int MSG_GET_MAX_BITRATE = 35;
    private static final int MSG_SET_MAX_BITRATE = 36;
    private static final int MSG_GET_DEFAULT_BITRATE = 37;
    private static final int MSG_SET_DEFAULT_BITRATE = 38;
    private static final int MSG_GET_EVS_DEFAULT_BITRATE = 39;
    private static final int MSG_SET_EVS_DEFAULT_BITRATE = 40;
    private static final int MSG_GET_WB_DEFAULT_BITRATE = 41;
    private static final int MSG_SET_WB_DEFAULT_BITRATE = 42;
    private static final int MSG_GET_NB_DEFAULT_BITRATE = 43;
    private static final int MSG_SET_NB_DEFAULT_BITRATE = 44;
    private static final int MSG_GET_EVS_RATE_SET = 45;
    private static final int MSG_SET_EVS_RATE_SET = 46;
    private static final int MSG_GET_WB_RATE_SET = 47;
    private static final int MSG_SET_WB_RATE_SET = 48;
    private static final int MSG_GET_NB_RATE_SET = 49;
    private static final int MSG_SET_NB_RATE_SET = 50;
    private static final int MSG_GET_CHANNEL_AWARE_MODE = 51;
    private static final int MSG_SET_CHANNEL_AWARE_MODE = 52;
    private static final int MSG_GET_CHANNEL_AWARE_BANDWIDTH = 53;
    private static final int MSG_SET_CHANNEL_AWARE_BANDWIDTH = 54;
    private static final int GET_SESSION_TIMER = 55;
    private static final int SET_SESSION_TIMER = 56;
    private static final int GET_FORCE_MT_SESSION_TIMER = 57;
    private static final int SET_FORCE_MT_SESSION_TIMER = 58;
    private static final int EDITADRESS_LENGTH_MAX=91;
    
    private PreferenceGroup mIMPISettingScreen;
    private PreferenceGroup mAuthTypeSettingScreen;
    private ListPreference mIPSetting;
    private EditTextPreference mMUTLength;
    private TwoStatePreference mSubscribeSwitch;
    private TwoStatePreference mSigcompSwitch;
    private ListPreference mSigcompSetting;
    private EditTextPreference mExpiresTimer;
    private ListPreference mVoiceCodeWay;
    private ListPreference mVoiceCodeSpeed;
    private TwoStatePreference mPreconditionSwitch;
    private EditTextPreference mTqosTimer;
    private EditTextPreference mTcallTimer;
    private EditTextPreference mTregTimer;
    private EditTextPreference mSessionTimer,mForceMtSessionTimer;
    private TwoStatePreference mPcscfSwitch;

    private int mPhoneCount = 0;
    private String mResp = null;
    private int mPhoneId = 0;
    private String mNewValue = null;
    private String mLastValue = null;
    private Context mContext = null;
    private SharedPreferences mSharePref;
    private String mVoiceSpeedIndex = null;
    private String mVoiceWayIndex = null;
    private Handler mUiThread = new Handler();
    private VolteHandler mVolteHandler;
    private String mUsername = null;
    private String mPassword = null;

    private boolean isSupportLTE = TelephonyManagerSprd.RadioCapbility.TDD_CSFB
            .equals(TelephonyManagerSprd.getRadioCapbility())
            || TelephonyManagerSprd.RadioCapbility.FDD_CSFB.equals(TelephonyManagerSprd
                    .getRadioCapbility())
            || TelephonyManagerSprd.RadioCapbility.CSFB.equals(TelephonyManagerSprd
                    .getRadioCapbility())
            || TelephonyManagerSprd.RadioCapbility.TDD_SVLTE.equals(TelephonyManagerSprd
                    .getRadioCapbility());
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_volte_setting);
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mContext = this;
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mSharePref.registerOnSharedPreferenceChangeListener(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mVolteHandler = new VolteHandler(ht.getLooper());

        mIPSetting = (ListPreference) findPreference(KEY_IP_SETTING);
        mIPSetting.setOnPreferenceChangeListener(this);
        mMUTLength = (EditTextPreference) findPreference(KEY_MUT_LENGTH);
        mMUTLength.setOnPreferenceChangeListener(this);
        mSubscribeSwitch = (TwoStatePreference) findPreference(KEY_SUBSCRIBE_SETTING);
        mSubscribeSwitch.setOnPreferenceChangeListener(this);
        mSigcompSwitch = (TwoStatePreference) findPreference(KEY_SIGCOMP_SETTING);
        mSigcompSwitch.setOnPreferenceChangeListener(this);
        mSigcompSetting = (ListPreference) findPreference(KEY_SIGCOMP_ALGORITHM);
        mSigcompSetting.setOnPreferenceChangeListener(this);
        mSigcompSetting.setEnabled(false);
        mExpiresTimer = (EditTextPreference) findPreference(KEY_EXPIRES_TIMER_SETTING);
        mExpiresTimer.setOnPreferenceChangeListener(this);
        mVoiceCodeWay = (ListPreference) findPreference(KEY_VOICE_CODE_WAY);
        mVoiceCodeWay.setOnPreferenceChangeListener(this);
        mVoiceCodeSpeed = (ListPreference) findPreference(KEY_VOICE_CODE_SPEED);
        mVoiceCodeSpeed.setOnPreferenceChangeListener(this);
        mPreconditionSwitch = (TwoStatePreference) findPreference(KEY_PRECONDITION_SETTING);
        mPreconditionSwitch.setOnPreferenceChangeListener(this);
        mTqosTimer = (EditTextPreference) findPreference(KEY_TQOS_TIMER_SETTING);
        mTqosTimer.setOnPreferenceChangeListener(this);
        mTcallTimer = (EditTextPreference) findPreference(KEY_TCALL_TIMER_SETTING);
        mTcallTimer.setOnPreferenceChangeListener(this);
        mTregTimer = (EditTextPreference) findPreference(KEY_TREG_TIMER_SETTING);
        mTregTimer.setOnPreferenceChangeListener(this);
        mPcscfSwitch = (TwoStatePreference) findPreference(KEY_PCSCF_SET);
        mPcscfSwitch.setOnPreferenceChangeListener(this);
        mSessionTimer=(EditTextPreference)findPreference(KEY_SESSION_TIMER);
        mSessionTimer.setOnPreferenceChangeListener(this);
        mForceMtSessionTimer=(EditTextPreference)findPreference(KEY_FORCE_MT_SESSION_TIMER);
        mForceMtSessionTimer.setOnPreferenceChangeListener(this);

        mVoiceCodeWay = (ListPreference) findPreference(KEY_VOICE_CODE_WAY);
        mVoiceCodeWay.setOnPreferenceChangeListener(this);
        mVoiceCodeSpeed = (ListPreference) findPreference(KEY_VOICE_CODE_SPEED);
        mVoiceCodeSpeed.setOnPreferenceChangeListener(this);

        mIMPISettingScreen = (PreferenceGroup) findPreference(KEY_CUSTOM_IMPI_SETTING);
        for (int i = 0; i < mPhoneCount; i++) {
            PreferenceCategory prefCategory = new PreferenceCategory(this);
            prefCategory.setTitle("SIM" + i);
            mIMPISettingScreen.addPreference(prefCategory);
            EditTextPreference impisetting = new EditTextPreference(this);
            impisetting.setKey(KEY_IMPI_SETTING + i);
            impisetting.setTitle("IMPI Setting");
            impisetting.setDialogTitle("IMPI Setting");
            impisetting.setOnPreferenceChangeListener(this);
            if (mSharePref.getString(KEY_IMPI_SETTING + i, null) == null) {
                impisetting.setSummary(R.string.input);
            } else {
                impisetting.setSummary(mSharePref.getString(KEY_IMPI_SETTING + i, null));
            }
            mIMPISettingScreen.addPreference(impisetting);

            EditTextPreference impusetting = new EditTextPreference(this);
            impusetting.setKey(KEY_IMPU_SETTING + i);
            impusetting.setTitle("IMPU Setting");
            impusetting.setOnPreferenceChangeListener(this);
            if (mSharePref.getString(KEY_IMPU_SETTING + i, null) == null) {
                impusetting.setSummary(R.string.input);
            } else {
                impusetting.setSummary(mSharePref.getString(KEY_IMPU_SETTING + i, null));
            }
            mIMPISettingScreen.addPreference(impusetting);

            EditTextPreference domain = new EditTextPreference(this);
            domain.setKey(KEY_DOMAIN_SETTING + i);
            domain.setTitle("Domain Setting");
            domain.setOnPreferenceChangeListener(this);
            if (mSharePref.getString(KEY_DOMAIN_SETTING + i, null) == null) {
                domain.setSummary(R.string.input);
            } else {
                domain.setSummary(mSharePref.getString(KEY_DOMAIN_SETTING + i, null));
            }
            mIMPISettingScreen.addPreference(domain);
        }

    }

    @Override
    public void onStart() {
        if (mIPSetting != null && mIPSetting.isEnabled()) {
            Message getIPState = mVolteHandler.obtainMessage(MSG_GET_IP_STATE);
            mVolteHandler.sendMessage(getIPState);
        }

        if (mMUTLength != null && mMUTLength.isEnabled()) {
            Message getMutLength = mVolteHandler.obtainMessage(MSG_GET_MUT_LENGTH);
            mVolteHandler.sendMessage(getMutLength);
        }

        if (mSubscribeSwitch != null && mSubscribeSwitch.isEnabled()) {
            Message getSubscribeState = mVolteHandler.obtainMessage(MSG_GET_SUBSCRIB_STATE);
            mVolteHandler.sendMessage(getSubscribeState);
        }

        if (mSigcompSwitch != null && mSigcompSwitch.isEnabled()) {
            Message getSigcompSwitch = mVolteHandler.obtainMessage(MSG_GET_SIGCOMP_STATE);
            mVolteHandler.sendMessage(getSigcompSwitch);
        }

        if (mExpiresTimer != null && mExpiresTimer.isEnabled()) {
            Message getExpirestime = mVolteHandler.obtainMessage(MSG_GET_EXPIRES_TIME);
            mVolteHandler.sendMessage(getExpirestime);
        }

        if (mVoiceCodeWay != null && mVoiceCodeWay.isEnabled()) {
            Message getVoiceCodeWay = mVolteHandler.obtainMessage(MSG_GET_VOICE_CODE_WAY);
            mVolteHandler.sendMessage(getVoiceCodeWay);
        }
        
        if (mPreconditionSwitch != null && mPreconditionSwitch.isEnabled()){
            Message getPrecondition = mVolteHandler.obtainMessage(MSG_GET_PRECONDITION_STATE);
            mVolteHandler.sendMessage(getPrecondition);
        }
        
        if (mTqosTimer != null && mTqosTimer.isEnabled()) {
            Message getTqosTimer = mVolteHandler.obtainMessage(MSG_GET_TQOS_TIMER);
            mVolteHandler.sendMessage(getTqosTimer);
        }
        
        if (mTcallTimer != null && mTcallTimer.isEnabled()) {
            Message getTcallTimer = mVolteHandler.obtainMessage(MSG_GET_TCALL_TIMER);
            mVolteHandler.sendMessage(getTcallTimer);
        }
        
        if (mTregTimer != null && mTregTimer.isEnabled()) {
            Message getTregTimer = mVolteHandler.obtainMessage(MSG_GET_TREG_TIMER);
            mVolteHandler.sendMessage(getTregTimer);
        }
        if (mPcscfSwitch != null) {
            String pcscfAddress = SystemProperties.get("persist.sys.volte.pcscf");
            Log.d(TAG,"onStart pcscfAddress is: " + pcscfAddress);
            if ("".equals(pcscfAddress)) {
                mPcscfSwitch.setChecked(false);
                mPcscfSwitch.setSummary(getString(R.string.pcscf_dynamic));
            } else {
                mPcscfSwitch.setChecked(true);
                mPcscfSwitch.setSummary(getString(R.string.pcscf_static) + ": " + pcscfAddress.trim());
            }
        }
        super.onStart();
    }
    

    @Override
	protected void onResume() {
        super.onResume();
        if(mSessionTimer != null){
            Message getTimer=mVolteHandler.obtainMessage(GET_SESSION_TIMER);
            mVolteHandler.sendMessage(getTimer);
        }

        if(mForceMtSessionTimer != null){
            Message getTimer=mVolteHandler
                    .obtainMessage(GET_FORCE_MT_SESSION_TIMER);
            mVolteHandler.sendMessage(getTimer);
        }
	}

    @Override
    public void onDestroy() {
        if (mVolteHandler != null) {
            mVolteHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        super.onDestroy();
    }

    private void editDialog(Context context) {
        final EditText inputServer = new EditText(context);
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(getString(R.string.pcscf_static_input));
        builder.setView(inputServer);
        builder.setCancelable(false);
        builder.setNegativeButton(getString(R.string.alertdialog_cancel),
                new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog,
                            int which) {
                        mPcscfSwitch.setChecked(false);
                    }
                });
        builder.setPositiveButton(getString(R.string.alertdialog_ok),
                new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog,
                            int which) {
                        String editAdress = inputServer.getText().toString();
                        Log.d(TAG,"Edit input: " + editAdress);
                        if (editAdress != null && editAdress.length() != 0) {
                            if(editAdress.getBytes().length>EDITADRESS_LENGTH_MAX){
                                Toast.makeText(mContext, "The length of the input value can not exceed 91", Toast.LENGTH_SHORT).show();
                                mPcscfSwitch.setChecked(false);
                                return;
                            }
                            SystemProperties.set("persist.sys.volte.pcscf", editAdress);
                            /*StringBuilder pcscfSummer= new StringBuilder(getString(R.string.pcscf_static));
                            pcscfSummer.append(": ").append(editAdress);
                            mPcscfSwitch.setChecked(true);
                            mPcscfSwitch.setSummary(pcscfSummer.toString());*/
                            PowerManager pm = (PowerManager) VolteSettingsActivity.this
                                    .getSystemService(Context.POWER_SERVICE);
                            pm.reboot("pcscfset");
                        } else {
                            mPcscfSwitch.setChecked(false);
                        }
                    }
                });
        builder.show();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        String prefkey = pref.getKey();
        if (prefkey.equals(KEY_IP_SETTING)) {
            Message setIPState = mVolteHandler.obtainMessage(MSG_SET_IP_STATE, newValue);
            mVolteHandler.sendMessage(setIPState);
            return false;
        }
        if (prefkey.equals(KEY_MUT_LENGTH)) {
            Message setMutLength = mVolteHandler.obtainMessage(MSG_SET_MUT_LENGTH, newValue);
            mVolteHandler.sendMessage(setMutLength);
            return false;
        }
        if (prefkey.equals(KEY_SUBSCRIBE_SETTING)) {
            if (mSubscribeSwitch.isChecked()) {
                Message setSubscribeClose = mVolteHandler
                        .obtainMessage(MSG_SET_SUBSCRIB_STATE, "0");
                mVolteHandler.sendMessage(setSubscribeClose);
            } else {
                Message setSubscribeOpen = mVolteHandler.obtainMessage(MSG_SET_SUBSCRIB_STATE, "1");
                mVolteHandler.sendMessage(setSubscribeOpen);
            }
            return false;
        }
        if (prefkey.equals(KEY_SIGCOMP_SETTING)) {
            if (mSigcompSwitch.isChecked()) {
                Message setSigcompClose = mVolteHandler.obtainMessage(MSG_SET_SIGCOMP_STATE, "0");
                mVolteHandler.sendMessage(setSigcompClose);
            } else {
                Message setSigcompOpen = mVolteHandler.obtainMessage(MSG_SET_SIGCOMP_STATE, "1");
                mVolteHandler.sendMessage(setSigcompOpen);
            }
            return false;
        }
        if (prefkey.equals(KEY_SIGCOMP_ALGORITHM)) {
            Message setSigcomp = mVolteHandler.obtainMessage(MSG_SET_SIGCOMP_INDEX, newValue);
            mVolteHandler.sendMessage(setSigcomp);
            return false;
        }
        if (prefkey.equals(KEY_EXPIRES_TIMER_SETTING)) {
            Message setExpirestime = mVolteHandler.obtainMessage(MSG_SET_EXPIRES_TIME, newValue);
            mVolteHandler.sendMessage(setExpirestime);
            return false;
        }
        if (prefkey.equals(KEY_VOICE_CODE_WAY)) {
            Message setCodeWay = mVolteHandler.obtainMessage(MSG_SET_VOICE_CODE_WAY, newValue);
            mVolteHandler.sendMessage(setCodeWay);
            return false;
        }
        if (prefkey.equals(KEY_VOICE_CODE_SPEED)) {
            Message setCodeSpeed = mVolteHandler.obtainMessage(MSG_SET_VOICE_CODE_SPEED, newValue);
            mVolteHandler.sendMessage(setCodeSpeed);
            return false;
        }
        if (prefkey.equals(KEY_PRECONDITION_SETTING)) {
            if (mPreconditionSwitch.isChecked()) {
                Message setPreClose = mVolteHandler.obtainMessage(MSG_SET_PRECONDITION_STATE, "0");
                mVolteHandler.sendMessage(setPreClose);
            } else {
                Message setPreOpen = mVolteHandler.obtainMessage(MSG_SET_PRECONDITION_STATE, "1");
                mVolteHandler.sendMessage(setPreOpen);
            }
            return false;
        }
        if (prefkey.equals(KEY_TQOS_TIMER_SETTING)) {
            Message setTqos = mVolteHandler.obtainMessage(MSG_SET_TQOS_TIMER,newValue);
            mVolteHandler.sendMessage(setTqos);
            return false;
        }
        if (prefkey.equals(KEY_TCALL_TIMER_SETTING)) {
            Message setTcall = mVolteHandler.obtainMessage(MSG_SET_TCALL_TIMER,newValue);
            mVolteHandler.sendMessage(setTcall);
            return false;
        }
        if (prefkey.equals(KEY_TREG_TIMER_SETTING)) {
            Message setTreg = mVolteHandler.obtainMessage(MSG_SET_TREG_TIMER,newValue);
            mVolteHandler.sendMessage(setTreg);
            return false;
        }
        if (pref == mSessionTimer){
            if("".equals(newValue.toString())){
                Toast.makeText(mContext, "warning:empty value",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
           Message m = mVolteHandler.obtainMessage(SET_SESSION_TIMER, newValue);
           mVolteHandler.sendMessage(m);
        }
        if (pref == mForceMtSessionTimer){
           if ("".equals(newValue.toString())) {
               Toast.makeText(mContext, "warning:empty value",
                       Toast.LENGTH_SHORT).show();
               return false;
           }
           Message m = mVolteHandler.obtainMessage(SET_FORCE_MT_SESSION_TIMER, newValue);
           mVolteHandler.sendMessage(m);
        }
        for (int i = 0; i < mPhoneCount; i++) {
            if (prefkey.equals(KEY_IMPI_SETTING + i)) {
                Message setIMPI = mVolteHandler.obtainMessage(MSG_SET_IMPI, i, 0, newValue);
                mVolteHandler.sendMessage(setIMPI);
                return false;
            } else if (prefkey.equals(KEY_IMPU_SETTING + i)) {
                Message setIMPU = mVolteHandler.obtainMessage(MSG_SET_IMPU, i, 0, newValue);
                mVolteHandler.sendMessage(setIMPU);
                return false;
            } else if (prefkey.equals(KEY_DOMAIN_SETTING + i)) {
                Message setDomain = mVolteHandler.obtainMessage(MSG_SET_DOMAIN, i, 0, newValue);
                mVolteHandler.sendMessage(setDomain);
                return false;
            }
        }

        if (prefkey.equals(KEY_PCSCF_SET)) {
            AlertDialog alertDialog = new AlertDialog.Builder(
                    VolteSettingsActivity.this)
                    .setTitle("P-CSCF")
                    .setCancelable(false)
                    .setMessage(getString(R.string.mode_switch_waring))
                    .setPositiveButton(getString(R.string.alertdialog_ok),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    String currentSatus = SystemProperties.get(
                                            "persist.sys.volte.pcscf");
                                    if (!"".equals(currentSatus)) {
                                        SystemProperties.set(
                                                 "persist.sys.volte.pcscf", "");
                                         PowerManager pm = (PowerManager) VolteSettingsActivity.this
                                                 .getSystemService(Context.POWER_SERVICE);
                                         pm.reboot("pcscfset");
                                    } else {
                                        editDialog(VolteSettingsActivity.this);
                                    }
                                }
                            })
                    .setNegativeButton(R.string.alertdialog_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    mPcscfSwitch.setChecked(!("".equals(SystemProperties.get(
                                            "persist.sys.volte.pcscf"))));
                                }
                            }).create();
            alertDialog.show();
            return true;
        }
        return true;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        /*
         * if (key.equals(KEY_IP_SETTING)) {
         * mIPSetting.setSummary(mIPSetting.getEntry()); String newIPState =
         * mSharePref.getString(KEY_IP_SETTING, ""); Message setIPState =
         * mVolteHandler.obtainMessage(MSG_SET_IP_STATE, newIPState);
         * mVolteHandler.sendMessage(setIPState); }
         */
    }

    class VolteHandler extends Handler {

        public VolteHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_SET_IMPI:
                    mPhoneId = msg.arg1;
                    final String impiValue = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_IMPI + impiValue, "atchannel" + mPhoneId);
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIMPISettingScreen.findPreference(KEY_IMPI_SETTING + mPhoneId)
                                        .setSummary(impiValue);
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_IMPI_SETTING + mPhoneId, impiValue);
                                editor.commit();
                                Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_IMPU:
                    mPhoneId = msg.arg1;
                    final String impuValue = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_IMPU + impuValue, "atchannel" + mPhoneId);
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIMPISettingScreen.findPreference(KEY_IMPU_SETTING + mPhoneId)
                                        .setSummary(impuValue);
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_IMPU_SETTING + mPhoneId, impuValue);
                                editor.commit();
                                Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_DOMAIN:
                    mPhoneId = msg.arg1;
                    final String domainValue = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_DOMAIN + domainValue, "atchannel"
                            + mPhoneId);
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIMPISettingScreen.findPreference(KEY_DOMAIN_SETTING + mPhoneId)
                                        .setSummary(domainValue);
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_DOMAIN_SETTING + mPhoneId, domainValue);
                                editor.commit();
                                Toast.makeText(mContext, "Success", Toast.LENGTH_SHORT).show();
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_IP_STATE:
                    mResp = sendAt(engconstents.ENG_VOLTE_IP_SETTING + "0", "atchannel0");
                    final String stateValue = anayResult(MSG_GET_IP_STATE, mResp);
                    if (stateValue.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIPSetting.setEnabled(false);
                                mIPSetting.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIPSetting.setValueIndex(Integer.valueOf(stateValue).intValue()-1);
                                mIPSetting.setSummary(mIPSetting.getEntry());
                            }
                        });
                    }
                    break;
                case MSG_SET_IP_STATE:
                    String setIpState = (String) msg.obj;
                    final int setIpStateValue = Integer.valueOf(setIpState).intValue() + 1;
                    mResp = sendAt(engconstents.ENG_VOLTE_IP_SETTING + "1," + setIpStateValue,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIPSetting.setValueIndex(setIpStateValue-1);
                                mIPSetting.setSummary(mIPSetting.getEntry());
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_MUT_LENGTH:
                    mResp = sendAt(engconstents.ENG_VOLTE_MUT_SETTING + "0", "atchannel0");
                    final String lengthValue = anayResult(MSG_GET_MUT_LENGTH, mResp);
                    if (lengthValue.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMUTLength.setEnabled(false);
                                mMUTLength.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMUTLength.setSummary(lengthValue);
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_MUT_LENGTH, lengthValue);
                                editor.commit();
                            }
                        });
                    }
                    break;
                case MSG_SET_MUT_LENGTH:
                    final String setMutLength = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_MUT_SETTING + "1," + setMutLength,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_MUT_LENGTH, setMutLength);
                                editor.commit();
                                mMUTLength.setSummary(setMutLength);
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_SUBSCRIB_STATE:
                    mResp = sendAt(engconstents.ENG_VOLTE_SUBSCRIB_SETTING + "0", "atchannel0");
                    final String getSubscrib = anayResult(MSG_GET_SUBSCRIB_STATE, mResp);
                    if (getSubscrib.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSubscribeSwitch.setEnabled(false);
                                mSubscribeSwitch.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSubscribeSwitch.setChecked(getSubscrib.contains("1"));
                            }
                        });
                    }
                    break;
                case MSG_SET_SUBSCRIB_STATE:
                    final String setSubscrib = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_SUBSCRIB_SETTING + "1," + setSubscrib,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSubscribeSwitch.setChecked(setSubscrib.contains("1"));
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSubscribeSwitch.setChecked(!setSubscrib.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_SIGCOMP_STATE:
                    mResp = sendAt(engconstents.ENG_VOLTE_SIGCOMP_SETTING + "0", "atchannel0");
                    final String getSigcomp = anayResult(MSG_GET_SIGCOMP_STATE, mResp);
                    if (getSigcomp.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSigcompSwitch.setEnabled(false);
                                mSigcompSwitch.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                boolean open = getSigcomp.split("\\,")[0].contains("1");
                                mSigcompSwitch.setChecked(open);
                                if (open) {
                                    mSigcompSetting.setEnabled(true);
                                    Editor editor = mSharePref.edit();
                                    editor.putString(KEY_SIGCOMP_ALGORITHM,
                                            getSigcomp.split("\\,")[1]);
                                    editor.commit();
                                    mSigcompSetting.setValueIndex(Integer.valueOf(
                                            getSigcomp.split("\\,")[1].trim()).intValue());
                                    mSigcompSetting.setSummary(mSigcompSetting.getEntry());
                                } else {
                                    mSigcompSetting.setEnabled(false);
                                }
                            }
                        });
                    }
                    break;
                case MSG_SET_SIGCOMP_STATE:
                    final String setSigcomp = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_SIGCOMP_SETTING + "1," + setSigcomp,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSigcompSwitch.setChecked(setSigcomp.contains("1"));
                                mSigcompSetting.setEnabled(true);
                                if (setSigcomp.contains("0")) {
                                    mSigcompSetting.setEnabled(false);
                                }
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mSigcompSwitch.setChecked(!setSigcomp.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_SIGCOMP_INDEX:
                    final String setSigcompIndex = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_VOLTE_SIGCOMP_SETTING + "1,1," + setSigcompIndex,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_SIGCOMP_ALGORITHM, setSigcompIndex);
                                editor.commit();
                                mSigcompSetting.setValueIndex(Integer.valueOf(setSigcompIndex).intValue());
                                mSigcompSetting.setSummary(mSigcompSetting.getEntry());
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_EXPIRES_TIME:
                    mResp = sendAt(engconstents.ENG_EXPIERSTIMER_SETTING + "0", "atchannel0");
                    final String getExpiresTime = anayResult(MSG_GET_EXPIRES_TIME, mResp);
                    if (getExpiresTime.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mExpiresTimer.setEnabled(false);
                                mExpiresTimer.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mExpiresTimer.setEnabled(true);
                                mExpiresTimer.setSummary(getExpiresTime);
                            }
                        });
                    }
                    break;
                case MSG_SET_EXPIRES_TIME:
                    final String setExpires = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_EXPIERSTIMER_SETTING + "1," + setExpires,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_EXPIRES_TIMER_SETTING, setExpires);
                                editor.commit();
                                mExpiresTimer.setSummary(setExpires);
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_VOICE_CODE_WAY:
                    mResp = sendAt(engconstents.ENG_VOICE_CODE_SETTING + "0", "atchannel0");
                    final String getVoiceCodeWay = anayResult(MSG_GET_VOICE_CODE_WAY, mResp);
                    if (mResp != null && mResp.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_VOICE_CODE_WAY, getVoiceCodeWay.split("\\,")[1]);
                                editor.commit();
                                if (getVoiceCodeWay.split("\\,")[1].contains("1")) {
                                    mVoiceCodeWay.setValueIndex(1);
                                    mVoiceCodeWay.setSummary(mVoiceCodeWay.getEntry());
                                    mVoiceCodeSpeed.setEntries(R.array.volte_voice_wb_code_speed);
                                    mVoiceCodeSpeed
                                            .setEntryValues(R.array.volte_voice_wb_code_speed_value);
                                    mVoiceCodeSpeed.setEnabled(true);
                                    int setWbValueIndex = Integer.valueOf(
                                            getVoiceCodeWay.split("\\,")[2].trim()).intValue();
                                    if (setWbValueIndex == 254) {
                                        mVoiceCodeSpeed.setValueIndex(9);
                                    } else {
                                        mVoiceCodeSpeed.setValueIndex(setWbValueIndex);
                                    }
                                    mVoiceCodeSpeed.setSummary(mVoiceCodeSpeed.getEntry());
                                    editor.putString(KEY_VOICE_WB_CODE_SPEED,
                                            getVoiceCodeWay.split("\\,")[2]);
                                    editor.commit();

                                } else if (getVoiceCodeWay.split("\\,")[1].contains("0")) {
                                    mVoiceCodeWay.setValueIndex(0);
                                    mVoiceCodeWay.setSummary(mVoiceCodeWay.getEntry());
                                    mVoiceCodeSpeed.setEntries(R.array.volte_voice_nb_code_speed);
                                    mVoiceCodeSpeed
                                            .setEntryValues(R.array.volte_voice_nb_code_speed_value);
                                    mVoiceCodeSpeed.setEnabled(true);
                                    int setNbValueIndex = Integer.valueOf(
                                            getVoiceCodeWay.split("\\,")[2].trim()).intValue();
                                    if (setNbValueIndex == 254) {
                                        mVoiceCodeSpeed.setValueIndex(8);
                                    } else {
                                        mVoiceCodeSpeed.setValueIndex(setNbValueIndex);
                                    }
                                    mVoiceCodeSpeed.setSummary(mVoiceCodeSpeed.getEntry());
                                    editor.putString(KEY_VOICE_NB_CODE_SPEED,
                                            getVoiceCodeWay.split("\\,")[2]);
                                    editor.commit();
                                } else if (getVoiceCodeWay.split("\\,")[1].contains("3")) {
                                    mVoiceCodeWay.setValueIndex(2);
                                    mVoiceCodeWay.setSummary(mVoiceCodeWay.getEntry());
                                    mVoiceCodeSpeed.setSummary(mVoiceCodeSpeed.getEntry());
                                    mVoiceCodeSpeed.setEnabled(false);
                                }
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mVoiceCodeWay.setEnabled(false);
                                mVoiceCodeWay.setSummary(R.string.feature_abnormal);
                                mVoiceCodeSpeed.setEnabled(false);
                                mVoiceCodeSpeed.setSummary(R.string.feature_abnormal);
                            }
                        });
                    }
                    break;
                case MSG_SET_VOICE_CODE_WAY:
                    final String setVoiceCodeWay = (String) msg.obj;
                    if (setVoiceCodeWay.equals("0")) {
                        mResp = sendAt(engconstents.ENG_VOICE_CODE_SETTING + "1," + 0 + ","
                                + 254, "atchannel0");
                    } else if (setVoiceCodeWay.equals("1")) {
                        mResp = sendAt(engconstents.ENG_VOICE_CODE_SETTING + "1," + 1 + ","
                                + 254, "atchannel0");
                    } else if (setVoiceCodeWay.equals("2")) {
                        mResp = sendAt(engconstents.ENG_VOICE_CODE_SETTING + "1," + 3, "atchannel0");
                    }
                    if (!mResp.contains("OK")) {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    Message getVoiceCodeWays = mVolteHandler.obtainMessage(MSG_GET_VOICE_CODE_WAY);
                    mVolteHandler.sendMessage(getVoiceCodeWays);
                    break;
                case MSG_SET_VOICE_CODE_SPEED:
                   final String setVoiceCodeSpeed = (String) msg.obj;
                   String setVoiceSpeed = null;
                    Log.d(TAG,"set Voice CodeSpeed: " + setVoiceCodeSpeed);
                    mVoiceWayIndex = mSharePref.getString(KEY_VOICE_CODE_WAY, "0");
                if (("0".equals(mVoiceWayIndex) && "8"
                        .equals(setVoiceCodeSpeed))
                        || ("1".equals(mVoiceWayIndex) && "9"
                                .equals(setVoiceCodeSpeed))) {
                   setVoiceSpeed = "254";

                } else {
                    setVoiceSpeed = setVoiceCodeSpeed;
                }
                    mResp = sendAt(engconstents.ENG_VOICE_CODE_SETTING + "1," + mVoiceWayIndex + ","
                            + setVoiceSpeed, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mVoiceCodeSpeed.setValueIndex(Integer.valueOf(setVoiceCodeSpeed)
                                        .intValue());
                                mVoiceCodeSpeed.setSummary(mVoiceCodeSpeed.getEntry());
                            }
                        });
                        Editor editor = mSharePref.edit();
                        if (mVoiceWayIndex.contains("1")) {
                            editor.putString(KEY_VOICE_WB_CODE_SPEED, setVoiceSpeed);
                        } else {
                            editor.putString(KEY_VOICE_NB_CODE_SPEED, setVoiceSpeed);
                        }
                        editor.commit();
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_PRECONDITION_STATE:
                    mResp = sendAt(engconstents.ENG_PRECONDITION_SETTING + "0", "atchannel0");
                    final String getPreconditionStatue = anayResult(MSG_GET_PRECONDITION_STATE, mResp);
                    if (getPreconditionStatue.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mPreconditionSwitch.setEnabled(false);
                                mPreconditionSwitch.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mPreconditionSwitch.setChecked(getPreconditionStatue.contains("1"));
                            }
                        });
                    }
                    break;
                case MSG_SET_PRECONDITION_STATE:
                    final String setPreconditionStatue = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_PRECONDITION_SETTING + "1," + setPreconditionStatue,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mPreconditionSwitch.setChecked(setPreconditionStatue.contains("1"));
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mPreconditionSwitch.setChecked(!setPreconditionStatue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_TQOS_TIMER:
                    mResp = sendAt(engconstents.ENG_TQOS_TIMER_SETTING + "0","atchannel0");
                    final String getTqosTimer = anayResult(MSG_GET_TQOS_TIMER, mResp);
                    if (getTqosTimer.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTqosTimer.setEnabled(false);
                                mTqosTimer.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTqosTimer.setEnabled(true);
                                mTqosTimer.setSummary(getTqosTimer);
                            }
                        });
                    }
                    break;
                case MSG_SET_TQOS_TIMER:
                    final String setTqosTimer = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_TQOS_TIMER_SETTING + "1," + setTqosTimer,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_TQOS_TIMER_SETTING, setTqosTimer);
                                editor.commit();
                                mTqosTimer.setSummary(setTqosTimer);
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_TCALL_TIMER:
                    mResp = sendAt(engconstents.ENG_TCALL_TIMER_SETTING + "0","atchannel0");
                    final String getTcallTimer = anayResult(MSG_GET_TCALL_TIMER, mResp);
                    if (getTcallTimer.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTcallTimer.setEnabled(false);
                                mTcallTimer.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTcallTimer.setEnabled(true);
                                mTcallTimer.setSummary(getTcallTimer);
                            }
                        });
                    }
                    break;
                case MSG_SET_TCALL_TIMER:
                    final String setTcallTimer = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_TCALL_TIMER_SETTING + "1," + setTcallTimer,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_TCALL_TIMER_SETTING, setTcallTimer);
                                editor.commit();
                                mTcallTimer.setSummary(setTcallTimer);
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_GET_TREG_TIMER:
                    mResp = sendAt(engconstents.ENG_TREG_TIMER_SETTING + "0","atchannel0");
                    final String getTrgeTimer = anayResult(MSG_GET_TREG_TIMER, mResp);
                    if (getTrgeTimer.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTregTimer.setEnabled(false);
                                mTregTimer.setSummary(R.string.feature_abnormal);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mTregTimer.setEnabled(true);
                                mTregTimer.setSummary(getTrgeTimer);
                            }
                        });
                    }
                    break;
                case MSG_SET_TREG_TIMER:
                    final String setTrgeTimer = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_TREG_TIMER_SETTING + "1," + setTrgeTimer,
                            "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_TCALL_TIMER_SETTING, setTrgeTimer);
                                editor.commit();
                                mTregTimer.setSummary(setTrgeTimer);
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case GET_SESSION_TIMER:
                    mResp = sendAt(engconstents.ENG_GET_SESSION_TIMER,"atchannel0");
                    Log.d(TAG, "GET_SESSION_TIMER" + "  " + "mATResponse: "+ mResp);
                    if (mResp.contains(IATUtils.AT_OK)) {
                        String[] str=mResp.split("\"");
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
                    mResp = sendAt(engconstents.ENG_SET_SESSION_TIMER+"\""+time+"\"","atchannel0");
                    Log.d(TAG, "SET_SESSION_TIMER" + "  " + "mATResponse: "+ mResp);
                    if (mResp.contains(IATUtils.AT_OK)) {
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
                    mResp = sendAt(engconstents.ENG_GET_FORCE_MT_SESSION_TIMER,"atchannel0");
                    Log.d(TAG, "GET_FORCE_MT_SESSION_TIMER" + "  " + "mATResponse: "+ mResp);
                    if (mResp.contains(IATUtils.AT_OK)) {
                        String[] str=mResp.split("\"");
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
                    mResp = sendAt(engconstents.ENG_SET_FORCE_MT_SESSION_TIMER+"\""+force_mt_time+"\"","atchannel0");
                    Log.d(TAG, "SET_FORCE_MT_SESSION_TIMER" + "  " + "mATResponse: "+ mResp);
                    if (mResp.contains(IATUtils.AT_OK)) {
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
                    break;
                default:
                    break;
            }
        }
    }

    /*
     * when at response is null,change response to "FAILED"
     */
    private String sendAt(String cmd, String servername) {
        String res = IATUtils.sendATCmd(cmd, servername);
        Log.d(TAG, "ATCmd is " + cmd + ", result is " + res);
        if (res != null) {
            return res;
        } else {
            return "FAILED";
        }
    }

    /*
     * 
     */
    private String anayResult(int mesg, String result) {
        String res = null;
        if (!result.contains("OK")) {
            return res = "FAILED";
        }
        switch (mesg) {
            case MSG_GET_IP_STATE:
                // res only contains IP state;ex:res ="1";
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_IP_STATE anayResult is " + res);
                break;
            case MSG_GET_MUT_LENGTH:
                // res only contains length;ex:res ="33";
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_MUT_LENGTH anayResult is " + res);
                break;
            case MSG_GET_SUBSCRIB_STATE:
                // res only contains subscribe switch state;ex:res="1" means
                // switch open
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_SUBSCRIB_STATE anayResult is " + res);
                break;
            case MSG_GET_SIGCOMP_STATE:
                // res contains switch state and algorithm;ex:res= "1,0" means
                // switch open and algorithm is default
                // <-+SPENGMDVOLTE：9,0,1,2
                // <-OK
                res = result.split("\\:")[1].trim().split("\n")[0].substring(4);
                Log.d(TAG, "MSG_GET_SIGCOMP_STATE anayResult is " + res);
                break;
            case MSG_GET_EXPIRES_TIME:
                // res only contains time;ex:res="3600"
                // <-+SPENGMDVOLTE：11,0,3600
                // <-OK
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_EXPIRES_TIME anayResult is " + res);
                break;
            case MSG_GET_VOICE_CODE_WAY:
                // res contains code_way and code_speed;ex:res="0,6" means
                // code_way is AMR-WB,code_speed is 19.85 kbps
                // -> AT+SPENGMDVOLTE:12,0,0，6
                // <-OK
                res = result.split("\\:")[1].split("\n")[0].substring(4);
                Log.d(TAG, "MSG_GET_VOICE_CODE_WAY anayResult is " + res);
                break;
            case MSG_GET_PRECONDITION_STATE:
                //res only contains precondition state;ex:res="1" means precondition open
                //<-+SPENGMDVOLTE：13,0,1
                //<-OK
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_PRECONDITION_STATE anayResult is " + res);
                break;
            case MSG_GET_TQOS_TIMER:
                //res only contains tqos time; ex:res="6" means tqos time is 6 sec
                //<-+SPENGMDVOLTE：14,0,6
                //<-OK
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_TQOS_TIMER anayResult is " + res);
                break;
            case MSG_GET_TCALL_TIMER:
                //res only contains tcall time; ex:res="10" means tqos time is 10 sec
                //<-+SPENGMDVOLTE：15,0,10
                //<-OK
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_TCALL_TIMER anayResult is " + res);
                break;
            case MSG_GET_TREG_TIMER:
                //res only contains treg time; ex:res="15" means tqos time is 15 sec
                //<-+SPENGMDVOLTE：16,0,15
                //<-OK
                res = result.split("\\:")[1].split("\\,")[2].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_TREG_TIMER anayResult is " + res);
                break;
                //res only contains auth type; ex:res="auto" means auth type is auto
            default:
                break;
        }
        return res;
    }
}
