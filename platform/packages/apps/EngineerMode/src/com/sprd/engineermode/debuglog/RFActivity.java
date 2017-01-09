
package com.sprd.engineermode.debuglog;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class RFActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener {

    private static final String TAG = "RFActivity";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";

    private static final String CFU_CONTROL = "persist.sys.callforwarding";

    private static final int SET_CFU = 1;

    private Handler mUiThread = new Handler();
    private RFHanler mRFHandler;
    private Context mContext;

    private ListPreference mSupplementaryServiceQuery;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_rf);
        mContext = this;
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);
        SharedPreferences sharePref = PreferenceManager.getDefaultSharedPreferences(this);
        sharePref.registerOnSharedPreferenceChangeListener(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mRFHandler = new RFHanler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mRFHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mRFHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        finish();
        super.onBackPressed();
    }

    class RFHanler extends Handler {

        public RFHanler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SET_CFU: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(CFU_CONTROL, valueStr);
                    break;
                }
                default:
                    break;
            }
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mRFHandler.obtainMessage(SET_CFU, re);
            mRFHandler.sendMessage(mSupplService);
        }
    }
}
