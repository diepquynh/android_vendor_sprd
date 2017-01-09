package com.sprd.systemupdate;

import java.util.HashMap;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.os.SystemProperties;

public class SettingActivity extends PreferenceActivity {
    private CheckBoxPreference mCheckPref;
    private CheckBoxPreference mSpecialCheck;
    private CheckBoxPreference mDailyCheck;
    /** BEGIN BUG565617 zhijie.yang 2016/05/30 **/
    private CheckBoxPreference mAutoPushCheck;
    /** END BUG565617 zhijie.yang 2016/05/30 **/
    private RadioPreference mOneMonthPref;
    private RadioPreference mTwoMonthPref;
    private RadioPreference mThreeMonthPref;

    private Storage mStorage;
    private PendingIntent mPendingIntent;
    private AlarmManager mAlarmManager;
    private TextView mText;
    private int mSelectId;
    private HashMap<Integer, RadioPreference> mAllProfilesMap;

    private static final String TAG = "SystemUpdate-Setting";
    private static final int SELECT_NONE = 0x00;

    @SuppressWarnings("deprecation")
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = new Intent("sprd.systemupdate.action.TIMER");
        mPendingIntent = PendingIntent.getBroadcast(this, 0, intent, 0);
        mAlarmManager = (AlarmManager) getSystemService(ALARM_SERVICE);
        mAllProfilesMap = new HashMap<Integer, RadioPreference>();
        mStorage = Storage.get(this);

