
package com.sprd.engineermode.telephony;

import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.widget.Toast;
import android.content.Intent;
import android.provider.Settings;
import android.text.TextUtils;
import android.preference.Preference.OnPreferenceClickListener;
import android.os.UserHandle;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;

public class AppSettingsPrefActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "AppSettingsPrefActivity";

    private static final int STATUS_OFF = 1;
    private static final int STATUS_ON = 0;
    public static final int REQUEST_UA_SETTINGS = 1;

    private static final String AUTO_RETRY_DIAL = "key_emergency_call_retry";
    private static final String MODEM_RESET = "modem_reset";
    private static final String ENABLE_VSER_GSER = "enable_vser_gser";
    private static final String ENABLE_FOR_MTBF_TEST = "enable_mtbf_test";
    private static final String UA_SETTING = "ua_setting";

    private EMSwitchPreference mEmergencyCallRetry = null;
    private EMSwitchPreference mModemReset = null;
    private EMSwitchPreference mEnableVserGser;
    private EMSwitchPreference mMtbfTest;
    private Preference mUAsetting;

    public static final String USER_AGENT_CHOICE = "user_agent_choice";
    public static final String CUSTOM_USER_AGENT_STRING = "custom_user_agent_string";
    public static final String OTHER_USER_AGENT_STRING = "other_user_agent_string";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_appsetting);

        mEmergencyCallRetry = (EMSwitchPreference) findPreference(AUTO_RETRY_DIAL);
        mEmergencyCallRetry.setOnPreferenceChangeListener(this);

        mModemReset = (EMSwitchPreference) findPreference(MODEM_RESET);
        mModemReset.setOnPreferenceChangeListener(this);

        mEnableVserGser = (EMSwitchPreference) findPreference(ENABLE_VSER_GSER);
        mEnableVserGser.setOnPreferenceChangeListener(this);

        mMtbfTest = (EMSwitchPreference) findPreference(ENABLE_FOR_MTBF_TEST);
        mMtbfTest.setOnPreferenceChangeListener(this);

        mUAsetting = (Preference) findPreference(UA_SETTING);
        if(!checkApkExist("com.sprd.uasetting")) {
            mUAsetting.setEnabled(false);
            mUAsetting.setSummary(R.string.apk_not_exist);
        }
        mUAsetting.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                int mUserId = UserHandle.myUserId();
                if (mUserId != 0) {
                    Toast.makeText(AppSettingsPrefActivity.this,
                            R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                    return false;
                }

                Intent intent = new Intent();
                intent.setClassName("com.sprd.uasetting", "com.sprd.uasetting.UASettingActivity");
                startActivityForResult(intent, REQUEST_UA_SETTINGS);
                return false;
            }
        });

        String autoRetry = SystemProperties.get("persist.sys.emergencyCallRetry");
        if (0 == autoRetry.compareTo("1")) {
            mEmergencyCallRetry.setChecked(true);
        } else {
            mEmergencyCallRetry.setChecked(false);
        }

        String result = SystemProperties.get("persist.sys.sprd.modemreset");
        mModemReset.setChecked(result.equals("1"));
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {

        final String key = preference.getKey();

        if (0 == AUTO_RETRY_DIAL.compareTo(key)) {
            if (preference instanceof EMSwitchPreference) {
                SystemProperties.set("persist.sys.emergencyCallRetry",
                        ((EMSwitchPreference) preference).isChecked() ? "0" : "1");
                Toast.makeText(AppSettingsPrefActivity.this, "Success", Toast.LENGTH_SHORT).show();
            }
        } else if (0 == MODEM_RESET.compareTo(key)) {
            SystemProperties.set("persist.sys.sprd.modemreset",
                    ((EMSwitchPreference) preference).isChecked() ? "0" : "1");
        } else if (0 == ENABLE_VSER_GSER.compareTo(key)) {
            boolean oldState = mEnableVserGser.isChecked();
            if (oldState) {
                SystemProperties.set("persist.sys.modem.diag", ",none");
            } else {
                SystemProperties.set("persist.sys.modem.diag", ",gser");
            }
        } else if (0 == ENABLE_FOR_MTBF_TEST.compareTo(key)) {
            boolean oldState = mMtbfTest.isChecked();
            if (oldState) {
                SystemProperties.set("persist.sys.sprd.mtbf", "0");
            } else {
                SystemProperties.set("persist.sys.sprd.mtbf", "1");
            }
        }
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        Log.d(TAG, "onActivityResult");
        Log.d(TAG, "resultCode: " + resultCode + "requestCode: " + requestCode + "RESULT_OK:"
                + RESULT_OK);
        if (resultCode == RESULT_OK) {
            switch (requestCode) {
                case REQUEST_UA_SETTINGS:
                    if (intent == null)
                        return;
                    String customUaString = intent.getStringExtra(CUSTOM_USER_AGENT_STRING);
                    if (!TextUtils.isEmpty(customUaString)) {
                        Settings.System.putString(getContentResolver(), CUSTOM_USER_AGENT_STRING,
                                customUaString);
                    }
                    int uaChoice = intent.getIntExtra(USER_AGENT_CHOICE, -1);
                    if (uaChoice != -1) {
                        Settings.System.putInt(getContentResolver(),
                                USER_AGENT_CHOICE, uaChoice);
                    }
                    String otherUaString = intent
                            .getStringExtra(OTHER_USER_AGENT_STRING);
                    if (!TextUtils.isEmpty(otherUaString)) {
                        Settings.System.putString(getContentResolver(),
                                OTHER_USER_AGENT_STRING, otherUaString);
                    }
                    break;
            }
        } else {
            Log.d(TAG, "onActivityResult,resultCode: " + resultCode);
            return;
        }

    }

    @Override
    protected void onResume() {
        super.onResume();
        String usbMode = SystemProperties.get("persist.sys.modem.diag", ",none");
        if (usbMode.contains(",gser")) {
            mEnableVserGser.setChecked(true);
        } else {
            mEnableVserGser.setChecked(false);
        }

        String mMtbfState = SystemProperties.get("persist.sys.sprd.mtbf", "1");
        if (mMtbfState.equals("1")) {
            mMtbfTest.setChecked(true);
        } else {
            mMtbfTest.setChecked(false);
        }
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

    public boolean checkApkExist(String packageName) {
        if (packageName == null || "".equals(packageName))
           return false;
        try {
           ApplicationInfo info = AppSettingsPrefActivity.this.getPackageManager()
             .getApplicationInfo(packageName,
           PackageManager.GET_UNINSTALLED_PACKAGES);
           return true;
        } catch (Exception e) {
           return false;
        }
    }
}
