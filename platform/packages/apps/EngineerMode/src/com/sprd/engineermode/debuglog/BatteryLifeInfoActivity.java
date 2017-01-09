
package com.sprd.engineermode.debuglog;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;
import com.sprd.engineermode.R;

import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.content.BroadcastReceiver;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.preference.PreferenceManager;
import android.content.SharedPreferences;
import android.preference.PreferenceActivity;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.preference.PreferenceCategory;
import com.sprd.engineermode.debuglog.SwitchBaseActivity;
import java.util.Date;
import java.text.ParseException;

public class BatteryLifeInfoActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {
 
    private static final String TAG = "BatteryLifeInfoActivity";

    private static List<String> mTimeArray = new ArrayList<String>();
    private static List<String> mBootModeArray = new ArrayList<String>();

    private static final String KEY_BATTERY_LIFE = "battery_life";
    private static final String KEY_BATTERY_TIME = "battery_time";
    private static final String KEY_OPEN_BATTERY = "open_battery_time";
    private static final String KEY_LOWER_BATTERY = "low_power_items";
    private static final String KEY_LOWER_BATTERY_TIME = "low_power_time";
    private static final String KEY_LIFE_TIME = "life_time";
    //sprd: added for bug 399267
    private static final int UPDATE_BATTERY_LEVEL = 100;
    private SwitchPreference mBatteryLife;
    private PreferenceCategory mBatteryTime;
    private Preference mOpenBatteryTime;
    private Preference mLowPowerTime;
    private Preference mLifeTime;

    private BatteryReceiver mReceiver;

    public static SharedPreferences.Editor mEditor;
    public static SharedPreferences mBatteryLifeTime;

    public static final String BATTERY_LIFE_TIME = "battery_life_time";

    private boolean mIsOpenLife = false;
    private boolean mIsInfoAlive = false;

