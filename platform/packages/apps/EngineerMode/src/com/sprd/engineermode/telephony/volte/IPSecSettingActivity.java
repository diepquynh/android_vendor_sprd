
package com.sprd.engineermode.telephony.volte;

import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.TwoStatePreference;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.util.Log;
import android.widget.Toast;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Message;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.R;

public class IPSecSettingActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "IPSecSettingActivity";
    private static final String KEY_IPSEC_SWITCH = "volte_ipsec_setting_switch";
    private static final String KEY_MAINTAIN_ALGORITHM_0 = "volte_maintain_algorithm_0";
    private static final String KEY_MAINTAIN_ALGORITHM_1 = "volte_maintain_algorithm_1";
    private static final String KEY_ENCRYPTION_ALGORITHM_0 = "volte_encryption_algorithm_0";
    private static final String KEY_ENCRYPTION_ALGORITHM_1 = "volte_encryption_algorithm_1";
    private static final String KEY_ENCRYPTION_ALGORITHM_2 = "volte_encryption_algorithm_2";

    private static final String KEY_MAIN_ALGORITHM = "volte_main_algorithm";
    private static final String KEY_ENCRY_ALGORITHM = "volte_encry_algorithm";

    private static final int MSG_GET_IPSEC_STATE = 0;
    private static final int MSG_SET_IPSEC_STATE = 1;
    private static final int MSG_SET_MAINTAIN_ALGORITHM_0 = 2;
    private static final int MSG_SET_MAINTAIN_ALGORITHM_1 = 3;
    private static final int MSG_SET_ENCRYPTION_ALGORITHM_0 = 4;
    private static final int MSG_SET_ENCRYPTION_ALGORITHM_1 = 5;
    private static final int MSG_SET_ENCRYPTION_ALGORITHM_2 = 6;

    private TwoStatePreference mIPsecSwitch;
    private CheckBoxPreference mMaintainAlgorithm0;
    private CheckBoxPreference mMaintainAlgorithm1;
    private CheckBoxPreference mEncryptionAlgorithm0;
    private CheckBoxPreference mEncryptionAlgorithm1;
    private CheckBoxPreference mEncryptionAlgorithm2;

    private Handler mUiThread = new Handler();
    private IpsecHandler mIpsecHandler;
    private String mResp = null;
    private String mNewValue = null;
    private SharedPreferences mSharePref;
    private String mMainAlgorithmIndex = null;
    private String mEncryAlgorithmIndex = null;
    private Context mContext = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_volte_ipsec_setting);
        mContext = this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mIpsecHandler = new IpsecHandler(ht.getLooper());
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mIPsecSwitch = (TwoStatePreference) findPreference(KEY_IPSEC_SWITCH);
        mIPsecSwitch.setOnPreferenceChangeListener(this);
        mMaintainAlgorithm0 = (CheckBoxPreference) findPreference(KEY_MAINTAIN_ALGORITHM_0);
        mMaintainAlgorithm0.setOnPreferenceChangeListener(this);
        mMaintainAlgorithm0.setEnabled(false);
        mMaintainAlgorithm1 = (CheckBoxPreference) findPreference(KEY_MAINTAIN_ALGORITHM_1);
        mMaintainAlgorithm1.setOnPreferenceChangeListener(this);
        mMaintainAlgorithm1.setEnabled(false);
        mEncryptionAlgorithm0 = (CheckBoxPreference) findPreference(KEY_ENCRYPTION_ALGORITHM_0);
        mEncryptionAlgorithm0.setOnPreferenceChangeListener(this);
        mEncryptionAlgorithm0.setEnabled(false);
        mEncryptionAlgorithm1 = (CheckBoxPreference) findPreference(KEY_ENCRYPTION_ALGORITHM_1);
        mEncryptionAlgorithm1.setOnPreferenceChangeListener(this);
        mEncryptionAlgorithm1.setEnabled(false);
        mEncryptionAlgorithm2 = (CheckBoxPreference) findPreference(KEY_ENCRYPTION_ALGORITHM_2);
        mEncryptionAlgorithm2.setOnPreferenceChangeListener(this);
        mEncryptionAlgorithm2.setEnabled(false);
    }

    @Override
    public void onStart() {
        if (mIPsecSwitch != null && mIPsecSwitch.isEnabled()) {
            Message getIpsec = mIpsecHandler.obtainMessage(MSG_GET_IPSEC_STATE);
            mIpsecHandler.sendMessage(getIpsec);
        }
        super.onStart();
    }

    @Override
    public void onDestroy() {
        if (mIpsecHandler != null) {
            mIpsecHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mIPsecSwitch) {
            if (mIPsecSwitch.isChecked()) {
                Message setIpsecClose = mIpsecHandler.obtainMessage(MSG_SET_IPSEC_STATE, "0");
                mIpsecHandler.sendMessage(setIpsecClose);
            } else {
                Message setIpsecOpen = mIpsecHandler.obtainMessage(MSG_SET_IPSEC_STATE, "1");
                mIpsecHandler.sendMessage(setIpsecOpen);
            }
        }
        if (pref == mMaintainAlgorithm0) {
            if (mMaintainAlgorithm0.isChecked()) {
                Message setMain0Close = mIpsecHandler.obtainMessage(MSG_SET_MAINTAIN_ALGORITHM_0, "0");
                mIpsecHandler.sendMessage(setMain0Close);
            } else {
                Message setMain0Open = mIpsecHandler.obtainMessage(MSG_SET_MAINTAIN_ALGORITHM_0, "1");
                mIpsecHandler.sendMessage(setMain0Open);
            }
        }
        if (pref == mMaintainAlgorithm1) {
            if (mMaintainAlgorithm1.isChecked()) {
                Message setMain1Close = mIpsecHandler.obtainMessage(MSG_SET_MAINTAIN_ALGORITHM_1, "0");
                mIpsecHandler.sendMessage(setMain1Close);
            } else {
                Message setMain1Open = mIpsecHandler.obtainMessage(MSG_SET_MAINTAIN_ALGORITHM_1, "1");
                mIpsecHandler.sendMessage(setMain1Open);
            }
        }
        if (pref == mEncryptionAlgorithm0) {
            if (mEncryptionAlgorithm0.isChecked()) {
                Message setEncry0Close = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_0, "0");
                mIpsecHandler.sendMessage(setEncry0Close);
            } else {
                Message setEncry0Open = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_0, "1");
                mIpsecHandler.sendMessage(setEncry0Open);
            }
        }
        if (pref == mEncryptionAlgorithm1) {
            if (mEncryptionAlgorithm1.isChecked()) {
                Message setEncry1Close = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_1, "0");
                mIpsecHandler.sendMessage(setEncry1Close);
            } else {
                Message setEncry1Open = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_1, "1");
                mIpsecHandler.sendMessage(setEncry1Open);
            }
        }
        if (pref == mEncryptionAlgorithm2) {
            if (mEncryptionAlgorithm2.isChecked()) {
                Message setEncry2Close = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_2, "0");
                mIpsecHandler.sendMessage(setEncry2Close);
            } else {
                Message setEncry2Open = mIpsecHandler.obtainMessage(MSG_SET_ENCRYPTION_ALGORITHM_2, "1");
                mIpsecHandler.sendMessage(setEncry2Open);
            }
        }
        return false;
    }

    class IpsecHandler extends Handler {
        public IpsecHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_GET_IPSEC_STATE:
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "0", "atchannel0");
                    mNewValue = anayResult(MSG_GET_IPSEC_STATE, mResp);
                    if (mNewValue.contains("FAILED")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIPsecSwitch.setEnabled(false);
                                mIPsecSwitch.setSummary(R.string.feature_abnormal);
                                checkAlgorithm(mNewValue, false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                boolean open = mNewValue.split("\\,")[2].contains("1");
                                mIPsecSwitch.setChecked(open);
                                if (open) {
                                    checkAlgorithm(mNewValue, true);
                                } else {
                                    checkAlgorithm(mNewValue, false);
                                }
                            }
                        });
                    }
                    break;
                case MSG_SET_IPSEC_STATE:
                    mNewValue = (String) msg.obj;
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1," + mNewValue, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIPsecSwitch.setChecked(mNewValue.contains("1"));
                                enabledAlgorithm(mNewValue.contains("1"));
                            }
                        });
                    } else {
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_MAINTAIN_ALGORITHM_0:
                    mNewValue = (String) msg.obj;
                    mEncryAlgorithmIndex = mSharePref.getString(KEY_ENCRY_ALGORITHM, "0");
                    mMainAlgorithmIndex = mSharePref.getString(KEY_MAIN_ALGORITHM, "0");
                    if (mNewValue.contains("1")) {
                        mMainAlgorithmIndex = Integer.toString(Integer.valueOf(mMainAlgorithmIndex).intValue()+1);
                    } else {
                        mMainAlgorithmIndex = Integer.toString(Integer.valueOf(mMainAlgorithmIndex).intValue()-1);
                    }
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1,1," + mMainAlgorithmIndex +","+mEncryAlgorithmIndex, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaintainAlgorithm0.setChecked(mNewValue.contains("1"));
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_MAIN_ALGORITHM, mMainAlgorithmIndex);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaintainAlgorithm0.setChecked(!mNewValue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_MAINTAIN_ALGORITHM_1:
                    mNewValue = (String) msg.obj;
                    mEncryAlgorithmIndex = mSharePref.getString(KEY_ENCRY_ALGORITHM, "0");
                    mMainAlgorithmIndex = mSharePref.getString(KEY_MAIN_ALGORITHM, "0");
                    if (mNewValue.contains("1")) {
                        mMainAlgorithmIndex = Integer.toString(Integer.valueOf(mMainAlgorithmIndex).intValue()+2);
                    } else {
                        mMainAlgorithmIndex = Integer.toString(Integer.valueOf(mMainAlgorithmIndex).intValue()-2);
                    }
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1,1," + mMainAlgorithmIndex + "," + mEncryAlgorithmIndex, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaintainAlgorithm1.setChecked(mNewValue.contains("1"));
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_MAIN_ALGORITHM, mMainAlgorithmIndex);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaintainAlgorithm0.setChecked(!mNewValue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_ENCRYPTION_ALGORITHM_0:
                    mNewValue = (String) msg.obj;
                    mMainAlgorithmIndex = mSharePref.getString(KEY_MAIN_ALGORITHM, "0");
                    mEncryAlgorithmIndex = mSharePref.getString(KEY_ENCRY_ALGORITHM, "0");
                    if (mNewValue.contains("1")) {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()+1);
                    } else {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()-1);
                    }
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1,1," + mMainAlgorithmIndex+"," + mEncryAlgorithmIndex, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm0.setChecked(mNewValue.contains("1"));
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_ENCRY_ALGORITHM, mEncryAlgorithmIndex);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm0.setChecked(!mNewValue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_ENCRYPTION_ALGORITHM_1:
                    mNewValue = (String) msg.obj;
                    mMainAlgorithmIndex = mSharePref.getString(KEY_MAIN_ALGORITHM, "0");
                    mEncryAlgorithmIndex = mSharePref.getString(KEY_ENCRY_ALGORITHM, "0");
                    if (mNewValue.contains("1")) {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()+2);
                    } else {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()-2);
                    }
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1,1," + mMainAlgorithmIndex+"," + mEncryAlgorithmIndex, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm1.setChecked(mNewValue.contains("1"));
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_ENCRY_ALGORITHM, mEncryAlgorithmIndex);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm1.setChecked(!mNewValue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_ENCRYPTION_ALGORITHM_2:
                    mNewValue = (String) msg.obj;
                    mMainAlgorithmIndex = mSharePref.getString(KEY_MAIN_ALGORITHM, "0");
                    mEncryAlgorithmIndex = mSharePref.getString(KEY_ENCRY_ALGORITHM, "0");
                    if (mNewValue.contains("1")) {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()+4);
                    } else {
                        mEncryAlgorithmIndex = Integer.toString(Integer.valueOf(mEncryAlgorithmIndex).intValue()-4);
                    }
                    mResp = sendAt(engconstents.ENG_IPSEC_SETTING + "1,1," + mMainAlgorithmIndex+"," + mEncryAlgorithmIndex, "atchannel0");
                    if (mResp.contains("OK")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm2.setChecked(mNewValue.contains("1"));
                                Editor editor = mSharePref.edit();
                                editor.putString(KEY_ENCRY_ALGORITHM, mEncryAlgorithmIndex);
                                editor.commit();
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEncryptionAlgorithm2.setChecked(!mNewValue.contains("1"));
                            }
                        });
                        Toast.makeText(mContext, "Fail", Toast.LENGTH_SHORT).show();
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

    private String anayResult(int mesg, String result) {
        String res = null;
        if (!result.contains("OK")) {
            return res = "FAILED";
        }
        switch (mesg) {
            case MSG_GET_IPSEC_STATE:
                // res contains ipsec switch state and algorithm
                // value;ex:res="1,1,6" means ipsec open and maintain_algorithm
                // is "01"(hmac-md5-96)
                // encryption_algorithm is "110"(null and aes-cbc)
                // <-+SPENGMDVOLTE：10,0,1,1,6
                // <-OK
                res = result.split("\\:")[1].split("\n")[0].trim();
                Log.d(TAG, "MSG_GET_IPSEC_STATE anayResult is " + res);
                break;
            default:
                break;
        }
        return res;
    }

    private void checkAlgorithm(String value, boolean isEnabled) {

        if (!value.contains("FAILED")) {
            String mainAlgorithm = value.split("\\,")[3];
            String encryptionAlgorithm = value.split("\\,")[4];
            boolean mMA0Ischecked = ((Integer.valueOf(mainAlgorithm.trim())
                    .intValue() & 0x01) > 0);
            boolean mMA1Ischecked = (((Integer.valueOf(mainAlgorithm.trim())
                    .intValue() >> 1) & 0x01) > 0);
            boolean mEA0Ischecked = (((Integer.valueOf(encryptionAlgorithm.trim())
                    .intValue()) & 0x01) > 0);
            boolean mEA1Ischecked = (((Integer.valueOf(encryptionAlgorithm.trim())
                    .intValue()) >> 1 & 0x01) > 0);
            boolean mEA2Ischecked = (((Integer.valueOf(encryptionAlgorithm.trim())
                    .intValue()) >> 2 & 0x01) > 0);
            enabledAlgorithm(isEnabled);
            mMaintainAlgorithm0.setChecked(mMA0Ischecked);
            mMaintainAlgorithm1.setChecked(mMA1Ischecked);
            mEncryptionAlgorithm0.setChecked(mEA0Ischecked);
            mEncryptionAlgorithm1.setChecked(mEA1Ischecked);
            mEncryptionAlgorithm2.setChecked(mEA2Ischecked);
            Editor editor = mSharePref.edit();
            editor.putString(KEY_MAIN_ALGORITHM, mainAlgorithm);
            editor.putString(KEY_ENCRY_ALGORITHM, encryptionAlgorithm);
            editor.commit();
        } else {
            enabledAlgorithm(false);
            mMaintainAlgorithm0.setChecked(false);
            mMaintainAlgorithm1.setChecked(false);
            mEncryptionAlgorithm0.setChecked(false);
            mEncryptionAlgorithm1.setChecked(false);
            mEncryptionAlgorithm2.setChecked(false);
        }
    }

    private void enabledAlgorithm(boolean isEnabled) {
        mMaintainAlgorithm0.setEnabled(isEnabled);
        mMaintainAlgorithm1.setEnabled(isEnabled);
        mEncryptionAlgorithm0.setEnabled(isEnabled);
        mEncryptionAlgorithm1.setEnabled(isEnabled);
        mEncryptionAlgorithm2.setEnabled(isEnabled);
    }
}
