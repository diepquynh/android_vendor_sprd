package com.sprd.generalsecurity.battery;

import java.lang.Override;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.android.internal.os.BatteryStatsHelper;
import com.android.internal.os.BatterySipper;

import android.content.res.Configuration;
import android.os.BatteryStats;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserManager;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.TextView;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.BatteryUtils;

public class BatteryManagement extends Activity {

    private static final String TAG = "BatteryManagement";
    private static final String KEY_STATUS_HEADER = "status_header";

    private static final int MSG_REFRESH_STATS = 1;
    private static final int MSG_REFRESH_CHARGING = 2;
    private static final int MSG_REFRESH_DISCHARGING = 3;
    private Context mContext;
    private BatteryStatsHelper mStatsHelper;
    private UserManager mUm;
    private Intent mBatteryBroadcast;
    private int mBatteryLevel;
    private boolean mIsCharging;
    private TextView mBatteryIcon;
    private LinearLayout item1, item2, item3, item4;
    private static final int ANIMATION_DURATION = 1000;

    private final int[] drawableIds = { R.drawable.battery_0,
            R.drawable.battery_1, R.drawable.battery_2, R.drawable.battery_3,
            R.drawable.battery_4, R.drawable.battery_5, R.drawable.battery_6,
            R.drawable.battery_7, R.drawable.battery_8, R.drawable.battery_9,
            R.drawable.battery_10, R.drawable.battery_11 };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = this;
        mUm = (UserManager) getSystemService(Context.USER_SERVICE);

        initViews(savedInstanceState);
    }

    BatteryFragment fmFragment;
    private void initViews(Bundle savedInstanceState) {
        setContentView(R.layout.manage_battery);
        mBatteryIcon = (TextView) findViewById(R.id.battery_icon);

        new Thread(new Runnable() {
            @Override
            public void run() {
                mStatsHelper = new BatteryStatsHelper(mContext);
                mStatsHelper.create(savedInstanceState);
                mStatsHelper.refreshStats(BatteryStats.STATS_SINCE_CHARGED,
                        mUm.getUserProfiles());
                mBatteryBroadcast = mStatsHelper.getBatteryBroadcast();
                mBatteryLevel = BatteryUtils.getBatteryLevel(mBatteryBroadcast);
                Log.i(TAG, "mBatteryLevel" + mBatteryLevel);
            }
        }).start();
        refreshUi(true);

        FragmentTransaction fragmentTransaction = getFragmentManager()
                .beginTransaction();
        fmFragment = new BatteryFragment();
        fragmentTransaction.add(R.id.modeList, fmFragment);
        fragmentTransaction.commitAllowingStateLoss();

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
    }

    private BroadcastReceiver mBatteryInfoReceiver = new BroadcastReceiver() {
        int tempLevel;

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "battery Receiver     " + intent);
            String action = intent.getAction();
            int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS,
                    BatteryManager.BATTERY_STATUS_UNKNOWN);
            if (Intent.ACTION_BATTERY_CHANGED.equals(action)
                    && updateBatteryStatus(intent)) {
                if (!mHandler.hasMessages(MSG_REFRESH_STATS)) {
                    final Message message = new Message();
                    if (updateBatteryStatus(intent)) {
                        message.arg1 = 1;
                    }
                    message.what = MSG_REFRESH_STATS;
                    mHandler.sendMessage(message);
                }
            }

            Log.i(TAG, "mBatteryLevel:" + mBatteryLevel + "   tempLevel:"
                    + tempLevel);
            if (status == BatteryManager.BATTERY_STATUS_CHARGING) {
                Log.i(TAG, "if  BatteryManager.BATTERY_STATUS_CHARGING");
                tempLevel = BatteryUtils.getPicId(drawableIds.length, mBatteryLevel);
//                tempLevel = 0;
                Log.i(TAG, "tempLevel" + tempLevel + "    mBatteryLevel:"
                        + mBatteryLevel + "   mBatteryLevel/10:"
                        + mBatteryLevel / 10);
                Message msg = new Message();
                msg.what = MSG_REFRESH_CHARGING;
                msg.arg1 = tempLevel;
                mHandler.sendMessage(msg);
            } else {
                Log.i(TAG, "else  BatteryManager.BATTERY_STATUS_CHARGING");
                mHandler.sendEmptyMessage(MSG_REFRESH_DISCHARGING);
            }
        }
    };

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_REFRESH_STATS:
                removeMessages(MSG_REFRESH_STATS);
                boolean updateDrawable = msg.arg1 == 1 ? true : false;
                Log.i(TAG, "refresh drawable: " + updateDrawable);
                mStatsHelper.clearStats();
                refreshUi(updateDrawable);
                break;
            case MSG_REFRESH_CHARGING:
                removeMessages(MSG_REFRESH_CHARGING);
                Log.i(TAG, "msg.arg1:" + msg.arg1);
                refreshUi(msg.arg1);
                Message ms = new Message();
                ms.what = MSG_REFRESH_CHARGING;
                ms.arg1 = (msg.arg1+1) < drawableIds.length ? (msg.arg1 +1) : BatteryUtils.getPicId(drawableIds.length, mBatteryLevel);
//                ms.arg1 = (msg.arg1+1) < drawableIds.length ? (msg.arg1 +1) : 0;
                sendMessageDelayed(ms, ANIMATION_DURATION);
                break;
            case MSG_REFRESH_DISCHARGING:
                removeMessages(MSG_REFRESH_CHARGING);
                refreshUi(true);
                break;
            }
        }
    };

    private void refreshUi(boolean updateDrawable) {
        Log.i(TAG, "--refreshUi--");
        if (updateDrawable && !mHandler.hasMessages(MSG_REFRESH_CHARGING)) {
            int id = BatteryUtils.getPicId(drawableIds.length, mBatteryLevel);
            mBatteryIcon.setBackgroundResource(drawableIds[id]);
            mBatteryIcon.setText(BatteryUtils
                    .getBatteryPercentage(mBatteryLevel));
            Log.i(TAG, "drawable id=" + id);
        } else {
            mBatteryIcon.setText(BatteryUtils
                    .getBatteryPercentage(mBatteryLevel));
        }
    }

    private void refreshUi(int id) {
        Log.i(TAG, "--refreshUi--");
        mBatteryIcon.setBackgroundResource(drawableIds[id]);
    }

    private boolean updateBatteryStatus(Intent intent) {
        if (intent != null) {
            final int batteryLevel = BatteryUtils.getBatteryLevel(intent);
            final boolean isCharging = isCharging(intent);
            if (batteryLevel != mBatteryLevel || isCharging != mIsCharging) {
                mBatteryLevel = batteryLevel;
                mIsCharging = isCharging;
                return true;
            }
        }
        return false;
    }

    private boolean isCharging(Intent intent) {
        int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS,
                BatteryManager.BATTERY_STATUS_UNKNOWN);
        if (status == BatteryManager.BATTERY_STATUS_CHARGING) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
//        mContext.unregisterReceiver(mBatteryInfoReceiver);
        if (isChangingConfigurations()) {
            mStatsHelper.storeState();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mHandler != null) {
            if (mHandler.hasMessages(MSG_REFRESH_CHARGING)) {
                mHandler.removeMessages(MSG_REFRESH_CHARGING);
            }
        }

        mContext.unregisterReceiver(mBatteryInfoReceiver);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mContext.registerReceiver(mBatteryInfoReceiver, new IntentFilter(
                Intent.ACTION_BATTERY_CHANGED));
    }

}
