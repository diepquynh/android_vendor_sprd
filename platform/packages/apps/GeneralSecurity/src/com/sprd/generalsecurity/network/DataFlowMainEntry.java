package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.app.AppGlobals;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.Context;
import android.content.Intent;
import android.content.Loader;
import android.content.pm.IPackageManager;
import android.content.SharedPreferences;
import android.net.INetworkStatsService;
import android.net.INetworkStatsSession;
import android.net.NetworkStats;
import android.net.NetworkTemplate;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.Contract;
import com.sprd.generalsecurity.utils.Formatter;

import java.io.File;
import java.lang.InterruptedException;
import java.lang.Override;
import java.lang.Throwable;
import java.util.GregorianCalendar;
import java.util.List;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import static android.net.NetworkTemplate.buildTemplateMobileAll;
import static android.net.NetworkTemplate.buildTemplateWifiWildcard;

public class DataFlowMainEntry extends Activity implements View.OnClickListener {

	private static final String SCREEN_SERVICE_ACTION = "com.sprd.generalsecurity.network.screenService";
    private INetworkStatsSession mStatsSession;
    private INetworkStatsService mStatsService;
    private NetworkTemplate mTemplate;
    NetworkStats mStats;
    private GregorianCalendar mCalendar;

    private float mSim1DataTotal;
    private float mSim1DataUsed;
    private float mSim2DataTotal;
    private float mSim2DataUsed;
    private static int mPrimaryCard = 0;

    private static final String KEY_MONTH_TOTAL = "key_edit_month_total";
    private static final String KEY_MONTH_USED = "key_edit_month_used";

    private static final String TEST_SUBSCRIBER_PROP = "test.subscriberid";
    private static final int M2BITS = 1024 * 1024;
    private static int mSimCount;

    private static String TAG = "DataFlowMainEntry";
    private static FloatKeyView mFloatView;

    private static Context mContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = this;
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        mSimCount = TeleUtils.getSimCount(this);

        Log.e(TAG, "TeleUt:" + TeleUtils.getSimCount(this) + ":" + TeleUtils.getPrimaryCard(this));
        mStatsService = INetworkStatsService.Stub.asInterface(
                ServiceManager.getService(Context.NETWORK_STATS_SERVICE));
        try {
            mStatsSession = mStatsService.openSession();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }

