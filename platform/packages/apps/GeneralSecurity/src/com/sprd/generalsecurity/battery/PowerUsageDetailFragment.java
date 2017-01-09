package com.sprd.generalsecurity.battery;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;

import com.android.internal.os.BatterySipper;
import com.android.internal.os.BatterySipper.DrainType;
import com.android.internal.os.BatteryStatsHelper;
import com.android.internal.util.FastPrintWriter;

import android.content.IntentFilter;
import android.os.UserManager;
import android.os.BatteryStats;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.text.DecimalFormat;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ApplicationErrorReport;
import android.app.FragmentTransaction;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserHandle;
import android.os.Process;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import com.android.internal.os.BatteryStatsHelper;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.BatteryUtils;

public class PowerUsageDetailFragment extends PreferenceFragment {
    private static int[] sDrainTypeDesciptions = new int[] {
            R.string.battery_desc_standby, R.string.battery_desc_radio,
            R.string.battery_desc_voice, R.string.battery_desc_wifi,
            R.string.battery_desc_bluetooth, R.string.battery_desc_flashlight,
            R.string.battery_desc_display, R.string.battery_desc_apps,
            R.string.battery_desc_users, R.string.battery_desc_unaccounted,
            R.string.battery_desc_overcounted, R.string.battery_desc_camera, };

