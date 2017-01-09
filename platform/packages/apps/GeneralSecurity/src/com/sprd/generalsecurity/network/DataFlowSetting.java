package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.app.AppGlobals;
import android.content.pm.IPackageManager;
import android.os.Bundle;
import android.os.ServiceManager;

import android.net.INetworkStatsSession;
import android.net.INetworkStatsService;
import android.net.NetworkTemplate;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.SwitchPreference;
import android.view.MenuItem;

import java.lang.Float;
import java.lang.Object;
import java.lang.Override;
import android.net.NetworkStats;

import java.util.GregorianCalendar;
import static android.net.NetworkTemplate.buildTemplateWifiWildcard;
import static android.net.NetworkTemplate.buildTemplateMobileWildcard;

import com.sprd.generalsecurity.R;

import com.sprd.generalsecurity.network.DataFlowService;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.SeekBarPreference;
import com.sprd.generalsecurity.utils.CustomEditTextPreference;
import com.sprd.generalsecurity.utils.Contract;

import android.preference.PreferenceManager;
import android.content.SharedPreferences;

import android.content.Intent;
import android.util.Log;

import android.widget.SeekBar;


public class DataFlowSetting extends Activity {
    private static final String TAG = "DataFlowSetting";
    private INetworkStatsSession mStatsSession;
    private INetworkStatsService mStatsService;
    private NetworkTemplate mTemplate;
    NetworkStats mStats;
    private GregorianCalendar mCalendar;
    int mSimId;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        Intent intent = getIntent();
        mSimId = intent.getIntExtra(Contract.EXTRA_SIM_ID, 0);
        Log.e(TAG, "sim:" + mSimId);
        int simCount = TeleUtils.getSimCount(this);
        if (simCount == 1 || simCount == 0) {
            mSimId = TeleUtils.getPrimaryCard(this) + 1;
            setTitle(getResources().getString(R.string.data_restrict_sim));
        } else if (simCount == 2) {
            setTitle(String.format(getResources().getString(R.string.data_restrict_title), mSimId));
        }

        PrefsFragment.setSimID(mSimId);