    private Context mContext;
    private CharSequence mOpenSummary = "unkown";
    private CharSequence mshutdownSummary = "unkown";
    private String mOpenTime = "unkown";
    private int mOpenLevel = 0;
    private String mShutdownTime = "unkown";
    private String mTotalTime = "unkown";
    /* sprd: added for bug 399267 {@*/
    Handler mMainThreadHandler = new Handler() {
        public void handleMessage(Message msg) {
            Log.i(TAG, "handleMessage  msg.what = " + msg.what);
            switch (msg.what) {
                case UPDATE_BATTERY_LEVEL: {
                    updatePrefernce(true);
                    break;
                }
            }
        }
    };
    /** @}*/
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_battery);
        mBatteryLife = (SwitchPreference) findPreference(KEY_BATTERY_LIFE);
        mBatteryLife.setOnPreferenceChangeListener(this);
        mBatteryTime = (PreferenceCategory) findPreference(KEY_BATTERY_TIME);
        mOpenBatteryTime = (Preference) findPreference(KEY_OPEN_BATTERY);
        mLowPowerTime = (Preference) findPreference(KEY_LOWER_BATTERY_TIME);
        mLifeTime = (Preference) findPreference(KEY_LIFE_TIME);

        mBatteryLifeTime = getSharedPreferences(BATTERY_LIFE_TIME, Context.MODE_PRIVATE);
        mEditor = mBatteryLifeTime.edit();
        mContext = this;

        mOpenTime = mBatteryLifeTime.getString(SwitchBaseActivity.PREF_OPEN_TIME, "unkown");
        mOpenLevel = mBatteryLifeTime.getInt(SwitchBaseActivity.PREF_OPEN_BATTERY, 100);
        mShutdownTime = mBatteryLifeTime.getString(SwitchBaseActivity.PREF_SHUT_DOWN_INFO, "unkown");
        mTotalTime = mBatteryLifeTime.getString(SwitchBaseActivity.PREF_TOTAL_TIME, "unkown");
        Log.d(TAG, "onCreate mOpenTime = " + mOpenTime + " mShutdownTime = " + mShutdownTime+" mTotalTime = "+mTotalTime);

        if (mTotalTime.equals("unkown") && !mShutdownTime.equals("unkown")
                && !mOpenTime.equals("unkown")) {
            SimpleDateFormat format = new SimpleDateFormat("MM-dd HH:mm:ss");
            Date startDate = null;
            Date shutDownTime = null;
            try {
                startDate = format.parse(mOpenTime);
                shutDownTime = format.parse(mShutdownTime);
                Log.d(TAG, "startDate = " + startDate + " shutDownTime = " + shutDownTime);
            } catch (ParseException e) {
                Log.d(TAG, "ParseException for startDate shutDownTime");
                e.printStackTrace();
            }
            if (shutDownTime != null && startDate != null) {
                long diff = shutDownTime.getTime() - startDate.getTime();
                long hours = diff / (1000 * 60 * 60);
                long minutes = (diff - hours * (1000 * 60 * 60)) / (1000 * 60);
                long seconds = (diff - hours * (1000 * 60 * 60) - minutes * (1000 * 60)) / (1000);
                Log.d(TAG, " diff = " + diff + " hours = " + hours + " minutes = " + minutes
                        + " seconds = " + seconds);

                mTotalTime = Long.toString(hours) + ":" + Long.toString(minutes) + ":"
                        + Long.toString(seconds);
                Log.d(TAG, " mTotalTime = " + mTotalTime);
            }
        }

        mIsOpenLife = mBatteryLife.isChecked();
        Log.d(TAG, "mIsOpenLife = " + mIsOpenLife);
        updatePrefernce(false);
        /* sprd: added for bug 399267 {@*/
        if(mIsOpenLife){
            Log.d(TAG, "register BatteryReceiver");
            mReceiver = new BatteryReceiver();
            IntentFilter filter = new IntentFilter();
            filter.addAction(Intent.ACTION_BATTERY_CHANGED);
            registerReceiver(mReceiver, filter);
        }
        /** @}*/
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private void updatePrefernce(boolean isChanged) {
        Log.d(TAG, "isChanged = " + isChanged + " mIsOpenLife = " + mIsOpenLife);
        if (mIsOpenLife) {
            if(isChanged){
                mEditor.putString(SwitchBaseActivity.PREF_OPEN_TIME, mOpenTime);
                mEditor.putInt(SwitchBaseActivity.PREF_OPEN_BATTERY, mOpenLevel);
                mEditor.apply();
            }
            Log.d(TAG, "mOpenTime = " + mOpenTime + " mShutdownTime = " + mShutdownTime
                    + "mTotalTime = " + mTotalTime + "mIsOpenLife =" + mIsOpenLife);
            getPreferenceScreen().addPreference(mBatteryTime);
            mOpenSummary = mContext.getString(R.string.open_time, mOpenTime) + "  "
                    + mContext.getString(R.string.battery_percent, mOpenLevel);

            mOpenBatteryTime.setSummary(mOpenSummary);
            if (!mShutdownTime.equals("unkown")) {
                mshutdownSummary = mContext.getString(R.string.open_time, mShutdownTime);
                mLowPowerTime.setSummary(mshutdownSummary);
            } else {
                mLowPowerTime.setSummary(null);
            }
            if (!mTotalTime.equals("unkown")) {
                mLifeTime.setSummary(mTotalTime);
            } else {
                mLifeTime.setSummary(null);
            }

        } else {
            getPreferenceScreen().removePreference(mBatteryTime);
            mEditor.clear();
            mEditor.apply();
            Log.d(TAG, "enter isChanged");
            mTotalTime = "unkown";
            mShutdownTime = "unkown";

        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mBatteryLife) {
            mIsOpenLife = !mBatteryLife.isChecked();
            Log.d(TAG, "onPreferenceChange mIsOpenLife = " + mIsOpenLife);
            /**
             * battery life time is mShutdownTime - mOpenTime
             * mOpenTime is the switch open time, and the batterypercent is the latest Intent.ACTION_BATTERY_CHANGED included
             */
            if (mIsOpenLife) {
                SimpleDateFormat sDateFormat = new SimpleDateFormat("MM-dd HH:mm:ss");
                mOpenTime = sDateFormat.format(new java.util.Date());
                mReceiver = new BatteryReceiver();
                IntentFilter filter = new IntentFilter();
                filter.addAction(Intent.ACTION_BATTERY_CHANGED);
                registerReceiver(mReceiver, filter);
            } else {
                if (mReceiver != null) {
                    unregisterReceiver(mReceiver);
                    mIsInfoAlive = false;
                    mReceiver = null;
                }
            }
            updatePrefernce(true);
        }
        return true;
    }

    private class BatteryReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            Log.i(TAG, "onReceive  action = " + action);
            if (action.equals(Intent.ACTION_BATTERY_CHANGED) && !mIsInfoAlive) {
                final int extra_level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, 0);
                mOpenLevel = extra_level < 0 ? 0 : extra_level > 100 ? 100 : extra_level;
                mIsInfoAlive = true;
                Log.i(TAG, "mOpenTime = " + mOpenTime + " battery level = " + mOpenLevel);
                /* sprd: added for bug 399267 {@*/
                Message msg = mMainThreadHandler.obtainMessage(UPDATE_BATTERY_LEVEL);
                mMainThreadHandler.sendMessage(msg);
                /** @}*/
            }
        }
    }

    @Override
    public void finish() {
        super.finish();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
            mIsInfoAlive = false;
            mReceiver = null;
            Log.d(TAG, "onDestroy  unregisterReceiver mIsInfoAlive= " + mIsInfoAlive);
        }
    }


}