    public static void startBatteryDetailPage(Context context,
            BatteryStatsHelper helper, int statsType, BatteryEntry entry) {
        // Initialize mStats if necessary.
        helper.getStats();

        final int dischargeAmount = helper.getStats().getDischargeAmount(
                statsType);
        Bundle args = new Bundle();
        args.putString(PowerUsageDetailFragment.EXTRA_TITLE, entry.name);
        args.putInt(PowerUsageDetailFragment.EXTRA_PERCENT,
                (int) ((entry.sipper.totalPowerMah * dischargeAmount / helper
                        .getTotalPower()) + .5));
        args.putInt(
                PowerUsageDetailFragment.EXTRA_GAUGE,
                (int) Math.ceil(entry.sipper.totalPowerMah * 100
                        / helper.getMaxPower()));
        args.putLong(PowerUsageDetailFragment.EXTRA_USAGE_DURATION,
                helper.getStatsPeriod());
        args.putString(PowerUsageDetailFragment.EXTRA_ICON_PACKAGE,
                entry.defaultPackageName);
        args.putString(PowerUsageDetailFragment.EXTRA_ICON_NAME,
                entry.name);
        args.putInt(PowerUsageDetailFragment.EXTRA_ICON_ID, entry.iconId);
        args.putDouble(PowerUsageDetailFragment.EXTRA_NO_COVERAGE,
                entry.sipper.noCoveragePercent);
        if (entry.sipper.uidObj != null) {
            args.putInt(PowerUsageDetailFragment.EXTRA_UID,
                    entry.sipper.uidObj.getUid());
        }
        args.putSerializable(PowerUsageDetailFragment.EXTRA_DRAIN_TYPE,
                entry.sipper.drainType);

        int userId = UserHandle.myUserId();
        int[] types;
        double[] values;
        switch (entry.sipper.drainType) {
        case APP:
        case USER: {
            BatteryStats.Uid uid = entry.sipper.uidObj;
            types = new int[] { R.string.usage_type_cpu,
                    R.string.usage_type_cpu_foreground,
                    R.string.usage_type_wake_lock, R.string.usage_type_gps,
                    R.string.usage_type_wifi_running,
                    R.string.usage_type_data_recv,
                    R.string.usage_type_data_send,
                    R.string.usage_type_radio_active,
                    R.string.usage_type_data_wifi_recv,
                    R.string.usage_type_data_wifi_send,
                    R.string.usage_type_audio, R.string.usage_type_video,
                    R.string.usage_type_camera, R.string.usage_type_flashlight,
                    R.string.usage_type_computed_power, };
            values = new double[] { entry.sipper.cpuTimeMs,
                    entry.sipper.cpuFgTimeMs, entry.sipper.wakeLockTimeMs,
                    entry.sipper.gpsTimeMs, entry.sipper.wifiRunningTimeMs,
                    entry.sipper.mobileRxPackets, entry.sipper.mobileTxPackets,
                    entry.sipper.mobileActive, entry.sipper.wifiRxPackets,
                    entry.sipper.wifiTxPackets, 0, 0,
                    entry.sipper.cameraTimeMs, entry.sipper.flashlightTimeMs,
                    entry.sipper.totalPowerMah, };

            if (entry.sipper.drainType == BatterySipper.DrainType.APP) {
                Writer result = new StringWriter();
                PrintWriter printWriter = new FastPrintWriter(result, false,
                        1024);
                helper.getStats().dumpLocked(context, printWriter, "",
                        helper.getStatsType(), uid.getUid());
                printWriter.flush();
                args.putString(PowerUsageDetailFragment.EXTRA_REPORT_DETAILS,
                        result.toString());

                result = new StringWriter();
                printWriter = new FastPrintWriter(result, false, 1024);
                helper.getStats().dumpCheckinLocked(context, printWriter,
                        helper.getStatsType(), uid.getUid());
                printWriter.flush();
                args.putString(
                        PowerUsageDetailFragment.EXTRA_REPORT_CHECKIN_DETAILS,
                        result.toString());
                if (uid.getUid() != 0) {
                    userId = UserHandle.getUserId(uid.getUid());
                }
            }
        }
            break;
        case CELL: {
            types = new int[] { R.string.usage_type_on_time,
                    R.string.usage_type_no_coverage,
                    R.string.usage_type_radio_active,
                    R.string.usage_type_computed_power, };
            values = new double[] { entry.sipper.usageTimeMs,
                    entry.sipper.noCoveragePercent, entry.sipper.mobileActive,
                    entry.sipper.totalPowerMah, };
        }
            break;
        case WIFI: {
            types = new int[] { R.string.usage_type_wifi_running,
                    R.string.usage_type_cpu,
                    R.string.usage_type_cpu_foreground,
                    R.string.usage_type_wake_lock,
                    R.string.usage_type_data_recv,
                    R.string.usage_type_data_send,
                    R.string.usage_type_data_wifi_recv,
                    R.string.usage_type_data_wifi_send,
                    R.string.usage_type_computed_power, };
            values = new double[] { entry.sipper.wifiRunningTimeMs,
                    entry.sipper.cpuTimeMs, entry.sipper.cpuFgTimeMs,
                    entry.sipper.wakeLockTimeMs, entry.sipper.mobileRxPackets,
                    entry.sipper.mobileTxPackets, entry.sipper.wifiRxPackets,
                    entry.sipper.wifiTxPackets, entry.sipper.totalPowerMah, };
        }
            break;
        case BLUETOOTH: {
            types = new int[] { R.string.usage_type_on_time,
                    R.string.usage_type_cpu,
                    R.string.usage_type_cpu_foreground,
                    R.string.usage_type_wake_lock,
                    R.string.usage_type_data_recv,
                    R.string.usage_type_data_send,
                    R.string.usage_type_data_wifi_recv,
                    R.string.usage_type_data_wifi_send,
                    R.string.usage_type_computed_power, };
            values = new double[] { entry.sipper.usageTimeMs,
                    entry.sipper.cpuTimeMs, entry.sipper.cpuFgTimeMs,
                    entry.sipper.wakeLockTimeMs, entry.sipper.mobileRxPackets,
                    entry.sipper.mobileTxPackets, entry.sipper.wifiRxPackets,
                    entry.sipper.wifiTxPackets, entry.sipper.totalPowerMah, };
        }
            break;
        case UNACCOUNTED: {
            types = new int[] { R.string.usage_type_total_battery_capacity,
                    R.string.usage_type_computed_power,
                    R.string.usage_type_actual_power, };
            values = new double[] {
                    helper.getPowerProfile().getBatteryCapacity(),
                    helper.getComputedPower(), helper.getMinDrainedPower(), };
        }
            break;
        case OVERCOUNTED: {
            types = new int[] { R.string.usage_type_total_battery_capacity,
                    R.string.usage_type_computed_power,
                    R.string.usage_type_actual_power, };
            values = new double[] {
                    helper.getPowerProfile().getBatteryCapacity(),
                    helper.getComputedPower(), helper.getMaxDrainedPower(), };
        }
            break;
        default: {
            types = new int[] { R.string.usage_type_on_time,
                    R.string.usage_type_computed_power, };
            values = new double[] { entry.sipper.usageTimeMs,
                    entry.sipper.totalPowerMah, };
        }
        }
        args.putIntArray(PowerUsageDetailFragment.EXTRA_DETAIL_TYPES, types);
        args.putDoubleArray(PowerUsageDetailFragment.EXTRA_DETAIL_VALUES,
                values);
        Intent it = new Intent("com.sprd.generalsecurity.battery.PowerUsageDetail", null);
        it.putExtra("detail", args);
        context.startActivity(it);
    }