        if (mSimCount == 0 || mSimCount == 1) {
            PrefsFragment prefsFragment = new PrefsFragment(this);
            setContentView(R.layout.data_entry);
            if (mSimCount == 0) {
                Button bt = (Button) findViewById(R.id.button);
                bt.setEnabled(false);
            }

            getFragmentManager().beginTransaction().replace(R.id.pref,
                    prefsFragment).commit();
            Button bt = (Button) findViewById(R.id.button);
            bt.setOnClickListener(this);
        } else {
            PrefsFragment prefsFragment = new PrefsFragment(this);
            setContentView(R.layout.data_entry_dual_sim);
            Button bt1 = (Button) findViewById(R.id.button1);
            Button bt2 = (Button) findViewById(R.id.button2);
            bt1.setOnClickListener(this);
            bt2.setOnClickListener(this);

            getFragmentManager().beginTransaction().replace(R.id.pref,
                    prefsFragment).commit();
        }
        initStatChart();
        mFloatView = FloatKeyView.getInstance(this);
    }


    @Override
    protected void onResume() {
        super.onResume();

        mPrimaryCard = TeleUtils.getPrimaryCard(this);
        startNetworkStatsLoader();
    }

    private void initStatChart() {
        if (mSimCount == 0 || mSimCount == 1) {
            updateDataUnsetView(true, 1);
            FrameLayout f = (FrameLayout) findViewById(R.id.imageView);
            int percentUsed = 20;
            f.addView(new PieView(this, percentUsed));
        } else {
            updateDataUnsetView(true, 1);
            updateDataUnsetView(true, 2);
            FrameLayout f = (FrameLayout) findViewById(R.id.imageView);
            int percentUsed = 20;
            f.addView(new PieView(this, percentUsed));

            f = (FrameLayout) findViewById(R.id.imageView2);
            f.addView(new PieView(this, percentUsed));
        }
    }

    /**
     * update the text views visibility
     *
     * @param dataUnset if data is set by user in the preference
     * @param whichSim  sim1 or sim2
     */
    private void updateDataUnsetView(boolean dataUnset, int whichSim) {
        if (whichSim == 1) {
            if (dataUnset) {
                findViewById(R.id.text_data_unset).setVisibility(View.VISIBLE);
                findViewById(R.id.text_data_used_sim1).setVisibility(View.GONE);
                findViewById(R.id.text_data_remained_sim1).setVisibility(View.GONE);
            } else {
                findViewById(R.id.text_data_unset).setVisibility(View.GONE);
                findViewById(R.id.text_data_used_sim1).setVisibility(View.VISIBLE);
                findViewById(R.id.text_data_remained_sim1).setVisibility(View.VISIBLE);
            }
        } else {
            if (dataUnset) {
                findViewById(R.id.text_data_unset2).setVisibility(View.VISIBLE);
                findViewById(R.id.text_data_used_sim2).setVisibility(View.GONE);
                findViewById(R.id.text_data_remained_sim2).setVisibility(View.GONE);
            } else {
                findViewById(R.id.text_data_unset2).setVisibility(View.GONE);
                findViewById(R.id.text_data_used_sim2).setVisibility(View.VISIBLE);
                findViewById(R.id.text_data_remained_sim2).setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    public void onClick(View v) {
        Intent it = new Intent("com.sprd.generalsecurity.network.dataflowsetting");
        if (v.getId() == R.id.button1 || v.getId() == R.id.button) {
            it.putExtra(Contract.EXTRA_SIM_ID, 1);
            startActivity(it);
        } else if (v.getId() == R.id.button2) {
            it.putExtra(Contract.EXTRA_SIM_ID, 2);
            startActivity(it);
        }
    }

    private void updatePieView(int simId, long bitsUsed, long bitsRemained) {
        float simDataTotal = simId == 0 ? mSim1DataTotal : mSim2DataTotal;
        FrameLayout f = simId == 0 ? (FrameLayout) findViewById(R.id.imageView) :
                (FrameLayout) findViewById(R.id.imageView2);

        int percentUsed = 0;
        if (bitsRemained == 0) {
            percentUsed = 100;
        } else {
            percentUsed = bitsUsed > 0 ? (int) (100 * bitsUsed / (simDataTotal * M2BITS)) : 0;
        }
        f.removeAllViews();
        f.addView(new PieView(this, percentUsed));
    }

    void updateStatsChart(int id, NetworkStats data) {
        long totalBytes = 0;
        NetworkStats.Entry entry = null;
        Log.e(TAG, "==========id=" + id + ":" + data.size());
        for (int i = 0; i < data.size(); i++) {
            entry = data.getValues(i, entry);
            totalBytes += (entry.rxBytes + entry.txBytes);
            Log.e(TAG, "--Got +" + entry.uid + ":" + (entry.rxBytes + entry.txBytes));
        }

        if (id == 0) { //SIM1
            SharedPreferences pref;
            if (mSimCount == 1) {
                if (mPrimaryCard == 0) {
                    pref = this.getSharedPreferences("sim1", Context.MODE_PRIVATE);
                } else {
                    pref = this.getSharedPreferences("sim2", Context.MODE_PRIVATE);
                }
            } else {//single card
                pref = this.getSharedPreferences("sim1", Context.MODE_PRIVATE);
            }
            Log.e(TAG, "got:" + pref.getString("key_edit_month_used", "0") + ":" + pref.getString("key_edit_month_total", "0"));
            mSim1DataTotal = Float.parseFloat(pref.getString(KEY_MONTH_TOTAL, "0"));
            mSim1DataUsed = Float.parseFloat(pref.getString(KEY_MONTH_USED, "0"));
            Log.e(TAG, "data:" +mSim1DataTotal + ":" + mSim1DataUsed );
            if (mSim1DataTotal == 0) {
                //Data unset
                updateDataUnsetView(true, 1);
                return;
            }

            updateDataUnsetView(false, 1);

            Log.e(TAG, "total=" + mSim1DataTotal + ":" + mSim1DataUsed + ":" + totalBytes);
            long bitsRemained = (long) ((mSim1DataTotal - mSim1DataUsed) * M2BITS - totalBytes);
            if (bitsRemained < 0) {
                bitsRemained = 0;
            }
            long bitsUsed = (long) (totalBytes + mSim1DataUsed * M2BITS);

            updatePieView(id, bitsUsed, bitsRemained);

            TextView tv = (TextView) findViewById(R.id.text_data_remained_sim1);
            String s = getResources().getString(R.string.data_remained);
            if (mSimCount == 0 || mSimCount == 1) {
                tv.setText(String.format(s, "\n" + Formatter.formatFileSize((Context) this, bitsRemained, false)));
            } else {
                tv.setText(String.format(s, Formatter.formatFileSize((Context) this, bitsRemained, false)));
            }

            tv = (TextView) findViewById(R.id.text_data_used_sim1);
            s = getResources().getString(R.string.data_used);
            if (mSimCount == 0 || mSimCount == 1) {
                tv.setText(String.format(s, Formatter.formatFileSize((Context) this, bitsUsed, true)));
            } else {
                tv.setText(String.format(s, "\n" + Formatter.formatFileSize((Context) this, bitsUsed, true)));
            }
        }
        if (id == 1) {
            SharedPreferences pref = this.getSharedPreferences("sim2", Context.MODE_PRIVATE);

            mSim2DataTotal = Float.parseFloat(pref.getString(KEY_MONTH_TOTAL, "0"));
            mSim2DataUsed = Float.parseFloat(pref.getString(KEY_MONTH_USED, "0"));
            Log.e(TAG, "sim2:" + mSim2DataTotal + ":" + mSim2DataUsed);
            if (mSim2DataTotal == 0) {
                //Data unset
                updateDataUnsetView(true, 2);
                return;
            }

            Log.e(TAG, "total=" + mSim2DataTotal + ":" + mSim2DataUsed + ":" + totalBytes);
            long bitsRemained = (long) ((mSim2DataTotal - mSim2DataUsed) * M2BITS - totalBytes);
            if (bitsRemained < 0) {
                bitsRemained = 0;
            }
            long bitsUsed = (long) (totalBytes + mSim2DataUsed * M2BITS);
            updatePieView(id, bitsUsed, bitsRemained);
            updateDataUnsetView(false, 2);

            TextView tv = (TextView) findViewById(R.id.text_data_remained_sim2);
            String s = getResources().getString(R.string.data_remained);

            if (mSimCount == 0 || mSimCount == 1) {
                tv.setText(String.format(s, "\n" + Formatter.formatFileSize((Context) this, bitsRemained, false)));
            } else {
                tv.setText(String.format(s, Formatter.formatFileSize((Context) this, bitsRemained, false)));
            }

            tv = (TextView) findViewById(R.id.text_data_used_sim2);
            s = getResources().getString(R.string.data_used);
            if (mSimCount == 0 || mSimCount == 1) {
                tv.setText(String.format(s, Formatter.formatFileSize((Context) this, bitsUsed, true)));
            } else {
                tv.setText(String.format(s, "\n" + Formatter.formatFileSize((Context) this, bitsUsed, true)));
            }
        }
    }

    private List<SubscriptionInfo> mSubInfoList;
    SubscriptionManager mSubscriptionManager;

    private boolean isMobileDataAvailable(int subId) {
        return mSubscriptionManager.getActiveSubscriptionInfo(subId) != null;
    }

    void startNetworkStatsLoader() {
        mSubscriptionManager = SubscriptionManager.from(this);

        mSubInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();
        final TelephonyManager tele = TelephonyManager.from(this);
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(this);
        String v = sharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, "0");

        long start = DateCycleUtils.getMonthCycleStart();

        if (mSimCount == 1) {
            //single card inserted, need to check the primary card num
            mTemplate = buildTemplateMobileAll(TeleUtils.getActiveSubscriberId(this));

            // kick off loader for sim1 detailed stats
            getLoaderManager().restartLoader(0,
                    SummaryForAllUidLoader.buildArgs(mTemplate, start, System.currentTimeMillis()), mSummaryCallbacks);

            return;
        }

        //dual card case
        if (mSimCount == 2) {
            SubscriptionInfo sir2 ;
            for (int i = 1; i <=2; i++) {
                mTemplate = buildTemplateMobileAll(
                        TeleUtils.getActiveSubscriberId(this, i));
                // kick off loader for sim1 detailed stats
                getLoaderManager().restartLoader(i - 1,
                        SummaryForAllUidLoader.buildArgs(mTemplate, start, System.currentTimeMillis()), mSummaryCallbacks);
            }
        }
    }

    private final LoaderCallbacks<NetworkStats> mSummaryCallbacks = new LoaderCallbacks<
            NetworkStats>() {
        @Override
        public Loader<NetworkStats> onCreateLoader(int id, Bundle args) {
            return new SummaryForAllUidLoader(DataFlowMainEntry.this, mStatsSession, args);
        }

        @Override
        public void onLoadFinished(Loader<NetworkStats> loader, NetworkStats data) {
            updateStatsChart(loader.getId(), data);
        }

        @Override
        public void onLoaderReset(Loader<NetworkStats> loader) {

        }
    };


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        onBackPressed();
        return true;
    }


    //Mainly for single sim
    public static class PrefsFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener {

        private static final int INDEX_MONTH = 0;
        private static final int INDEX_WEEK = 1;
        private static final int INDEX_DAY = 2;

        private static boolean mDualRelease;//= true;

        private int mPrimarySim = 0;
        private Context mContext;
        SharedPreferences mSharedPref;

        private FloatKeyView mFloatKeyView;
        private boolean mShowRealSpeed, mShowLockFlow;

        private static final String KEY_CYCLE = "pref_data_cycle";
        private static final String KEY_SPEED = "networkspeed_switch";
        private static final String KEY_KEYGUARD = "keyguard_data_switch";

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            mDualRelease = DataFlowMainEntry.mSimCount == 2 ? true : false;

            if (mDualRelease) {
                addPreferencesFromResource(R.xml.data_entry_dual_sim);
            } else {
                addPreferencesFromResource(R.xml.data_entry);
            }
        }

        public PrefsFragment(Context context) {
            super();
            mContext = context;
            mPrimarySim = TeleUtils.getPrimaryCard(mContext);
        }

        public PrefsFragment() {
            super();
        }

        @Override
        public void onResume() {
            super.onResume();

            mSharedPref = PreferenceManager.getDefaultSharedPreferences(getActivity());
            if (!mDualRelease) {
                ListPreference lp = (ListPreference) findPreference(KEY_CYCLE);
                lp.setOnPreferenceChangeListener(this);

                String v = mSharedPref.getString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, "0");
                setSummary(v);
            }
            if (NEW_FEATURE_ENABLED) {

                CheckBoxPreference cp = (CheckBoxPreference) findPreference(KEY_SPEED);
                cp.setOnPreferenceChangeListener(this);
                CheckBoxPreference cpKeyguard = (CheckBoxPreference) findPreference(KEY_KEYGUARD);
                cpKeyguard.setOnPreferenceChangeListener(this);
            }

            mShowLockFlow = mSharedPref.getBoolean("keyguard_data_switch", false);
            mShowRealSpeed = mSharedPref.getBoolean("networkspeed_switch", false);
        }

        void setSummary(String v) {
            int index = Integer.parseInt(v);
            ListPreference lp = (ListPreference) findPreference(KEY_CYCLE);
            switch (index) {
                case INDEX_MONTH:
                    lp.setSummary(getResources().getString(R.string.pref_data_cycle_default));
                    break;
                case INDEX_WEEK:
                    lp.setSummary(getResources().getString(R.string.data_cycle_week));
                    break;
                case INDEX_DAY:
                    lp.setSummary(getResources().getString(R.string.data_cycle_day));
                    break;
            }
        }

        ScreenStateReceiver mScreenStateReceiver = new ScreenStateReceiver();

        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            if (preference.getKey().equalsIgnoreCase(KEY_CYCLE)) {
                setSummary((String) newValue);

                //back up, so when new sim inserted, the new valuse can be resumed.
                if (TeleUtils.getSimSlotCount(mContext) == 2 && TeleUtils.getSimCount(mContext) == 1) {
                    SharedPreferences.Editor editor = mSharedPref.edit();
                    if (mPrimarySim == 0) {
                        editor.putString(DateCycleUtils.KEY_DATA_CYCLE_SIM1, (String) newValue);
                    } else {
                        editor.putString(DateCycleUtils.KEY_DATA_CYCLE_SIM2, (String) newValue);
                    }
                    editor.commit();
                }
                return true;
            }

            if (NEW_FEATURE_ENABLED) {

                if (preference.getKey().equalsIgnoreCase(KEY_SPEED)) {
                    if ((boolean) newValue) {
                        /* SPRD: modify for #612719 @{ */
                        if (mFloatKeyView == null && mContext != null) {
                            mFloatKeyView = FloatKeyView.getInstance(mContext);
                        }
                        if (mFloatKeyView != null) {
                            mFloatKeyView.addToWindow();
                        }
                        /* @} */

                        Intent it = new Intent(this.getActivity(), ScreenStateService.class);
                        getActivity().startService(it);
                        mShowRealSpeed = true;
                    } else {
                        /* SPRD: modify for #612719 @{ */
                        if (mFloatKeyView == null && mContext != null) {
                            mFloatKeyView = FloatKeyView.getInstance(mContext);
                        }
                        /* @} */
                        if (mFloatKeyView != null) {
                            mFloatKeyView.removeFromWindow();
                        }
                        mShowRealSpeed = false;
                        checkToStopScreenStateService();
                    }
                    return true;
                }

                if (preference.getKey().equalsIgnoreCase(KEY_KEYGUARD)) {
                    if ((boolean) newValue) {
                        Intent it = new Intent(this.getActivity(), ScreenStateService.class);
                        getActivity().startService(it);
                        mShowLockFlow = true;
                    } else {
                        mShowLockFlow = false;
                        checkToStopScreenStateService();
                    }
                }
            }
            return true;
        }

        void checkToStopScreenStateService() {
            if (!mShowLockFlow && !mShowRealSpeed) {
                Intent it = new Intent(this.getActivity(), ScreenStateService.class);
                getActivity().stopService(it);
                Log.e(TAG, "stop ScreenStateService");
            }
        }
    }

    private static final boolean NEW_FEATURE_ENABLED = true;
}
