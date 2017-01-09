
package com.sprd.engineermode.debuglog;

import android.graphics.Color;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;
import android.preference.PreferenceManager;
import android.content.SharedPreferences;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import com.sprd.engineermode.R;

import android.widget.AbsListView;
import android.widget.AbsListView.OnScrollListener;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.List;
import android.content.BroadcastReceiver;

public class PowerOffInfoActivity extends SwitchBaseActivity {
    private static final String TAG = "PowerOffInfoActivity";

    private static List<String> mTimeArray = new ArrayList<String>();
    private static List<String> mShutdownArray = new ArrayList<String>();

    private SwitchMachineAdapter mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPowerOffPref = getSharedPreferences(POWER_OFF_PREF_NAME, Context.MODE_PRIVATE);
        mPowerOffCount = mPowerOffPref.getLong(INFO_COUNT, 0);
        mEditor = mPowerOffPref.edit();
        if (mTimeArray.size() > 0) {
            mTimeArray.clear();
            mShutdownArray.clear();
        }
        for (int i = 1; i <= mPowerOffCount; i++) {
            String key = PREF_INFO_NUM + Integer.toString(i);
            String keyTime = key + PREF_INFO_TIME;
            String keyMode = key + PREF_INFO_MODE;
            Log.d(TAG, "i = " + i + " keyTime = " + keyTime + " keyMode = " + keyMode);
            Log.d(TAG,
                    "mPowerOffPref.getString(keyTime, null) = "
                            + mPowerOffPref.getString(keyTime, null));
            Log.d(TAG,
                    "mPowerOffPref.getString(keyMode, null) = "
                            + mPowerOffPref.getString(keyMode, null));
            mTimeArray.add(mPowerOffPref.getString(keyTime, null));
            mShutdownArray.add(mPowerOffPref.getString(keyMode, null));
        }

    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "mPowerOffCount = " + mPowerOffCount);

        if (mAdapter == null) {
            Log.d(TAG, "mAdapter == null");
            mAdapter = new SwitchMachineAdapter(this, mTimeArray, mShutdownArray, MODE_POWER_OFF);
            mListView.setAdapter(mAdapter);
            mListView.setOnScrollListener(new OnScrollListener() {
                @Override
                public void onScrollStateChanged(AbsListView view, int scrollState) {
                    if (scrollState != AbsListView.OnScrollListener.SCROLL_STATE_IDLE) {
                        // mAdapter.setScrollingBlean(true);
                        // mAdapter.notifyDataSetChanged();
                    }
                }

                @Override
                public void onScroll(AbsListView view, int firstVisibleItem,
                        int visibleItemCount, int totalItemCount) {
                }
            });
        }
        mAdapter.notifyDataSetChanged();
        mListView.setSelection(0);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "onOptionsItemSelected");
        switch (item.getItemId()) {
            case R.id.dump_info:
                Log.i(TAG, "dump_info");
                checkSDCard();
                if (mPowerOffCount == 0) {
                    Toast.makeText(PowerOffInfoActivity.this, R.string.no_data_toast,
                            Toast.LENGTH_SHORT).show();
                } else if (!mIsMounted) {
                    Toast.makeText(PowerOffInfoActivity.this, R.string.no_sd_toast,
                            Toast.LENGTH_SHORT).show();
                } else {
                    saveToSd(POWER_OFF_PATH, POWER_OFF_NAME);
                }
                return true;
            case R.id.clear_info:
                Log.i(TAG, "click clear_info");
                if (mPowerOffCount == 0) {
                    Toast.makeText(PowerOffInfoActivity.this, R.string.no_data_toast,
                            Toast.LENGTH_SHORT).show();
                    return false;
                }
                doReset();
                Toast.makeText(PowerOffInfoActivity.this, R.string.clear_success_toast,
                        Toast.LENGTH_SHORT).show();
                return true;
            default:
                Log.i(TAG, "default");
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void doReset() {
        mPowerOffCount = 0;
        mTimeArray.clear();
        mShutdownArray.clear();
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
        mEditor.clear();
        mEditor.apply();
        invalidateOptionsMenu();
    }

    public static class ShutDownReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            mPowerOffPref = context.getSharedPreferences(POWER_OFF_PREF_NAME, Context.MODE_PRIVATE);

            mPowerOffCount = mPowerOffPref.getLong(INFO_COUNT, 0);

            mPowerOffCount++;
            mEditor = mPowerOffPref.edit();

            SimpleDateFormat sDateFormat = new SimpleDateFormat("MM-dd HH:mm:ss");
            String date = sDateFormat.format(new java.util.Date());
            String shutDownMode = intent.getStringExtra("shutdown_mode");
            Log.d(TAG, "date = " + date + " shutDownMode = " + shutDownMode + " mPowerOffCount = "
                    + mPowerOffCount);
            mEditor.putLong(INFO_COUNT, mPowerOffCount);

            String key = PREF_INFO_NUM + Long.toString(mPowerOffCount);
            String keyTime = key + PREF_INFO_TIME;
            String keyMode = key + PREF_INFO_MODE;

            mEditor.putString(keyTime, date);
            mEditor.putString(keyMode, shutDownMode);
            mEditor.apply();

            if (shutDownMode != null && shutDownMode.contains("no_power")) {
                mBatteryLifeTime = context.getSharedPreferences(BATTERY_LIFE_TIME,
                        Context.MODE_PRIVATE);
                boolean isFirst = mBatteryLifeTime.getBoolean(PREF_IS_FIRST_SHUTDOWN, true);
                Log.d(TAG, "isFirst = " + isFirst);
                if (isFirst) {
                    SharedPreferences.Editor editor = mBatteryLifeTime.edit();
                    editor.putString(PREF_SHUT_DOWN_INFO, date);
                    editor.putBoolean(PREF_IS_FIRST_SHUTDOWN, false);
                    editor.apply();
                }

            }

        }
    }

    @Override
    public void finish() {
        super.finish();
    }

}