    public static final int ACTION_DISPLAY_SETTINGS = 1;
    public static final int ACTION_WIFI_SETTINGS = 2;
    public static final int ACTION_BLUETOOTH_SETTINGS = 3;
    public static final int ACTION_WIRELESS_SETTINGS = 4;
    public static final int ACTION_APP_DETAILS = 5;
    public static final int ACTION_LOCATION_SETTINGS = 6;
    public static final int ACTION_FORCE_STOP = 7;
    public static final int ACTION_REPORT = 8;

    public static final int USAGE_SINCE_UNPLUGGED = 1;
    public static final int USAGE_SINCE_RESET = 2;

    public static final String EXTRA_TITLE = "title";
    public static final String EXTRA_PERCENT = "percent";
    public static final String EXTRA_GAUGE = "gauge";
    public static final String EXTRA_UID = "uid";
    public static final String EXTRA_USAGE_SINCE = "since";
    public static final String EXTRA_USAGE_DURATION = "duration";
    public static final String EXTRA_REPORT_DETAILS = "report_details";
    public static final String EXTRA_REPORT_CHECKIN_DETAILS = "report_checkin_details";
    public static final String EXTRA_DETAIL_TYPES = "types"; // Array of usage
                                                                // types (cpu,
                                                                // gps, etc)
    public static final String EXTRA_DETAIL_VALUES = "values"; // Array of
                                                                // doubles
    public static final String EXTRA_DRAIN_TYPE = "drainType"; // DrainType
    public static final String EXTRA_ICON_PACKAGE = "iconPackage"; // String
    public static final String EXTRA_ICON_NAME = "name";
    public static final String EXTRA_NO_COVERAGE = "noCoverage";
    public static final String EXTRA_ICON_ID = "iconId"; // Int

    private static final String TAG = "PowerUsageDetailFragment";

    private static final String KEY_DETAILS_PARENT = "details_parent";
    private static final String KEY_CONTROLS_PARENT = "controls_parent";
    private static final String KEY_MESSAGES_PARENT = "messages_parent";
    private static final String KEY_PACKAGES_PARENT = "packages_parent";
    private static final String KEY_TWO_BUTTONS = "two_buttons";
    private static final String KEY_HIGH_POWER = "high_power";

    private PackageManager mPm;
    private DevicePolicyManager mDpm;
    private int mUsageSince;
    private int[] mTypes;
    private int mUid;
    private String mPkg;
    private double[] mValues;
    private long mStartTime;
    private BatterySipper.DrainType mDrainType;
    private double mNoCoverage; // Percentage of time that there was no coverage

    private PreferenceCategory mDetailsParent;
    private PreferenceCategory mControlsParent;

    private boolean mUsesGps;

    private String[] mPackages;
    String pkg;
    ApplicationInfo mApp;
    ComponentName mInstaller;
    private Preference mHighPower;

    protected BatteryStatsHelper mStatsHelper;
    protected UserManager mUm;
    private String mBatteryLevel;
    private Context mContext;

    public PowerUsageDetailFragment() {
        super();
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mStatsHelper.create(icicle);
        mContext = getActivity();
        mPm = getActivity().getPackageManager();
        mDpm = (DevicePolicyManager) getActivity().getSystemService(
                Context.DEVICE_POLICY_SERVICE);

        addPreferencesFromResource(R.xml.power_usage_details);
        mDetailsParent = (PreferenceCategory) findPreference(KEY_DETAILS_PARENT);
        mControlsParent = (PreferenceCategory) findPreference(KEY_CONTROLS_PARENT);

        createDetails();
    }