        setContentView(R.layout.list_item_textview);
        addPreferencesFromResource(R.xml.setting);
        getListView().setItemsCanFocus(true);
        mText = (TextView) findViewById(R.id.countdown);
        mCheckPref = (CheckBoxPreference) findPreference("timer_setting");
        mSpecialCheck = (CheckBoxPreference) findPreference("enable_special_check");
        mDailyCheck = (CheckBoxPreference) findPreference("enable_daily_check");
        /** BEGIN BUG565617 zhijie.yang 2016/05/30 **/
        mAutoPushCheck = (CheckBoxPreference) findPreference("enable_auto_push_check");
        /** END BUG565617 zhijie.yang 2016/05/30 **/
        mOneMonthPref = (RadioPreference) findPreference("one_month");
        mOneMonthPref.setId(1);
        mTwoMonthPref = (RadioPreference) findPreference("two_month");
        mTwoMonthPref.setId(2);
        mThreeMonthPref = (RadioPreference) findPreference("three_month");
        mThreeMonthPref.setId(3);
    }

    @Override
    protected void onStart() {
        mAllProfilesMap = new HashMap<Integer, RadioPreference>();
        mAllProfilesMap.put(mOneMonthPref.getId(), mOneMonthPref);
        mAllProfilesMap.put(mTwoMonthPref.getId(), mTwoMonthPref);
        mAllProfilesMap.put(mThreeMonthPref.getId(), mThreeMonthPref);

        mSelectId = mStorage.getSelectId();
        if (mSelectId == SELECT_NONE) {
            mSelectId = mOneMonthPref.getId();
            mStorage.setSelectId(mSelectId);
            mStorage.setPretime(System.currentTimeMillis());
            mStorage.setInterval(mOneMonthPref.getTime());
        }

        if (mStorage.getPretime() > System.currentTimeMillis()
                || System.currentTimeMillis() - mStorage.getPretime() > mStorage
                        .getInterval()) {
            mCheckPref.setChecked(false);
            mStorage.setPretime(System.currentTimeMillis());
        }
        if (mSpecialCheck != null) {
            if ("1".equals(SystemProperties.get("persist.sys.special.enable","0"))) {
                mSpecialCheck.setChecked(true);
            } else {
                mSpecialCheck.setChecked(false);
            }
        }
        if (mDailyCheck != null) {
            if ("1".equals(SystemProperties.get("persist.sys.daily.enable","0"))) {
                mDailyCheck.setChecked(true);
            } else {
                mDailyCheck.setChecked(false);
            }
        }
        /** BEGIN BUG565617 zhijie.yang 2016/05/30 **/
        if (mAutoPushCheck != null) {
            if ("1".equals(SystemProperties.get("persist.sys.autopush.enable","0"))) {
                mAutoPushCheck.setChecked(true);
            } else {
                mAutoPushCheck.setChecked(false);
            }
        }
        /** END BUG565617 zhijie.yang 2016/05/30 **/
        updateState();
        super.onStart();
    }

    @Override
    @Deprecated
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        if (preference == mCheckPref) {
            if (mCheckPref.isChecked()) {
                if (mSelectId != 0) {
                    for (int id : mAllProfilesMap.keySet()) {
                        if (id == mSelectId) {
                            RadioPreference value = mAllProfilesMap.get(id);
                            value.setChecked(true);
                            mStorage.setPretime(System.currentTimeMillis());
                            mStorage.setInterval(value.getTime());
                            mAlarmManager.setRepeating(
                                    AlarmManager.RTC_WAKEUP,
                                    System.currentTimeMillis()
                                            + value.getTime(), value.getTime(),
                                    mPendingIntent);
                        }
                    }

                }

            } else {
                mAlarmManager.cancel(mPendingIntent);
                mStorage.setPretime(System.currentTimeMillis());
            }
        } else if (preference == mOneMonthPref) {
            mSelectId = mOneMonthPref.getId();
            mStorage.setSelectId(mSelectId);
            mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP,
                    System.currentTimeMillis() + mOneMonthPref.getTime(),
                    mOneMonthPref.getTime(), mPendingIntent);
            mStorage.setInterval(mOneMonthPref.getTime());
            mStorage.setPretime(System.currentTimeMillis());
        } else if (preference == mTwoMonthPref) {
            mSelectId = mTwoMonthPref.getId();
            mStorage.setSelectId(mSelectId);
            mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP,
                    System.currentTimeMillis() + mTwoMonthPref.getTime(),
                    mTwoMonthPref.getTime(), mPendingIntent);
            mStorage.setInterval(mTwoMonthPref.getTime());
            mStorage.setPretime(System.currentTimeMillis());
        } else if (preference == mThreeMonthPref) {
            mSelectId = mThreeMonthPref.getId();
            mStorage.setSelectId(mSelectId);
            mAlarmManager.setRepeating(AlarmManager.RTC_WAKEUP,
                    System.currentTimeMillis() + mThreeMonthPref.getTime(),
                    mThreeMonthPref.getTime(), mPendingIntent);
            mStorage.setInterval(mThreeMonthPref.getTime());
            mStorage.setPretime(System.currentTimeMillis());
        } else if (preference == mSpecialCheck) {
            if (mSpecialCheck.isChecked()) {
                SystemProperties.set("persist.sys.special.enable", "1");
            } else {
                SystemProperties.set("persist.sys.special.enable", "0");
            }
       /** BEGIN BUG565617 zhijie.yang 2016/05/30 **/
        } else if (preference == mDailyCheck) {
            if (mDailyCheck.isChecked()) {
                SystemProperties.set("persist.sys.daily.enable", "1");
            } else {
                SystemProperties.set("persist.sys.daily.enable", "0");
            }
        } else if (preference == mAutoPushCheck) {
            if (mAutoPushCheck.isChecked()) {
                SystemProperties.set("persist.sys.autopush.enable", "1");
            } else {
                SystemProperties.set("persist.sys.autopush.enable", "0");
            }

        }
        /** END BUG565617 zhijie.yang 2016/05/30 **/
        updateState();
        return true;
    }

    public void updateState() {
        for (int id : mAllProfilesMap.keySet()) {
            RadioPreference value = mAllProfilesMap.get(id);
            if (mSelectId != SELECT_NONE) {
                if (id == mSelectId) {
                    value.setChecked(true);
                } else {
                    value.setChecked(false);
                }
            }
        }

        if (mCheckPref.isChecked()) {
            long interval = mStorage.getInterval(); // 30 60 90 days
            long now = System.currentTimeMillis();
            long pretime = mStorage.getPretime();

            long daytime = 3600 * 24 * 1000;
            long showtime = (daytime + interval - (now - pretime)) / 86400000;
            if (Utils.DEBUG) {
                Log.i(TAG, "showtime:" + showtime);
            }

            if (mSelectId == 0) {
                mText.setText(R.string.please_choose_time);
            } else {
                mText.setText(getString(R.string.there_are) + " " + showtime
                        + " " + getString(R.string.days));
            }

            mText.setVisibility(View.VISIBLE);

            mOneMonthPref.setEnabled(true);
            mTwoMonthPref.setEnabled(true);
            mThreeMonthPref.setEnabled(true);
        } else {
            mOneMonthPref.setEnabled(false);
            mTwoMonthPref.setEnabled(false);
            mThreeMonthPref.setEnabled(false);
            mText.setVisibility(View.GONE);
        }

    }

}
