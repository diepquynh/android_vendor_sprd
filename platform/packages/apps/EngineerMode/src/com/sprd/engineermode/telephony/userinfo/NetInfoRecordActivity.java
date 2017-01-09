package com.sprd.engineermode.telephony.userinfo;

import java.io.File;
import android.os.Bundle;
import android.os.SystemProperties;
import android.os.Handler;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceClickListener;
import android.util.Log;
import android.app.AlertDialog;
import android.widget.EditText;
import android.widget.Toast;
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


public class NetInfoRecordActivity extends PreferenceActivity implements
Preference.OnPreferenceChangeListener{
    private static final String TAG = "NetInfoRecordActivity";

    private static final String KEY_NETINFOSTATISTICS_SWITCH = "key_netinfostatistics_record";
    private static final String KEY_SERVER_CELL_SWITCH = "key_server_cell_record";
    private static final String KEY_NEIGHBOUR_CELL_SWITCH = "key_neighbour_cell_record";
    private static final String KEY_ADJACENT_CELL_SWITCH = "key_adjacent_cell_record";
    private static final String KEY_OUTFIELD_SWITCH = "key_outfield_record";
    private static final String KEY_QUERY_TIME = "query_time";

    public static Intent serviceIntent = null;
    public static boolean mIsOpenNetinfoRecord = SystemProperties.getBoolean("persist.sys.open.net.record", false);
    public static boolean mIsOpenAllNetInfoItems=false;
    public static boolean mIsCloseAllNetInfoItems=false;

    private EMSwitchPreference mNetinfostatistics,mServerCell,mNeighbourCell,mAdjacentCell,mOutfield;
    private Preference mQueryTime;
    private Context mContext;
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    private Handler mUiThread = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_netinfo_record);
        mContext=this;
        mNetinfostatistics = (EMSwitchPreference) findPreference(KEY_NETINFOSTATISTICS_SWITCH);
        mNetinfostatistics.setOnPreferenceChangeListener(this);
        mServerCell = (EMSwitchPreference) findPreference(KEY_SERVER_CELL_SWITCH);
        mServerCell.setOnPreferenceChangeListener(this);
        mNeighbourCell = (EMSwitchPreference) findPreference(KEY_NEIGHBOUR_CELL_SWITCH);
        mNeighbourCell.setOnPreferenceChangeListener(this);
        mAdjacentCell = (EMSwitchPreference) findPreference(KEY_ADJACENT_CELL_SWITCH);
        mAdjacentCell.setOnPreferenceChangeListener(this);
        mOutfield = (EMSwitchPreference) findPreference(KEY_OUTFIELD_SWITCH);
        mOutfield.setOnPreferenceChangeListener(this);

        mQueryTime= (Preference) findPreference(KEY_QUERY_TIME);
        mQueryTime.setOnPreferenceChangeListener(this);
        preferences = mContext
                .getSharedPreferences("netinforecord", mContext.MODE_PRIVATE);
        editor = preferences.edit();
        mQueryTime.setSummary(preferences.getString("query_time", "8")+"s");
    }

    @Override
    protected void onResume() {
        super.onResume();
        mNetinfostatistics.setChecked(false);
        mServerCell.setChecked(false);
        mNeighbourCell.setChecked(false);
        mAdjacentCell.setChecked(false);
        mOutfield.setChecked(false);

        if(preferences.getString("netinfostatistics_record","0").equals("1")){
            mNetinfostatistics.setChecked(true);
        }
        if("1".equals(preferences.getString("server_record","0"))){
            mServerCell.setChecked(true);
        }
        if("1".equals(preferences.getString("neighbour_record","0"))){
            mNeighbourCell.setChecked(true);
        }
        if("1".equals(preferences.getString("adjacent_record","0"))){
            mAdjacentCell.setChecked(true);
        }
        if("1".equals(preferences.getString("outfield_record","0"))){
            mOutfield.setChecked(true);
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        super.onDestroy();
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String key = preference.getKey();

        if(KEY_NETINFOSTATISTICS_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("netinfostatistics_record", "0");
                editor.commit();
                if(!mServerCell.isChecked() && !mNeighbourCell.isChecked() &&
                        !mAdjacentCell.isChecked() && !mOutfield.isChecked()){
                    closeService();
                }
            }else{
                editor.putString("netinfostatistics_record", "1");
                editor.commit();
                if(serviceIntent == null){
                    openService();
                }
            }
        }else if(KEY_SERVER_CELL_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("server_record", "0");
                editor.commit();
                if(!mNeighbourCell.isChecked() &&
                        !mAdjacentCell.isChecked() && !mOutfield.isChecked()){
                    editor.putString("netinfo_record", "0");
                    editor.commit();
                    if(!mNetinfostatistics.isChecked()){
                        closeService();
                    }
                }
            }else{
                editor.putString("server_record", "1");
                editor.putString("netinfo_record", "1");
                editor.commit();
                if(serviceIntent == null){
                    openService();
                }
            }
        }else if(KEY_NEIGHBOUR_CELL_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("neighbour_record", "0");
                editor.commit();
                if(!mServerCell.isChecked() &&
                        !mAdjacentCell.isChecked() && !mOutfield.isChecked()){
                    editor.putString("netinfo_record", "0");
                    editor.commit();
                    if(!mNetinfostatistics.isChecked()){
                        closeService();
                    }
                }
            }else{
                editor.putString("neighbour_record", "1");
                editor.putString("netinfo_record", "1");
                editor.commit();
                if(serviceIntent == null){
                    openService();
                }
            }
        }else if(KEY_ADJACENT_CELL_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("adjacent_record", "0");
                editor.commit();
                if(!mServerCell.isChecked() && !mNeighbourCell.isChecked()
                         && !mOutfield.isChecked()){
                    editor.putString("netinfo_record", "0");
                    editor.commit();
                    if(!mNetinfostatistics.isChecked()){
                        closeService();
                    }
                }
            }else{
                editor.putString("adjacent_record", "1");
                editor.putString("netinfo_record", "1");
                editor.commit();
                if(serviceIntent == null){
                    openService();
                }
            }
        }else if(KEY_OUTFIELD_SWITCH.equals(key)){
            if(((EMSwitchPreference) preference).isChecked()){
                editor.putString("outfield_record", "0");
                editor.commit();
                if(!mServerCell.isChecked() && !mNeighbourCell.isChecked() &&
                        !mAdjacentCell.isChecked()){
                    editor.putString("netinfo_record", "0");
                    editor.commit();
                    if(!mNetinfostatistics.isChecked()){
                        closeService();
                    }
                }
            }else{
                editor.putString("outfield_record", "1");
                editor.putString("netinfo_record", "1");
                editor.commit();
                if(serviceIntent == null){
                    openService();
                }
            }
        }
        return true;
    }

    private void closeService(){
        Log.d(TAG,"close");
        if(serviceIntent != null){
            stopService(serviceIntent);
            SystemProperties.set("persist.sys.open.net.record", "false");
            mIsOpenNetinfoRecord = false;
            serviceIntent=null;
        }
    }

    private void openService(){
        Log.d(TAG,"open");
        serviceIntent = new Intent(NetInfoRecordActivity.this, NetinfoRecordService.class);
        if(serviceIntent != null){
            SystemProperties.set("persist.sys.open.net.record", "true");
            mIsOpenNetinfoRecord = true;
            startService(serviceIntent);
        }
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

            String value = preferences.getString("query_time", "8");

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
                                    NetinfoRecordService.time=Integer.valueOf(inputServer
                                            .getText().toString());
                                }
                            });
                        }
                    });
            builder.show();
        }
        return true;
    }
}