        setContentView(R.layout.data_setting);
        getFragmentManager().beginTransaction().replace(R.id.setting,
                new PrefsFragment()).commit();
    }



    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }

    //BEGIN_INCLUDE(fragment)
    public static class PrefsFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {

        private static final String KEY_MONTH_TOTAL = "key_edit_month_total";
        private static final String  KEY_MONTH_USED = "key_edit_month_used";
        private static final String  KEY_DAY_RESTRICT = "key_day_flow_restrict";
        private static final String KEY_USED_SETTING_TIME = "key_data_use_time";
        private static final String KEY_SEEKBAR = "seek_bar_restrict";
        private static final String KEY_MONTH_REMIND_SWITCH = "key_restrict_month_reminder";
        private static final String KEY_DAY_REMIND_SWITCH = "key_restrict_day_reminder";

        private static final String SIZE_UNIT = " MB";

        private SeekBarPreference seekBarPreference;
        private int mSeekBarProgress;
        private static int mSimID;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            PreferenceManager pm = getPreferenceManager();
            pm.setSharedPreferencesName("sim" + mSimID);
            addPreferencesFromResource(R.xml.data_settings_pref);

            SwitchPreference switchPreference = (SwitchPreference) findPreference(KEY_MONTH_REMIND_SWITCH);
            switchPreference.setOnPreferenceChangeListener(this);

            switchPreference = (SwitchPreference) findPreference(KEY_DAY_REMIND_SWITCH);
            switchPreference.setOnPreferenceChangeListener(this);

            CustomEditTextPreference editTextPreference = (CustomEditTextPreference)findPreference(KEY_MONTH_TOTAL);
            editTextPreference.setOnPreferenceChangeListener(this);
            setSummary(KEY_MONTH_TOTAL, null);

            editTextPreference = (CustomEditTextPreference)findPreference(KEY_MONTH_USED);
            editTextPreference.setOnPreferenceChangeListener(this);
            setSummary(KEY_MONTH_USED, null);

            editTextPreference = (CustomEditTextPreference)findPreference(KEY_DAY_RESTRICT);
            editTextPreference.setOnPreferenceChangeListener(this);
            setSummary(KEY_DAY_RESTRICT, null);

            seekBarPreference = (SeekBarPreference)findPreference(KEY_SEEKBAR);

            seekBarPreference.setOnSeekBarPrefsChangeListener(new SeekBarPreference.OnSeekBarPrefsChangeListener() {
                //public void OnSeekBarChangeListener(SeekBar seekBar, boolean isChecked);
                public void onStopTrackingTouch(String key, SeekBar seekBar) {
                    android.util.Log.e(TAG, "onStopTracking:" + seekBar.getProgress() + ":" + mSeekBarProgress);
                    if (mSeekBarProgress != seekBar.getProgress()) {
                        updateSeekBarSetting();
                        mSeekBarProgress = seekBar.getProgress();
                    }
                }

                public void onStartTrackingTouch(String key, SeekBar seekBar) {

                }

                public void onProgressChanged(String key, SeekBar seekBar, int progress, boolean fromUser) {
                    seekBarPreference.setDataRestrict(progress + 50);
                }
            });
        }

        public static void setSimID(int i) {
            mSimID = i;
        }

        @Override
        public void onResume() {
            super.onResume();

            seekBarPreference = (SeekBarPreference)findPreference(KEY_SEEKBAR);
            mSeekBarProgress = seekBarPreference.getProgress();
            SharedPreferences sp = seekBarPreference.getSharedPreferences();
            if (sp.getString(KEY_MONTH_TOTAL, "0") != "") {
                seekBarPreference.setMonthRestrict(Float.parseFloat(sp.getString(KEY_MONTH_TOTAL, "0")));
                preValueMonthRestrict = (sp.getInt(KEY_SEEKBAR, 0) + 50) * Float.parseFloat(sp.getString(KEY_MONTH_TOTAL, "0"));
                preValueMonthDataUsed = Float.parseFloat(sp.getString(KEY_MONTH_USED, "0"));
            } else {
                seekBarPreference.setMonthRestrict(0f);
            }
            seekBarPreference.setDataRestrict(sp.getInt(KEY_SEEKBAR, 0) + 50);

            updateSeekBarSetting();
        }

        private float preValueMonthRestrict;
        private float preValueMonthDataUsed;
        private void updateSeekBarSetting() {
            seekBarPreference = (SeekBarPreference)findPreference(KEY_SEEKBAR);
            SharedPreferences sp = seekBarPreference.getSharedPreferences();
            seekBarPreference.setMonthRestrict(Float.parseFloat(sp.getString(KEY_MONTH_TOTAL, "0")));
            seekBarPreference.setDataRestrict(sp.getInt(KEY_SEEKBAR, 0) + 50);

            //remind setting changed, start service to check again.
            float newValueMonthRestrict = (sp.getInt(KEY_SEEKBAR, 0) + 50) * Float.parseFloat(sp.getString(KEY_MONTH_TOTAL, "0"));
            if (preValueMonthRestrict != newValueMonthRestrict)  {
                Log.e(TAG, "ooo:" + (preValueMonthRestrict - newValueMonthRestrict) + ":"+ preValueMonthRestrict + ":" + newValueMonthRestrict);
                preValueMonthRestrict = newValueMonthRestrict;
                resetRemindTrigger1AndCheckReminder();
            }

            float newValuseMonthDataUsed = Float.parseFloat(sp.getString(KEY_MONTH_USED, "0"));
            if (preValueMonthDataUsed != newValuseMonthDataUsed) {
                Log.e(TAG, "monthUsed update:" + preValueMonthDataUsed + ":" + newValueMonthRestrict);
                preValueMonthDataUsed = newValuseMonthDataUsed;
                resetRemindTrigger1AndCheckReminder();
            }
        }


        void resetRemindTrigger1AndCheckReminder() {
            SharedPreferences sp = getPreferenceManager().getSharedPreferences();
            Log.e(TAG, "update:" + mSimID + ":" + sp.getString(KEY_MONTH_TOTAL, "0"));

            SharedPreferences.Editor editor = sp.edit();
            editor.putLong("sim_month_remind_time_trigger_warn", 0); //remind trigger time update to 0, so will remind again for the new setting.
            editor.putLong("sim_month_remind_time_trigger_over", 0);
            editor.apply();
            Intent it = new Intent(this.getActivity(), DataFlowService.class);
            getActivity().startService(it);
            Log.e(TAG, "startService :" + it);
        }

        void setSummary(String key, String v) {
            CustomEditTextPreference editTextPreference = (CustomEditTextPreference)findPreference(key);
            if (v == null) {
                PreferenceManager pm = getPreferenceManager();
                SharedPreferences sharedPref = pm.getSharedPreferences();
                v = sharedPref.getString(key, "0");
            }
            editTextPreference.setSummary(v + SIZE_UNIT);
        }

        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            Log.e(TAG, "update 0:" + preference.getKey());
            if (preference.getKey().equalsIgnoreCase(KEY_MONTH_USED)) {
                updateSeekBarSetting();
                SharedPreferences.Editor editor = preference.getEditor();
                editor.putLong(KEY_USED_SETTING_TIME, System.currentTimeMillis());
                editor.apply();
                return true;
            } else if (preference.getKey().equalsIgnoreCase(KEY_MONTH_TOTAL)) {
                updateSeekBarSetting();
                return true;
            } else if (preference.getKey().equalsIgnoreCase(KEY_MONTH_REMIND_SWITCH) ||
                    preference.getKey().equalsIgnoreCase(KEY_DAY_REMIND_SWITCH)) {
                if ((boolean)newValue) {
                    //swich enabled
                    Intent it = new Intent(this.getActivity(), DataFlowService.class);
                    getActivity().startService(it);
                }
            }
//            setSummary(preference.getKey(), (String) newValue);
            return true;
        }
    }
//END_INCLUDE(fragment)


}
