package com.sprd.systemupdate;

import java.util.HashMap;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class UpgradeActivity extends PreferenceActivity {
    private RadioPreference mRightNowPref;
    private RadioPreference mOneHourPref;
    private RadioPreference mTwoHourPref;
    private RadioPreference mFourHourPref;
    private RadioPreference mTwentyFourHourPref;
    private RadioPreference mNeverPref;
    private Storage mStorage;
    private Utils utils;
    private int mSelectId;
    private HashMap<Integer, RadioPreference> mAllProfilesMap;

    private static final String TAG = "UpgradeActivity.java";
    private Context mContext;

    @SuppressWarnings("deprecation")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mStorage = Storage.get(this);
        mStorage.setState(Storage.State.DOWNLOADED);

        mContext = this;
        utils = new Utils(mContext);
        utils.monitorBatteryState();
        mAllProfilesMap = new HashMap<Integer, RadioPreference>();
        Intent intent = new Intent("sprd.systemupdate.action.ASK_UPGRADE");
        final PendingIntent pendingIntent = PendingIntent.getBroadcast(this, 0,
                intent, 0);
        final AlarmManager alarmManager = (AlarmManager) getSystemService(ALARM_SERVICE);

        setContentView(R.layout.list_item_one_button);
        addPreferencesFromResource(R.xml.version_upgrade);
        getListView().setItemsCanFocus(true);

        Button ok = (Button) findViewById(R.id.download);
        ok.setText(R.string.ok);
        mSelectId = 1;
        mRightNowPref = (RadioPreference) findPreference("right_now");
        mRightNowPref.setId(1);
        mRightNowPref.setChecked(true);
        mOneHourPref = (RadioPreference) findPreference("one_hour");
        mOneHourPref.setId(2);
        mTwoHourPref = (RadioPreference) findPreference("two_hour");
        mTwoHourPref.setId(3);
        mFourHourPref = (RadioPreference) findPreference("four_hour");
        mFourHourPref.setId(4);
        mTwentyFourHourPref = (RadioPreference) findPreference("twenty_four_hour");
        mTwentyFourHourPref.setId(5);
        mNeverPref = (RadioPreference) findPreference("never");
        mNeverPref.setId(6);

        ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mSelectId == 1) {
                    mStorage.setState(Storage.State.DOWNLOADED);
                    if (utils.isUpdateFileExist()) {
                        if (utils.isBatteryPowerEnough()) {
                            utils.ShowUpgradeDialog();
                        }
                    }
                } else if (mSelectId == 6) {
                    mStorage.setState(Storage.State.DOWNLOADED);
                    startActivity(new Intent(UpgradeActivity.this,
                            SystemUpdateActivity.class));
                    finish();
                } else {
                    mStorage.setState(Storage.State.WAIT_UPDATE);
                    RadioPreference curpref = mAllProfilesMap.get(mSelectId);
                    alarmManager.set(AlarmManager.RTC_WAKEUP,
                            System.currentTimeMillis() + curpref.getTime(),
                            pendingIntent);
                    startActivity(new Intent(UpgradeActivity.this,
                            SystemUpdateActivity.class));
                    finish();
                }

            }
        });

    }

    protected void onStart() {
        mAllProfilesMap = new HashMap<Integer, RadioPreference>();
        mAllProfilesMap.put(mRightNowPref.getId(), mRightNowPref);
        mAllProfilesMap.put(mOneHourPref.getId(), mOneHourPref);
        mAllProfilesMap.put(mTwoHourPref.getId(), mTwoHourPref);
        mAllProfilesMap.put(mFourHourPref.getId(), mFourHourPref);
        mAllProfilesMap.put(mTwentyFourHourPref.getId(), mTwentyFourHourPref);
        mAllProfilesMap.put(mNeverPref.getId(), mNeverPref);
        super.onStart();
    }

    protected void onResume() {

        if (getIntent().getIntExtra("from_where", Storage.fromWhere.NIL) == Storage.fromWhere.NOTIFI_OLD) {
            getIntent().putExtra("from_where", Storage.fromWhere.NIL);
            Toast toast = Toast.makeText(mContext,
                    R.string.push_version_is_downloaded, Toast.LENGTH_LONG);
            toast.setGravity(Gravity.CENTER, 0, 0);
            toast.show();

        }

        super.onResume();

    }

    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {

        for (int id : mAllProfilesMap.keySet()) {
            RadioPreference value = mAllProfilesMap.get(id);
            if (preference == value) {
                mSelectId = value.getId();
                value.setChecked(true);
            } else {
                value.setChecked(false);
            }

        }

        return true;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
            startActivity(new Intent(UpgradeActivity.this,
                    SystemUpdateActivity.class));
            finish();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onDestroy() {
        utils.cancelMonitorBatteryState();
        super.onDestroy();
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
    }

}