    @Override
    public void onStart() {
        super.onStart();
        mStatsHelper.clearStats();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateBatteryStatus(getActivity().registerReceiver(
                mBatteryInfoReceiver,
                new IntentFilter(Intent.ACTION_BATTERY_CHANGED)));
        if (mHandler.hasMessages(MSG_REFRESH_STATS)) {
            mHandler.removeMessages(MSG_REFRESH_STATS);
            mStatsHelper.clearStats();
        }

        mStartTime = android.os.Process.getElapsedCpuTime();

    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mBatteryInfoReceiver);
    }

    @Override
    public void onStop() {
        super.onStop();
        mHandler.removeMessages(MSG_REFRESH_STATS);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (getActivity().isChangingConfigurations()) {
            mStatsHelper.storeState();
        }
    }

    private boolean updateBatteryStatus(Intent intent) {
        if (intent != null) {
            String batteryLevel = BatteryUtils.getBatteryPercentage(intent);
            if (!batteryLevel.equals(mBatteryLevel)) {
                mBatteryLevel = batteryLevel;
                return true;
            }
        }
        return false;
    }

    static final int MSG_REFRESH_STATS = 100;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_REFRESH_STATS:
                mStatsHelper.clearStats();
                refreshStats();
                break;
            }
        }
    };

    private BroadcastReceiver mBatteryInfoReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_BATTERY_CHANGED.equals(action)
                    && updateBatteryStatus(intent)) {
                if (!mHandler.hasMessages(MSG_REFRESH_STATS)) {
                    mHandler.sendEmptyMessageDelayed(MSG_REFRESH_STATS, 500);
                }
            }
        }
    };

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mUm = (UserManager) activity.getSystemService(Context.USER_SERVICE);
        mStatsHelper = new BatteryStatsHelper(activity, true);
    }

    private void createDetails() {
        final Bundle args = getArguments();
        Log.i(TAG,"createDetails:"+args);

        pkg = args.getString(PowerUsageDetailFragment.EXTRA_ICON_PACKAGE);
        mPkg = args.getString(PowerUsageDetailFragment.EXTRA_ICON_PACKAGE);
        mUsageSince = args.getInt(EXTRA_USAGE_SINCE, USAGE_SINCE_UNPLUGGED);
        mUid = args.getInt(EXTRA_UID, 0);
        mPackages = mContext.getPackageManager().getPackagesForUid(mUid);
        mDrainType = (BatterySipper.DrainType) args
                .getSerializable(EXTRA_DRAIN_TYPE);
        mNoCoverage = args.getDouble(EXTRA_NO_COVERAGE, 0);

        mTypes = args.getIntArray(EXTRA_DETAIL_TYPES);
        mValues = args.getDoubleArray(EXTRA_DETAIL_VALUES);

        if (mPackages != null && mPackages.length > 0) {
            try {
                mApp = mContext.getPackageManager().getApplicationInfo(
                        mPackages[0], 0);
            } catch (NameNotFoundException e) {
            }
        } else {
            Log.d(TAG, "No packages!!");
        }
        // check if error reporting is enabled in secure settings
        int enabled = android.provider.Settings.Global.getInt(
                mContext.getContentResolver(),
                android.provider.Settings.Global.SEND_ACTION_APP_ERROR, 0);
        if (mApp != null) {
            mInstaller = ApplicationErrorReport.getErrorReportReceiver(
                    mContext, mPackages[0], mApp.flags);
        }

        refreshStats();

        fillDetailsSection();
        fillControlsSection(mUid);
    }

    protected void refreshStats() {
        mStatsHelper.refreshStats(BatteryStats.STATS_SINCE_CHARGED,
                mUm.getUserProfiles());
    }


    public void onClick(View v) {
        doAction((Integer) v.getTag());
    }

    // utility method used to start sub activity
    private void startApplicationDetailsActivity() {
        // start new fragment to display extended information

        Intent intent = new Intent();
        intent.setAction("android.settings.APPLICATION_DETAILS_SETTINGS");
        Uri uri = Uri.fromParts("package", pkg != null ? pkg : mPackages[0] , null);
        intent.setData(uri);
        mContext.startActivity(intent);
    }

    private void doAction(int action) {
        switch (action) {
        case ACTION_DISPLAY_SETTINGS:
            mContext.startActivity(new Intent(
                    android.provider.Settings.ACTION_DISPLAY_SETTINGS));
            break;
        case ACTION_WIFI_SETTINGS:
            mContext.startActivity(new Intent(
                    android.provider.Settings.ACTION_WIFI_SETTINGS));
            break;
        case ACTION_BLUETOOTH_SETTINGS:
            mContext.startActivity(new Intent(
                    android.provider.Settings.ACTION_BLUETOOTH_SETTINGS));
            break;
        case ACTION_WIRELESS_SETTINGS:
            mContext.startActivity(new Intent(
                    android.provider.Settings.ACTION_WIRELESS_SETTINGS));
            break;
        case ACTION_APP_DETAILS:
            startApplicationDetailsActivity();
            break;
        case ACTION_LOCATION_SETTINGS:
            mContext.startActivity(new Intent(
                    android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
            break;
        }
    }

    private void fillDetailsSection() {
        Log.i(TAG,"fillDetailsSection");
        Log.i(TAG,"mTypes:"+mTypes+" ;mValues.length:"+mValues.length+" mValues:"+mValues);
        if (mTypes != null && mValues != null) {
            for (int i = 0; i < mTypes.length; i++) {
                // Only add an item if the time is greater than zero
                if (mValues[i] <= 0) {
                    Log.i(TAG,"mValues[i]<=0 mValue "+i+":"+mValues[i]);
                    continue;
                }
                final String label = getString(mTypes[i]);
                String value = null;
                switch (mTypes[i]) {
                case R.string.usage_type_data_recv:
                case R.string.usage_type_data_send:
                case R.string.usage_type_data_wifi_recv:
                case R.string.usage_type_data_wifi_send:
                    final long packets = (long) (mValues[i]);
                    value = Long.toString(packets);
                    break;
                case R.string.usage_type_no_coverage:
                    Log.i(TAG,"usage_type_no_coverage mValue "+i+":"+mValues[i]);
                    final int percentage = (int) Math.floor(mValues[i]);
                    value = BatteryUtils.formatPercentage(percentage);
                    break;
                case R.string.usage_type_total_battery_capacity:
                case R.string.usage_type_computed_power:
                case R.string.usage_type_actual_power:
                    Log.i(TAG,"usage_type_actual_power mValue "+i+":"+mValues[i]);
                    final DecimalFormat format = new DecimalFormat("#0.00");
                    value = getActivity().getString(R.string.mah, mValues[i] < 0.01 ? "< 0.01": format.format(mValues[i]).toString());
                    break;
                case R.string.usage_type_gps:
                    mUsesGps = true;
                    // Fall through
                default:
                    Log.i(TAG,"default mValue "+i+":"+mValues[i]);
                    value = String.valueOf(mValues[i]);
                    value = BatteryUtils.formatElapsedTime(getActivity(), mValues[i], true);
                }
                addHorizontalPreference(mDetailsParent, label, value);
            }
        }
    }

    private void addHorizontalPreference(PreferenceCategory parent,
            CharSequence title, CharSequence summary) {
        Preference pref = new Preference(getActivity());
        pref.setLayoutResource(R.layout.horizontal_preference);
        pref.setTitle(title);
        pref.setSummary(summary);
        pref.setSelectable(false);
        parent.addPreference(pref);
    }

    private void fillControlsSection(int uid) {
        Log.i(TAG,"fillControlsSection");
        PackageManager pm = getActivity().getPackageManager();
        String[] packages = pm.getPackagesForUid(uid);

        Log.i(TAG, "pkg:"+pkg);

        boolean removeHeader = true;
        switch (mDrainType) {
        case APP:
            if (packages!=null) {
                Log.i(TAG, "packages length:"+packages.length);
                for (int i=0;i<packages.length;i++) {
                    Log.i(TAG, "\tpackages i:"+packages[i]);
                }
            }
            // If it is a Java application and only one package is associated
            // with the Uid
            if (packages != null) {
                addControl(R.string.battery_action_app_details,
                        R.string.battery_sugg_apps_info, ACTION_APP_DETAILS);
                removeHeader = false;
                // If the application has a settings screen, jump to that
            }
            break;
        case SCREEN:
            addControl(R.string.display_settings,
                    R.string.battery_sugg_display, ACTION_DISPLAY_SETTINGS);
            removeHeader = false;
            break;
        case WIFI:
            addControl(R.string.wifi_settings, R.string.battery_sugg_wifi,
                    ACTION_WIFI_SETTINGS);
            removeHeader = false;
            break;
        case BLUETOOTH:
            addControl(R.string.bluetooth_settings,
                    R.string.battery_sugg_bluetooth_basic,
                    ACTION_BLUETOOTH_SETTINGS);
            removeHeader = false;
            break;
        case CELL:
            if (mNoCoverage > 10) {
                addControl(R.string.radio_controls_title,
                        R.string.battery_sugg_radio, ACTION_WIRELESS_SETTINGS);
                removeHeader = false;
            }
            break;
        }
        if (removeHeader) {
            mControlsParent.setTitle(null);
            Log.i(TAG,"remove all");
        }
    }

    private void addControl(int pageSummary, int actionTitle, final int action) {
        Preference pref = new Preference(getActivity());
        pref.setTitle(actionTitle);
        pref.setLayoutResource(R.layout.horizontal_preference);
        pref.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                doAction(action);
                return true;
            }
        });
        mControlsParent.addPreference(pref);
    }
}
