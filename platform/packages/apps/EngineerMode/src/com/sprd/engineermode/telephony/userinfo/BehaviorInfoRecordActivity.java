package com.sprd.engineermode.telephony.userinfo;

import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceClickListener;

import android.util.Log;
import android.widget.EditText;
import android.widget.Toast;
import android.app.AlertDialog;
import android.text.InputFilter;
import android.text.InputFilter.LengthFilter;
import android.text.method.NumberKeyListener;
import android.content.Context;
import android.content.Intent;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;

public class BehaviorInfoRecordActivity extends PreferenceActivity implements
Preference.OnPreferenceChangeListener{
    private static final String TAG = "BehaviorInfoRecordActivity";

    private static final String KEY_RECORD_SWITCH = "key_behavior_info_record";
    private static final String KEY_SCREEN_SWITCH = "key_screen_info_record";
    private static final String KEY_WIFI_SWITCH = "key_wifi_info_record";
    private static final String KEY_DATA_SWITCH = "key_data_info_record";
    private static final String KEY_BT_SWITCH = "key_bt_info_record";
    private static final String KEY_GPS_SWITCH = "key_gps_info_record";
    private static final String KEY_CALL_SWITCH = "key_call_info_record";
    private static final String KEY_SERVER_FREQ = "key_server_freq_record";
    private static final String KEY_QUERY_TIME = "query_time";

    private Context mContext;
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    private EMSwitchPreference mRecord,mScreenRecord,mWifiRecord,mDataRecord,mBtRecord,mGpsRecord,mCallRecord,mServerFreqRecord;
    private Preference mQueryTime;

    public static Intent bServiceIntent = null;
    private Handler mUiThread = new Handler();
    public static boolean mIsOpenBehaviorRecord = SystemProperties.getBoolean("persist.sys.open.user.record", false);
    public static boolean mIsOpenAll=false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_behavior_info_record);
        mContext=this;

        preferences = mContext
                .getSharedPreferences("behavior_record", mContext.MODE_PRIVATE);
        editor = preferences.edit();

        initUI();
    }

    private void initUI(){
        mScreenRecord = (EMSwitchPreference) findPreference(KEY_SCREEN_SWITCH);
        mScreenRecord.setOnPreferenceChangeListener(this);
        mWifiRecord = (EMSwitchPreference) findPreference(KEY_WIFI_SWITCH);
        mWifiRecord.setOnPreferenceChangeListener(this);
        mDataRecord = (EMSwitchPreference) findPreference(KEY_DATA_SWITCH);
        mDataRecord.setOnPreferenceChangeListener(this);
        mBtRecord = (EMSwitchPreference) findPreference(KEY_BT_SWITCH);
        mBtRecord.setOnPreferenceChangeListener(this);
        mGpsRecord = (EMSwitchPreference) findPreference(KEY_GPS_SWITCH);
        mGpsRecord.setOnPreferenceChangeListener(this);
        mCallRecord = (EMSwitchPreference) findPreference(KEY_CALL_SWITCH);
        mCallRecord.setOnPreferenceChangeListener(this);
        mServerFreqRecord = (EMSwitchPreference) findPreference(KEY_SERVER_FREQ);
        mServerFreqRecord.setOnPreferenceChangeListener(this);
        mQueryTime= (Preference) findPreference(KEY_QUERY_TIME);
        mQueryTime.setOnPreferenceChangeListener(this);
        mQueryTime.setSummary(preferences.getString("query_time", "2")+"s");
    }

    @Override
    protected void onResume() {
        super.onResume();
        mScreenRecord.setChecked(false);
        mWifiRecord.setChecked(false);
        mDataRecord.setChecked(false);
        mBtRecord.setChecked(false);
        mGpsRecord.setChecked(false);
        mCallRecord.setChecked(false);

        if("1".equals(preferences.getString("screen_record","0"))){
            mScreenRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("wifi_record","0"))){
            mWifiRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("data_record","0"))){
            mDataRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("bt_record","0"))){
            mBtRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("gps_record","0"))){
            mGpsRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("call_record","0"))){
            mCallRecord.setChecked(true);
        }
        if("1".equals(preferences.getString("server_freq_record","0"))){
            mServerFreqRecord.setChecked(true);
        }
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {

        final String key = preference.getKey();

        if(KEY_SCREEN_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("screen_record", "0");
                editor.commit();
                if(!mWifiRecord.isChecked() && !mDataRecord.isChecked() &&
                        !mBtRecord.isChecked() && !mGpsRecord.isChecked() && !mCallRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("screen_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_WIFI_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("wifi_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mDataRecord.isChecked() &&
                        !mBtRecord.isChecked() && !mGpsRecord.isChecked() && !mCallRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("wifi_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_DATA_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("data_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mWifiRecord.isChecked() &&
                        !mBtRecord.isChecked() && !mGpsRecord.isChecked() && !mCallRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("data_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_BT_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("bt_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mWifiRecord.isChecked() &&
                        !mDataRecord.isChecked() && !mGpsRecord.isChecked() && !mCallRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("bt_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_GPS_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("gps_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mWifiRecord.isChecked() &&
                        !mDataRecord.isChecked() && !mBtRecord.isChecked() && !mCallRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("gps_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_CALL_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("call_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mWifiRecord.isChecked() &&
                        !mDataRecord.isChecked() && !mBtRecord.isChecked() && !mGpsRecord.isChecked() && !mServerFreqRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("call_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }else if(KEY_SERVER_FREQ.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("server_freq_record", "0");
                editor.commit();
                if(!mScreenRecord.isChecked() && !mWifiRecord.isChecked() &&
                        !mDataRecord.isChecked() && !mBtRecord.isChecked() && !mGpsRecord.isChecked() && !mCallRecord.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("server_freq_record", "1");
                editor.commit();
                if(bServiceIntent == null){
                    openService();
                }
            }
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        final String key = preference.getKey();

        if(KEY_QUERY_TIME.equals(key)){
            final EditText inputServer = new EditText(mContext);
            inputServer.setKeyListener(new NumberKeyListener() {
                private char[] numberChars = { '0', '1', '2', '3', '4', '5',
                        '6', '7', '8', '9' };

                // Used to limit input method
                @Override
                public int getInputType() {
                    return android.text.InputType.TYPE_CLASS_PHONE;
                }

                // Used to control the characters to input
                @Override
                protected char[] getAcceptedChars() {
                    return numberChars;
                }
            });

            String value = preferences.getString("query_time", "2");

            inputServer.setText(value);
            LengthFilter lengthFilter = new LengthFilter(5);
            InputFilter[] inputFilter = { lengthFilter };
            // Limit the input length
            inputServer.setFilters(inputFilter);

            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("Input time(s) for query");
            builder.setView(inputServer);
            builder.setCancelable(false);
            builder.setNegativeButton("Cancel",
                    new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {

                        }
                    });
            builder.setPositiveButton("Ok",
                    new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    if ("".equals(inputServer.getText()
                                            .toString())) {
                                        Toast.makeText(mContext,
                                                "please input value", Toast.LENGTH_LONG)
                                                        .show();
                                        return;
                                    }
                                    mQueryTime.setSummary(inputServer
                                            .getText().toString()+"s");
                                    editor.putString("query_time", inputServer
                                            .getText().toString());
                                    editor.commit();

                                    BehaviorRecordService.time=Integer.valueOf(inputServer
                                            .getText().toString());
                                }
                            });
                        }
                    });
            builder.show();
        }
        return true;
    }

    private void closeService(){
        Log.d(TAG,"close");
        if(bServiceIntent != null){
            stopService(bServiceIntent);
            SystemProperties.set("persist.sys.open.user.record", "false");
            mIsOpenBehaviorRecord = false;
            bServiceIntent=null;
        }
    }

    private void openService(){
        Log.d(TAG,"open");
        bServiceIntent = new Intent(BehaviorInfoRecordActivity.this, BehaviorRecordService.class);
        if(bServiceIntent != null){
            SystemProperties.set("persist.sys.open.user.record", "true");
            mIsOpenBehaviorRecord = true;
            startService(bServiceIntent);
        }
    }
}
