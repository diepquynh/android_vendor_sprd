package com.sprd.generalsecurity.battery;

import java.lang.Override;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.android.internal.os.BatteryStatsHelper;
import com.android.internal.os.BatterySipper;
import com.android.internal.os.PowerProfile;
import android.os.SystemProperties;

import android.media.AudioManager;
import android.net.Uri;
import android.os.BatteryStats;
import android.os.IPowerManager;
import android.os.PowerManager;
import android.os.ServiceManager;
import android.os.Build;
import android.os.Vibrator;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.Process;
import com.android.internal.os.BatterySipper.DrainType;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.wifi.WifiManager;
import android.nfc.NfcAdapter;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;

import android.util.TypedValue;

import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;
import java.util.Comparator;
import com.sprd.generalsecurity.R;

public class BatteryFragment extends PreferenceFragment implements
        OnPreferenceChangeListener {

    private static final String TAG = "BatteryManagement";
    private static final String KEY_STATUS_HEADER = "status_header";

    private static final int BRIGHTNESS_PERCENT_TEN = 1;
    private static final int BRIGHTNESS_PERCENT_THR = 3;

    private IPowerManager mPower;
    private static final String MODE_SAVE = "battery_mode_save";
    public static final int COMMON_MODE_ID = 0;
    public static final int INTELLIGENT_MODE_ID = 1;
    public static final int LONG_MODE_ID = 2;
    public static final int SLEEP_MODE_ID = 3;

    public static final int TIME_OUT_THR_SECONDS = 30000;
    public static final int TIME_OUT_THR_MINUTES = 1800000;

    private static int mSelectedId;
    private static int mSelectedOldId;

    private Context mContext;
    private UserManager mUm;
    private LinearLayout item1, item2, item3, item4;

    private PreferenceCategory mModeListCategory;
    private PreferenceCategory mHardwareListCategory;
    private PreferenceCategory mSoftwareListCategory;

    private PowerProfile profile;

    private BatteryStatsHelper mStatsHelper;
    private Dialog mDialog;

    private SharedPreferences mSp;
    private static CommonMode mConMode;
    private boolean mSupportBtWifiSoftApCoexit = true;

    private static HashMap<Integer, ModePreference> mAllModeMap;

    final int[] mTitles = new int[] { R.string.commonMode,
            R.string.intelligentMode, R.string.longMode, R.string.sleepMode };

    private WifiManager mWifiManager;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.manage_battery);

        mContext = getActivity();
        mUm = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mPower = IPowerManager.Stub.asInterface(ServiceManager
                .getService("power"));

        mWifiManager = (WifiManager) mContext
                .getSystemService(Context.WIFI_SERVICE);
        if (SystemProperties.get("ro.btwifisoftap.coexist", "true").equals(
                "false")) {
            mSupportBtWifiSoftApCoexit = false;
        }

        mSp = mContext.getSharedPreferences("battery", mContext.MODE_PRIVATE);
        if (mSp != null) {
            int id = mSp.getInt(MODE_SAVE, COMMON_MODE_ID);
            if (checkTheMode(id)) {
                mSelectedId = id;
            } else {
                mSelectedId = COMMON_MODE_ID;
            }
        }

        mModeListCategory = (PreferenceCategory) findPreference("mode_list");
        addModeList();

        mStatsHelper = new BatteryStatsHelper(mContext);
        mStatsHelper.create(savedInstanceState);
        mStatsHelper.refreshStats(BatteryStats.STATS_SINCE_CHARGED,
                mUm.getUserProfiles());

        mHardwareListCategory = (PreferenceCategory) findPreference("hardware_list");
        mSoftwareListCategory = (PreferenceCategory) findPreference("software_list");

        addCategoryList();
    }

    private static final int MIN_AVERAGE_POWER_THRESHOLD_MILLI_AMP = 10;

    @Override
    public void onStart() {
        super.onStart();

        Preference tmpPref;
        tmpPref = mModeListCategory.findPreference("standard_mode");

        tmpPref.setTitle(R.string.commonMode);

        tmpPref = mModeListCategory.findPreference("intelligent_mode");
        tmpPref.setTitle(R.string.intelligentMode);
        tmpPref.setSummary(R.string.intelligentModeSummary);

        tmpPref = mModeListCategory.findPreference("long_mode");
        tmpPref.setTitle(R.string.longMode);
        tmpPref.setSummary(R.string.longModeSummary);

        tmpPref = mModeListCategory.findPreference("sleep_mode");
        tmpPref.setTitle(R.string.sleepMode);
        tmpPref.setSummary(R.string.sleepModeSummary);
    }

    private static boolean isSharedGid(int uid) {
        return UserHandle.getAppIdFromSharedAppGid(uid) > 0;
    }

    private static boolean isSystemUid(int uid) {
        return uid >= Process.SYSTEM_UID && uid < Process.FIRST_APPLICATION_UID;
    }

    private static List<BatterySipper> getCoalescedUsageList(final List<BatterySipper> sippers) {
        final SparseArray<BatterySipper> uidList = new SparseArray<>();

        final ArrayList<BatterySipper> results = new ArrayList<>();
        final int numSippers = sippers.size();
        for (int i = 0; i < numSippers; i++) {
            BatterySipper sipper = sippers.get(i);
            if (sipper.getUid() > 0) {
                int realUid = sipper.getUid();

                // Check if this UID is a shared GID. If so, we combine it with the OWNER's
                // actual app UID.
                if (isSharedGid(sipper.getUid())) {
                    realUid = UserHandle.getUid(UserHandle.USER_SYSTEM,
                            UserHandle.getAppIdFromSharedAppGid(sipper.getUid()));
                }

                // Check if this UID is a system UID (mediaserver, logd, nfc, drm, etc).
                if (isSystemUid(realUid)
                        && !"mediaserver".equals(sipper.packageWithHighestDrain)) {
                    // Use the system UID for all UIDs running in their own sandbox that
                    // are not apps. We exclude mediaserver because we already are expected to
                    // report that as a separate item.
                    realUid = Process.SYSTEM_UID;
                }

                int index = uidList.indexOfKey(realUid);
                if (index < 0) {
                    // New entry.
                    uidList.put(realUid, sipper);
                } else {
                    // Combine BatterySippers if we already have one with this UID.
                    final BatterySipper existingSipper = uidList.valueAt(index);
                    existingSipper.add(sipper);
                    if (existingSipper.packageWithHighestDrain == null
                            && sipper.packageWithHighestDrain != null) {
                        existingSipper.packageWithHighestDrain = sipper.packageWithHighestDrain;
                    }

                    final int existingPackageLen = existingSipper.mPackages != null ?
                            existingSipper.mPackages.length : 0;
                    final int newPackageLen = sipper.mPackages != null ?
                            sipper.mPackages.length : 0;
                    if (newPackageLen > 0) {
                        String[] newPackages = new String[existingPackageLen + newPackageLen];
                        if (existingPackageLen > 0) {
                            System.arraycopy(existingSipper.mPackages, 0, newPackages, 0,
                                    existingPackageLen);
                        }
                        System.arraycopy(sipper.mPackages, 0, newPackages, existingPackageLen,
                                newPackageLen);
                        existingSipper.mPackages = newPackages;
                    }
                }
            } else {
                results.add(sipper);
            }
        }

        final int numUidSippers = uidList.size();
        for (int i = 0; i < numUidSippers; i++) {
            results.add(uidList.valueAt(i));
        }

        // The sort order must have changed, so re-sort based on total power use.
        Collections.sort(results, new Comparator<BatterySipper>() {
            @Override
            public int compare(BatterySipper a, BatterySipper b) {
                return Double.compare(b.totalPowerMah, a.totalPowerMah);
            }
        });
        return results;
    }

    private void addNotAvailableMessage(PreferenceCategory category) {
        final String NOT_AVAILABLE = "not_available";
        Preference notAvailable =  new Preference(mContext);
        notAvailable.setKey(NOT_AVAILABLE);
        notAvailable.setTitle(mContext.getResources().getString(R.string.power_usage_not_available));
        category.addPreference(notAvailable);
    }

    void addCategoryList() {
        mStatsHelper.refreshStats(BatteryStats.STATS_SINCE_CHARGED,
                mUm.getUserProfiles());

        final PowerProfile powerProfile = mStatsHelper.getPowerProfile();
        final BatteryStats stats = mStatsHelper.getStats();
        final double averagePower = powerProfile.getAveragePower(PowerProfile.POWER_SCREEN_FULL);

        boolean addedSomeSoftware = false;
        boolean addedSomeHardware = false;
        if (averagePower >= MIN_AVERAGE_POWER_THRESHOLD_MILLI_AMP) {
            final List<BatterySipper> usageList = getCoalescedUsageList(
                    mStatsHelper.getUsageList());

            final int dischargeAmount = stats != null ?
                                stats.getDischargeAmount(BatteryStats.STATS_SINCE_CHARGED) : 0;
            final int numSippers = usageList.size();

            TypedValue value = new TypedValue();
            mContext.getTheme().resolveAttribute(android.R.attr.colorControlNormal, value, true);
            int colorControl = getContext().getColor(value.resourceId);

            for (int i = 0; i < numSippers; i++) {
                double totalPower = mStatsHelper.getTotalPower();
                final BatterySipper sipper = usageList.get(i);
                final double percentOfTotal = (sipper.totalPowerMah / totalPower) * dischargeAmount;

                if (sipper.totalPowerMah <= 0) {
                    continue;
                }
                if (((int) (percentOfTotal + .5)) < 1) {
                    continue;
                }
                if (sipper.drainType == BatterySipper.DrainType.OVERCOUNTED) {
                    // Don't show over-counted unless it is at least 2/3 the size of
                    // the largest real entry, and its percent of total is more significant
                    if (sipper.totalPowerMah < ((mStatsHelper.getMaxRealPower()*2)/3)) {
                        continue;
                    }
                    if (percentOfTotal < 10) {
                        continue;
                    }
                    if ("user".equals(Build.TYPE)) {
                        continue;
                    }
                }
                if (sipper.drainType == BatterySipper.DrainType.UNACCOUNTED) {
                    // Don't show over-counted unless it is at least 1/2 the size of
                    // the largest real entry, and its percent of total is more significant
                    if (sipper.totalPowerMah < (mStatsHelper.getMaxRealPower()/2)) {
                        continue;
                    }
                    if (percentOfTotal < 5) {
                        continue;
                    }
                    if ("user".equals(Build.TYPE)) {
                        continue;
                    }
                }

                final UserHandle userHandle = new UserHandle(
                        UserHandle.getUserId(sipper.getUid()));
                final BatteryEntry entry = new BatteryEntry(getActivity(),
                        mHandler, mUm, sipper);
                if (entry.name == null || ("Uid:").startsWith(entry.name)) {
                    Log.i(TAG, "entry name:"+entry.name);
                    continue;
                }

                if (entry.sipper.drainType == DrainType.APP) {
                    int uid = entry.sipper.uidObj.getUid();
                    String defaultname = entry.defaultPackageName;
                    String[] packages = mContext.getPackageManager()
                            .getPackagesForUid(uid);
                    Log.i(TAG, "defaultname:" + defaultname
                            + "    packages.length:" + (packages == null ? 0 : packages.length));
                    if (packages != null) {
                        for (int j = 0; j < packages.length; j++) {
                            Log.i(TAG, "\t\tpackage" + j + ":" + packages[j]);
                        }
                    }
                    if (packages != null && packages.length > 0 && defaultname == null) {
                        defaultname = packages[0];
                    }
                    Log.i(TAG, "\t\tdefaultname:" + defaultname);
                    if (entry.getName() == null) {
                        if (defaultname == null) {
                            Log.i(TAG, "\t\tdefaultname=null continue");
                            continue;
                        } else {
                            try {
                                getActivity().getPackageManager().getPackageUid(
                                        defaultname, UserHandle.myUserId());
                            } catch (NameNotFoundException e) {
                                Log.i(TAG, "\t\tdefaultname NameNotFoundException continue");
                                continue;
                            }
                        }
                    }
                }

                final BatteryItemPreference pref = new BatteryItemPreference(
                        mContext, entry);
                final String key = sipper.drainType == DrainType.APP ? sipper.getPackages() != null
                        ? TextUtils.concat(sipper.getPackages()).toString()
                        : String.valueOf(sipper.getUid())
                        : sipper.drainType.toString();
                pref.setKey(key);

                final double percentOfMax = (sipper.totalPowerMah * 100)
                        / mStatsHelper.getMaxPower();
                pref.setPercent(percentOfMax, percentOfTotal);
                if ((sipper.drainType != DrainType.APP || sipper.uidObj.getUid() == 0)
                        && sipper.drainType != DrainType.USER) {
                    pref.setTint(colorControl);
                }

                BatteryEntry.startRequestQueue();
                switch (sipper.drainType) {
                    case APP:
                        mSoftwareListCategory.addPreference(pref);
                        addedSomeSoftware = true;
                        break;
                    case IDLE:
                    case CELL:
                    case PHONE:
                    case WIFI:
                    case BLUETOOTH:
                    case SCREEN:
                    case FLASHLIGHT:
                        mHardwareListCategory.addPreference(pref);
                        addedSomeHardware = true;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!addedSomeSoftware) {
            addNotAvailableMessage(mSoftwareListCategory);
        }
        if (!addedSomeHardware) {
            addNotAvailableMessage(mHardwareListCategory);
        }
    }

    void addModeList() {
        ModePreference commonPref;
        ModePreference intelligentPref;
        ModePreference longPref;
        ModePreference sleepPref;

        if (mAllModeMap == null) {
            mAllModeMap = new HashMap<Integer, ModePreference>();
        }

        if (mAllModeMap.isEmpty()) {
            commonPref = new ModePreference(mContext);
            commonPref.setId(COMMON_MODE_ID);
            commonPref.setKey("standard_mode");
            commonPref.setTitle(mContext.getResources().getString(R.string.commonMode));
            Log.i("xxx",".............");
            commonPref.setPersistent(false);
            commonPref.setOnPreferenceChangeListener(this);

            intelligentPref = new ModePreference(mContext);
            intelligentPref.setKey("intelligent_mode");
            intelligentPref.setTitle(mContext.getResources().getString(R.string.intelligentMode));
            intelligentPref.setSummary(R.string.intelligentModeSummary);
            intelligentPref.setId(INTELLIGENT_MODE_ID);
            intelligentPref.setPersistent(false);
            intelligentPref.setOnPreferenceChangeListener(this);

            longPref = new ModePreference(mContext);
            longPref.setKey("long_mode");
            longPref.setTitle(mContext.getResources().getString(R.string.longMode));
            longPref.setSummary(mContext.getResources().getString(R.string.longModeSummary));
            longPref.setId(LONG_MODE_ID);
            longPref.setPersistent(false);
            longPref.setOnPreferenceChangeListener(this);

            sleepPref = new ModePreference(mContext);
            sleepPref.setKey("sleep_mode");
            sleepPref.setTitle(mContext.getResources().getString(R.string.sleepMode));
            sleepPref.setSummary(mContext.getResources().getString(R.string.sleepModeSummary));
            sleepPref.setId(SLEEP_MODE_ID);
            sleepPref.setPersistent(false);
            sleepPref.setOnPreferenceChangeListener(this);

            mAllModeMap.put(commonPref.getId(), commonPref);
            mAllModeMap.put(intelligentPref.getId(), intelligentPref);
            mAllModeMap.put(longPref.getId(), longPref);
            mAllModeMap.put(sleepPref.getId(), sleepPref);
        } else {
            commonPref = mAllModeMap.get(COMMON_MODE_ID);
            intelligentPref = mAllModeMap.get(INTELLIGENT_MODE_ID);
            longPref = mAllModeMap.get(LONG_MODE_ID);
            sleepPref = mAllModeMap.get(SLEEP_MODE_ID);
        }

        mModeListCategory.addPreference(commonPref);
        mModeListCategory.addPreference(intelligentPref);
        mModeListCategory.addPreference(longPref);
        mModeListCategory.addPreference(sleepPref);

        mSelectedOldId = mSelectedId;
        updateSelectedState();
        updateMode();
    }

    @Override
    public boolean onPreferenceChange(Preference arg0, Object arg1) {
        return false;
    }

    private static void updateSelectedState() {
        for (HashMap.Entry<Integer, ModePreference> mode : mAllModeMap
                .entrySet()) {
            if (mSelectedId != mode.getKey()) {
                ((ModePreference) mode.getValue()).setChecked(false);
            } else {
                ((ModePreference) mode.getValue()).setChecked(true);
            }
        }
    }

    public void updateMode() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    int effects = Settings.System.getInt(mContext.getContentResolver(),
                            Settings.System.SOUND_EFFECTS_ENABLED);
                    int back = Settings.System.getInt(mContext.getContentResolver(),
                            Settings.System.HAPTIC_FEEDBACK_ENABLED);
                    Log.i(TAG, "effects:"+effects+" \t back:"+back);
                } catch (SettingNotFoundException e) {
                    e.printStackTrace();
                    return;
                }
                switch (mSelectedId) {
                case COMMON_MODE_ID:
                    updateByMode(mConMode.brightnessLevel,
                            mConMode.isAirplaneMode == 1,
                            mConMode.isMasterSyncAutomatically,
                            !mConMode.isDataEnabled, mConMode.isClosedWifiHotspot);
                    break;
                case INTELLIGENT_MODE_ID:
                    updateByMode(BRIGHTNESS_PERCENT_THR, false, false, false, false);
                    break;
                case LONG_MODE_ID:
                    updateByMode(BRIGHTNESS_PERCENT_TEN, false, true, true, true);
                    break;
                case SLEEP_MODE_ID:
                    updateByMode(BRIGHTNESS_PERCENT_TEN, true, true, true, true);
                    break;
                }
            }
        }).start();
    }

    private void updateByMode(float brightnessLevel, boolean isOpenAirplaneMode,
            boolean isCloseAutoSync, boolean isCloseDataConnect, boolean isClosedWifiHotspot) {
        setBrightness(brightnessLevel);
        setStandbyScreen();
        closeWlan();
        closeGPS();
        closeBT();
        setVibrator();
        closeMobileData(isCloseDataConnect);
        setAirplaneOn(isOpenAirplaneMode);
        closeAutoSync(isCloseAutoSync);
        closeMobileData(isCloseDataConnect);
        closeNFC();
        setWifiHotspotState(isClosedWifiHotspot);
    }

    private void setWifiHotspotState(boolean isClosedWifiHotspot) {
        boolean change = false;

        Log.i(TAG,"mSelectedId:"+mSelectedId+"\t mSelectedOldId:"+mSelectedOldId);
        switch(mSelectedId) {
            case COMMON_MODE_ID:
                if (mSelectedOldId != COMMON_MODE_ID && mSelectedOldId != INTELLIGENT_MODE_ID) {
                    if (mConMode.isClosedWifiHotspot != true) {
                        change = true;
                    }
                }
                break;
            case INTELLIGENT_MODE_ID:
                if (mSelectedOldId != COMMON_MODE_ID && mSelectedOldId != INTELLIGENT_MODE_ID) {
                    if (mConMode.isClosedWifiHotspot != true) {
                        change = true;
                    }
                }
                break;
            default:
                if (mSelectedOldId == COMMON_MODE_ID || mSelectedOldId == INTELLIGENT_MODE_ID) {
                    if (mConMode.isClosedWifiHotspot != true) {
                        change = true;
                    }
                }
                break;
        }

        boolean airplaneOn = (Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0);
        Log.i(TAG,"Change\t:"+change);
        if (change) {
            Log.i(TAG,"Change");
            Log.i(TAG,"isClosed 1:"+getWifiHotspotInfo(airplaneOn));
            WifiManager wifiManager = (WifiManager) mContext
                    .getSystemService(Context.WIFI_SERVICE);
            wifiManager.setWifiApEnabled(null, false);
            int wifiSavedState = Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.WIFI_SAVED_STATE, 0);
            if (isClosedWifiHotspot) {
                Log.i(TAG,"To close");
                if (wifiSavedState == 1) {
                    wifiManager.setWifiEnabled(true);
                    Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.WIFI_SAVED_STATE, 0);
                }
            } else if (!airplaneOn){
                Log.i(TAG,"To open");
                if (!mSupportBtWifiSoftApCoexit) {
                    BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
                    int btState = adapter.getState();
                    if(btState != BluetoothAdapter.STATE_OFF) {
                        closeBT();
                    }
                }

                Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.SOFTAP_ENABLING_OR_ENABLED, 1);
                if ((wifiSavedState == WifiManager.WIFI_STATE_ENABLING) ||
                        (wifiSavedState == WifiManager.WIFI_STATE_ENABLED)) {
                    wifiManager.setWifiEnabled(false);
                    Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.WIFI_SAVED_STATE, 1);
                }
                wifiManager.setWifiApEnabled(null, true);
            }

            Log.i(TAG,"isClosed 2:"+getWifiHotspotInfo(airplaneOn));
            return;
        }
        Log.i(TAG,"No Change");
    }

    private void closeNFC() {
    }

    private void setBrightness(float level) {
        int automatic = Settings.System.getIntForUser(
                mContext.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL,
                UserHandle.USER_CURRENT);
        boolean isAutoBrightness = automatic != Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
        Log.i(TAG, "isAutoBrightness:" + isAutoBrightness + "\t\t level:"
                + level);
        if (isAutoBrightness) {
            float value = Settings.System.getFloatForUser(
                    mContext.getContentResolver(),
                    Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ, 0,
                    UserHandle.USER_CURRENT);
            Log.i(TAG, "isAutoBrightness:" + isAutoBrightness + "\t\t level:"
                    + level);
            float adj = level;
            if (mSelectedId == COMMON_MODE_ID) {
                if (!mConMode.isAutoBrightness) {
                    Log.i(TAG, "level:"+level);
                    float percent = getBrightnessPercent((int)level);
                    Log.i(TAG, "percent x/10:"+percent);
                    level = getAutoBrightnessLevel(percent);
                    Log.i(TAG, "level:"+level);
                }
                Settings.System.putFloatForUser(
                        mContext.getContentResolver(),
                        Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ,
                        level, UserHandle.USER_CURRENT);
                return;
            }
            adj = getAutoBrightnessLevel(level);
            Log.i(TAG, "value:" + value +"level:"+level+ "\t\t adj:" + adj);
            Settings.System.putFloatForUser(mContext.getContentResolver(),
                    Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ, adj,
                    UserHandle.USER_CURRENT);
            return;
        }

        if (mSelectedId == COMMON_MODE_ID) {
            if (mConMode.isAutoBrightness) {
                Log.i(TAG, "level:"+level);
                float percent = getAutoBrightnessPercent(level);
                Log.i(TAG, "percent x/10:"+percent);
                level = getBrightnessLevel((int)percent);
                Log.i(TAG, "level:"+level);
            }
            Settings.System.putIntForUser(mContext.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS, (int)level,
                    UserHandle.USER_CURRENT);
            return;
        }
        try {
            mPower.setTemporaryScreenBrightnessSettingOverride(getBrightnessLevel((int)level));
            Settings.System.putIntForUser(mContext.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS, getBrightnessLevel((int)level),
                    UserHandle.USER_CURRENT);
        } catch (RemoteException ex) {
        }
    }

    private int getBrightnessLevel(int percent) {
        final PowerManager pm = (PowerManager) mContext
                .getSystemService(Context.POWER_SERVICE);
        final int mMaximumScreenBrightness = pm
                .getMaximumScreenBrightnessSetting();
        Log.i(TAG, "(int) (mMaximumScreenBrightness * percent / 10.0):"+(int)Math.ceil((mMaximumScreenBrightness * percent / 10.0)));
        return (int)(mMaximumScreenBrightness * percent / 10.0);
    }

    private float getBrightnessPercent(int level) {
        final PowerManager pm = (PowerManager) mContext
                .getSystemService(Context.POWER_SERVICE);
        final int mMaximumScreenBrightness = pm
                .getMaximumScreenBrightnessSetting();
        float percent = (float)(level * 10.0 / mMaximumScreenBrightness);
        Log.i(TAG,"level:"+level+"  >> percent:"+percent);
        return percent;
    }

    private void setStandbyScreen() {
        final ContentResolver resolver = mContext.getContentResolver();
        Settings.System.putInt(resolver, SCREEN_OFF_TIMEOUT,
                mSelectedId != COMMON_MODE_ID ? TIME_OUT_THR_SECONDS
                        : mConMode.screenOffTimeout);
    }

    private void closeWlan() {
        WifiManager wifiManager = (WifiManager) mContext
                .getSystemService(Context.WIFI_SERVICE);
        final int wifiApState = wifiManager.getWifiApState();
//        if ((wifiApState == WifiManager.WIFI_AP_STATE_ENABLING)
//                || (wifiApState == WifiManager.WIFI_AP_STATE_ENABLED)) {
//            wifiManager.setWifiApEnabled(null, mSelectedId != COMMON_MODE_ID  ? false : mConMode.isWifiEnabled);
//        }

        wifiManager.setWifiEnabled(mSelectedId != COMMON_MODE_ID ? false : mConMode.isWifiEnabled);
    }

    private void closeGPS() {
        final int currentUserId = ActivityManager.getCurrentUser();
        final ContentResolver resolver = mContext.getContentResolver();
        if (mUm.hasUserRestriction(UserManager.DISALLOW_SHARE_LOCATION,
                new UserHandle(currentUserId))) {
            return;
        }
        Settings.Secure.putIntForUser(resolver, Settings.Secure.LOCATION_MODE,
                mSelectedId != COMMON_MODE_ID ? Settings.Secure.LOCATION_MODE_OFF : mConMode.locationState , currentUserId);
    }

    private void closeBT() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter
                .getDefaultAdapter();
        if (bluetoothAdapter != null) {
            if (mSelectedId == COMMON_MODE_ID && mConMode.isBluetoothEnabled) {
                bluetoothAdapter.enable();
                return;
            }
            bluetoothAdapter.disable();
        }
    }

    private void closeMobileData(boolean closeData) {
        final TelephonyManager telephonyManager = TelephonyManager
                .from(mContext);
        Log.i(TAG, "setdataEnable:"+(!closeData));
        if (telephonyManager != null) {
            telephonyManager.setDataEnabled(SubscriptionManager.getDefaultDataSubscriptionId(), !closeData);
        }
    }

    private void setAirplaneOn(boolean on) {
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, on ? 1 : 0);
        Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        intent.putExtra("state", on);
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void setVibrator() {
        final AudioManager audioManager = (AudioManager) mContext
                .getSystemService(Context.AUDIO_SERVICE);
        final ContentResolver resolver = mContext.getContentResolver();
        Vibrator vibrator = (Vibrator) mContext
                .getSystemService(Context.VIBRATOR_SERVICE);
        boolean hasVibrator = vibrator == null ? false : vibrator.hasVibrator();
        int ringerMode = audioManager.getRingerModeInternal();
        Log.i(TAG,"ringerMode:"+ringerMode);
        //Only has vibrator in Silent mode
        updateSlientVibrate();
        //update touchEffects
        updateTouchMode();
    }

    private void updateSlientVibrate() {
        final AudioManager audioManager = (AudioManager) mContext
                .getSystemService(Context.AUDIO_SERVICE);
        final ContentResolver resolver = mContext.getContentResolver();
        Vibrator vibrator = (Vibrator) mContext
                .getSystemService(Context.VIBRATOR_SERVICE);
        boolean hasVibrator = vibrator == null ? false : vibrator.hasVibrator();
        int ringerMode = audioManager.getRingerModeInternal();

        //Only has vibrator in Silent mode
        if (mSelectedId == COMMON_MODE_ID) {
            audioManager.setRingerModeInternal(mConMode.ringerMode);
        } else {
            if (AudioManager.RINGER_MODE_SILENT == ringerMode) {
                if (hasVibrator) {
                    audioManager.setRingerModeInternal(AudioManager.RINGER_MODE_VIBRATE);

                    Settings.System.putInt(resolver,
                            Settings.System.VIBRATE_WHEN_RINGING, 1);
                    try {
                        Settings.System.putInt(resolver, "vibrate_when_message", 1);
                    } catch (Exception e) {
                        Log.e(TAG, "exception:" + e.toString());
                    }

                    if (hasVibrator) {
                    audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER,
                            AudioManager.VIBRATE_SETTING_ON);
                    audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION,
                            AudioManager.VIBRATE_SETTING_ON);
                    }
                }
            } else if (AudioManager.RINGER_MODE_VIBRATE == ringerMode) {
                Settings.System.putInt(resolver,
                        Settings.System.VIBRATE_WHEN_RINGING, 0);
                try {
                    Settings.System.putInt(resolver, "vibrate_when_message", 0);
                } catch (Exception e) {
                    Log.e(TAG, "exception:" + e.toString());
                }
                if (hasVibrator) {
                audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_OFF);
                audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION, AudioManager.VIBRATE_SETTING_OFF);
                }
            } else {
                Settings.System.putInt(resolver,
                        Settings.System.VIBRATE_WHEN_RINGING, 0);
                try {
                    Settings.System.putInt(resolver, "vibrate_when_message", 0);
                } catch (Exception e) {
                    Log.e(TAG, "exception:" + e.toString());
                }
                if (hasVibrator) {
                    audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_OFF);
                    audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION, AudioManager.VIBRATE_SETTING_OFF);
                }
            }
        }
    }

    /* SPRD 561247: Vibration and remind when touch @{ */
    private void updateTouchMode() {
        final AudioManager audioManager = (AudioManager) mContext
                .getSystemService(Context.AUDIO_SERVICE);
        if (mSelectedId == COMMON_MODE_ID) {
            Settings.System.putInt(mContext.getContentResolver(),
                    Settings.System.SOUND_EFFECTS_ENABLED, mConMode.touchEffects);
            Settings.System.putInt(mContext.getContentResolver(),
                    Settings.System.HAPTIC_FEEDBACK_ENABLED, mConMode.touchFeedback);
            if (mConMode.touchEffects == 1) {
                audioManager.loadSoundEffects();
            } else {
                audioManager.unloadSoundEffects();
            }

        } else {
            Settings.System.putInt(mContext.getContentResolver(),
                    Settings.System.SOUND_EFFECTS_ENABLED, 0);
            Settings.System.putInt(mContext.getContentResolver(),
                    Settings.System.HAPTIC_FEEDBACK_ENABLED, 0);
            audioManager.unloadSoundEffects();

        }
    }
    /* @} */

    private void closeAutoSync(boolean close) {
        final UserHandle userHandle = Process.myUserHandle();
        ContentResolver.setMasterSyncAutomaticallyAsUser(close,
                userHandle.getIdentifier());
    }

    private void showDialogDetail(final int id) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);

        builder.setTitle(mTitles[id]);
        int array = R.array.mode_title;
        switch (id) {
        case LONG_MODE_ID:
            array = R.array.title_for_long;
            break;
        case SLEEP_MODE_ID:
            array = R.array.title_for_sleep;
            break;
        }

        final DialogAdapter adapter = new DialogAdapter(mContext, array, id);

        LayoutInflater inflater = LayoutInflater.from(mContext);
        View view = inflater.inflate(R.layout.battery_dialog, null);
        TextView header = (TextView) view.findViewById(R.id.header);
        ListView listview = (ListView) view.findViewById(R.id.list);
        TextView footer = (TextView) view.findViewById(R.id.footer);
        String[] asks = mContext.getResources().getStringArray(
                R.array.ask_for_dialog);
        header.setText(mContext.getResources().getString(
                R.string.batteryDialogTitle,
                mContext.getResources().getString(mTitles[id])));
        footer.setText(id == INTELLIGENT_MODE_ID ? asks[0] : asks[1]);
        listview.setAdapter(adapter);
        builder.setView(view);

        builder.setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.i(TAG, "setNegativeButton");
                    }
                });
        builder.setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        mSelectedOldId = mSelectedId;
                        mSelectedId = id;
                        updateSelectedState();
                        updateMode();
                    }
                });

        mDialog = builder.show();
        Window localWindow = mDialog.getWindow();
        LayoutParams params = localWindow.getAttributes();

        localWindow.setGravity(70);
        // localWindow.getDecorView().setPadding(1, 1, 1, 1);
        params.width = -1;
        params.height = -2;
        localWindow.setAttributes(params);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {

        if (preference != null && preference instanceof ModePreference) {
            if (mDialog != null && mDialog.isShowing()) {
                mDialog.dismiss();
            }
            if (mSelectedId == COMMON_MODE_ID) {
                if (mConMode != null) {
                    setCommonData();
                }
            }
            ModePreference modePref = (ModePreference) preference;
            final int profileId = modePref.getId();
            switch (profileId) {
            case COMMON_MODE_ID:
                mSelectedOldId = mSelectedId;
                mSelectedId = COMMON_MODE_ID;
                updateSelectedState();
                updateMode();
                break;
            case INTELLIGENT_MODE_ID:
                if (mSelectedId != INTELLIGENT_MODE_ID) {
                    showDialogDetail(INTELLIGENT_MODE_ID);
                }
                break;
            case LONG_MODE_ID:
                if (mSelectedId != LONG_MODE_ID) {
                    showDialogDetail(LONG_MODE_ID);
                }
                break;
            case SLEEP_MODE_ID:
                if (mSelectedId != SLEEP_MODE_ID) {
                    showDialogDetail(SLEEP_MODE_ID);
                }
                break;
            }
        }

        if (preference != null && preference instanceof BatteryItemPreference) {
            final BatteryItemPreference pref = (BatteryItemPreference) preference;
            final BatteryEntry entry = pref.mInfo;
            if (entry != null && entry.sipper != null) {
                PowerUsageDetailFragment.startBatteryDetailPage(mContext, mStatsHelper, BatteryStats.STATS_SINCE_CHARGED, entry);
            }
        }
        return true;
    }

    private void setCommonData() {
        try {
            final ContentResolver resolver = mContext.getContentResolver();
            final int screenOffTimeout = Settings.System.getInt(resolver,
                    SCREEN_OFF_TIMEOUT);
            final float brightnessLevel;

            int automatic = Settings.System.getIntForUser(
                    mContext.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS_MODE,
                    Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL,
                    UserHandle.USER_CURRENT);
            boolean isAutoBrightness = automatic != Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
            if (!isAutoBrightness) {
                brightnessLevel = Settings.System.getIntForUser(resolver,
                        Settings.System.SCREEN_BRIGHTNESS,
                        UserHandle.USER_CURRENT);
            } else {
                brightnessLevel = Settings.System.getFloatForUser(
                        mContext.getContentResolver(),
                        Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ, 0,
                        UserHandle.USER_CURRENT);
            }
            final WifiManager wifiManager = (WifiManager) mContext
                    .getSystemService(Context.WIFI_SERVICE);
            final boolean isWifiEnabled = wifiManager != null ? wifiManager
                    .isWifiEnabled() : false;
            final int locationState = Settings.Secure.getIntForUser(resolver,
                    Settings.Secure.LOCATION_MODE, UserHandle.USER_CURRENT);
            final BluetoothAdapter bluetoothAdapter = BluetoothAdapter
                    .getDefaultAdapter();
            final boolean isBluetoothEnabled = bluetoothAdapter != null ? bluetoothAdapter
                    .isEnabled() : false;
            final AudioManager audioManager = (AudioManager) mContext
                    .getSystemService(Context.AUDIO_SERVICE);
            final int soundMode = Settings.System.getInt(resolver,
                    Settings.System.VIBRATE_WHEN_RINGING, 0);
            final int messageMode = Settings.System.getInt(resolver,
                    "vibrate_when_message", 0);
            final int ringerMode = audioManager.getRingerMode();
            Log.i(TAG, "ringerMode:" + ringerMode);
            final int isAirplaneMode = Settings.Global.getInt(resolver,
                    Settings.Global.AIRPLANE_MODE_ON);
            final boolean isMasterSyncAutomatically = ContentResolver
                    .getMasterSyncAutomatically();
            final TelephonyManager telephonyManager = TelephonyManager
                    .from(mContext);
            // final boolean isDataEnabled = telephonyManager != null ?
            // telephonyManager
            // .getDataEnabled() : false;
            int subId = SubscriptionManager.getDefaultDataSubscriptionId();

            Log.i(TAG, "telephonyManager:" + telephonyManager);
            if (telephonyManager != null) {
                Log.i(TAG, "subId:" + subId
                        + "\t telephonyManager.getDataEnabled(subId):"
                        + telephonyManager.getDataEnabled(subId));
            }
            final boolean isDataEnabled = telephonyManager != null ? telephonyManager
                    .getDataEnabled(subId) : false;

            final int touchEffects = Settings.System.getInt(
                    mContext.getContentResolver(),
                    Settings.System.SOUND_EFFECTS_ENABLED, 0);
            final int touchFeedback = Settings.System.getInt(
                    mContext.getContentResolver(),
                    Settings.System.HAPTIC_FEEDBACK_ENABLED, 0);

            mConMode.brightnessLevel = brightnessLevel;
            mConMode.screenOffTimeout = screenOffTimeout;
            mConMode.isWifiEnabled = isWifiEnabled;
            mConMode.locationState = locationState;
            mConMode.isBluetoothEnabled = isBluetoothEnabled;
            mConMode.soundMode = soundMode;
            mConMode.messageMode = messageMode;
            mConMode.ringerMode = ringerMode;
            mConMode.isAirplaneMode = isAirplaneMode;
            mConMode.isMasterSyncAutomatically = isMasterSyncAutomatically;
            mConMode.isDataEnabled = isDataEnabled;
            mConMode.isAutoBrightness = isAutoBrightness;
            mConMode.touchEffects = touchEffects;
            mConMode.touchFeedback = touchFeedback;
            mConMode.isClosedWifiHotspot = getWifiHotspotInfo(isAirplaneMode != 0);
        } catch (SettingNotFoundException e) {
            e.printStackTrace();
            return;
        }
    }

    private class DialogAdapter extends BaseAdapter {
        private LayoutInflater mInflater;
        private int mModeArrayId;
        private String[] mArrayTitle;
        private int mId;

        public DialogAdapter(Context context, int arrayTitleId, int id) {
            this.mInflater = LayoutInflater.from(context);
            mArrayTitle = context.getResources().getStringArray(arrayTitleId);
            mId = id;
        }

        @Override
        public int getCount() {
            return mArrayTitle != null ? mArrayTitle.length : 0;
        }

        @Override
        public Object getItem(int position) {
            return null;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        class ViewHolder {
            TextView title;
            TextView state;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder = new ViewHolder();
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.battery_dialog_item,
                        null);
                holder.title = (TextView) convertView.findViewById(R.id.title);
                holder.state = (TextView) convertView.findViewById(R.id.state);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            if (mArrayTitle != null) {
                holder.title.setText(mArrayTitle[position]);
                if (position == 0) {
                    holder.state.setTextColor(Color.BLACK);
                    holder.state
                            .setText(mContext.getResources().getStringArray(
                                    R.array.lightLevel)[mId == BatteryFragment.INTELLIGENT_MODE_ID ? 0
                                    : 1]);
                } else if (position == 1) {
                    holder.state.setText(mContext.getResources().getString(
                            R.string.standbyScreen));
                } else if (position == (mArrayTitle.length - 1)) {
                    holder.state.setText(mContext.getResources().getString(
                            R.string.vibartion_only_in_silent));
                } else {
                    if (mId == BatteryFragment.SLEEP_MODE_ID) {
                        if (position == mArrayTitle.length - 2) {
                            holder.state.setTextColor(Color.GREEN);
                            holder.state.setText(mContext.getResources()
                                    .getString(R.string.mode_on));
                        } else {
                            holder.state.setTextColor(Color.RED);
                            holder.state.setText(mContext.getResources()
                                    .getString(R.string.mode_off));
                        }
                    } else {
                        holder.state.setTextColor(Color.RED);
                        holder.state.setText(mContext.getResources().getString(
                                R.string.mode_off));
                    }
                }
            }
            return convertView;
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mSp != null) {
            Editor edit = mSp.edit();
            edit.putInt(MODE_SAVE, mSelectedId);
            edit.commit();
        }
        mHandler.removeMessages(BatteryEntry.MSG_UPDATE_NAME_ICON);
        BatteryEntry.stopRequestQueue();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mDialog != null && mDialog.isShowing()) {
            mDialog.dismiss();
        }
        if (getActivity().isChangingConfigurations()) {
            BatteryEntry.clearUidCache();
        }

    }

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
            case BatteryEntry.MSG_UPDATE_NAME_ICON:
                BatteryEntry entry = (BatteryEntry) msg.obj;
                BatteryItemPreference bip = (BatteryItemPreference) findPreference(Integer
                        .toString(entry.sipper.uidObj.getUid()));
                if (bip != null) {
                    final int userId = UserHandle.getUserId(entry.sipper
                            .getUid());
                    final UserHandle userHandle = new UserHandle(userId);
                    bip.updateNameAndIcon(entry.name, mUm.getBadgedIconForUser(entry.getIcon(),
                            userHandle));
                }
                break;
            case BatteryEntry.MSG_REPORT_FULLY_DRAWN:
                Activity activity = getActivity();
                if (activity != null) {
                    activity.reportFullyDrawn();
                }
                break;
            }
            super.handleMessage(msg);
        }
    };

    private boolean checkTheMode(int id) {
        CommonMode tempConMode = mConMode;
        try {
            final ContentResolver resolver = mContext.getContentResolver();
            final int screenOffTimeout = Settings.System.getInt(resolver,
                    SCREEN_OFF_TIMEOUT);
            final float brightnessLevel;

            int automatic = Settings.System.getIntForUser(
                    mContext.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS_MODE,
                    Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL,
                    UserHandle.USER_CURRENT);
            boolean isAutoBrightness = automatic != Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;
            if (!isAutoBrightness) {
                brightnessLevel = Settings.System.getIntForUser(resolver,
                        Settings.System.SCREEN_BRIGHTNESS, UserHandle.USER_CURRENT);
            } else {
                brightnessLevel = Settings.System.getFloatForUser(
                        mContext.getContentResolver(),
                        Settings.System.SCREEN_AUTO_BRIGHTNESS_ADJ, 0,
                        UserHandle.USER_CURRENT);
            }
            final WifiManager wifiManager = (WifiManager) mContext
                    .getSystemService(Context.WIFI_SERVICE);
            final boolean isWifiEnabled = wifiManager != null ? wifiManager
                    .isWifiEnabled() : false;
            final int locationState = Settings.Secure.getIntForUser(resolver,
                    Settings.Secure.LOCATION_MODE, UserHandle.USER_CURRENT);
            final BluetoothAdapter bluetoothAdapter = BluetoothAdapter
                    .getDefaultAdapter();
            final boolean isBluetoothEnabled = bluetoothAdapter != null ? bluetoothAdapter
                    .isEnabled() : false;
            final AudioManager audioManager = (AudioManager) mContext
                    .getSystemService(Context.AUDIO_SERVICE);
            final int soundMode = Settings.System.getInt(resolver,
                    Settings.System.VIBRATE_WHEN_RINGING, 0);
            final int messageMode = Settings.System.getInt(resolver, "vibrate_when_message", 0);
            final int ringerMode = audioManager.getRingerMode();
            Log.i(TAG,"ringerMode:"+ringerMode);
            final int isAirplaneMode = Settings.Global.getInt(resolver,
                    Settings.Global.AIRPLANE_MODE_ON);
            final boolean isMasterSyncAutomatically = ContentResolver
                    .getMasterSyncAutomatically();
            final TelephonyManager telephonyManager = TelephonyManager
                    .from(mContext);

            int subId = SubscriptionManager.getDefaultDataSubscriptionId();

            Log.i(TAG,"telephonyManager:"+telephonyManager);
            if (telephonyManager != null) {
                Log.i(TAG,"subId:"+subId+"\t telephonyManager.getDataEnabled(subId):"+telephonyManager.getDataEnabled(subId));
            }
            final boolean isDataEnabled = telephonyManager != null ? telephonyManager
                    .getDataEnabled(subId) : false;

            /* SPRD 561247: Vibration and remind when touch @{ */
            final int touchEffects = Settings.System.getInt(mContext.getContentResolver(),
                            Settings.System.SOUND_EFFECTS_ENABLED, 0);
            final int touchFeedback = Settings.System.getInt(mContext.getContentResolver(),
                            Settings.System.HAPTIC_FEEDBACK_ENABLED, 0);
            final boolean isClosedWifiHotspot = getWifiHotspotInfo(isAirplaneMode != 0);
            Log.i(TAG,"isClosedWifiHotspot:"+isClosedWifiHotspot);
            /* @} */
            mConMode = new CommonMode(brightnessLevel, screenOffTimeout,
                    isWifiEnabled, locationState, isBluetoothEnabled,
                    soundMode, messageMode, ringerMode, isAirplaneMode,
                    isMasterSyncAutomatically, isDataEnabled, isAutoBrightness, touchEffects, touchFeedback, isClosedWifiHotspot);

            if (id == COMMON_MODE_ID) {
                return true;
            }
            Log.i(TAG, "subId:"+subId+"\t isDataEnabled:"+isDataEnabled);
            Log.i(TAG, "touchEffects:"+touchEffects+"\t touchFeedback:"+touchFeedback);
            /* SPRD 561247: Vibration and remind when touch @{ */
            if (touchEffects != 0 || touchFeedback != 0) {
                Log.i(TAG, "touchEffects return");
                return false;
            }
            /* @} */
            if (id == SLEEP_MODE_ID) {
                if (isAirplaneMode != 1) {
                    Log.i(TAG, "isAirplaneMode!=1 return");
                    return false;
                }
            } else {
                if (isAirplaneMode == 1) {
                    Log.i(TAG, "isAirplaneMode==1 return");
                    return false;
                }
            }
            if (id == INTELLIGENT_MODE_ID) {
                if (isMasterSyncAutomatically) {
                    Log.i(TAG, "isMasterSyncAutomatically return");
                    return false;
                }
                if (!(telephonyManager != null && isDataEnabled && subId > 0)) {
                    Log.i(TAG, "isDataEnabled return"+"\t telephonyManager:"+telephonyManager);
//                    return false;
                }
                if (!isAutoBrightness) {
                    if ((int)brightnessLevel != getBrightnessLevel(BRIGHTNESS_PERCENT_THR)) {
                        Log.i(TAG, "isAutoBrightness if return");
                        return false;
                    }
                } else {
                    if (brightnessLevel != getAutoBrightnessLevel(BRIGHTNESS_PERCENT_THR)) {
                        Log.i(TAG, "isAutoBrightness else return");
                        return false;
                    }
                }
            }
            if (id != INTELLIGENT_MODE_ID) {
                if (!isAutoBrightness) {
                    if ((int)brightnessLevel != getBrightnessLevel(BRIGHTNESS_PERCENT_TEN)) {
                        Log.i(TAG, "isAutoBrightness if return");
                        return false;
                    }
                } else {
                    if (brightnessLevel != getAutoBrightnessLevel(BRIGHTNESS_PERCENT_TEN)) {
                        Log.i(TAG, "isAutoBrightness else return");
                        return false;
                    }
                }
                if (telephonyManager != null && isDataEnabled) {
                    Log.i(TAG, "isDataEnabled return"+"\t telephonyManager:"+telephonyManager);
                    return false;
                }
                if (!isClosedWifiHotspot) {
                    Log.i(TAG, "2 isClosedWifiHotspot"+isClosedWifiHotspot);
                    return false;
                }
            }
            if (screenOffTimeout != 30000) {
                Log.i(TAG, "screenOffTimeout");
                return false;
            }
            if (wifiManager != null && isWifiEnabled) {
                Log.i(TAG, "isWifiEnabled");
                return false;
            }
            if (Settings.Secure.LOCATION_MODE_OFF != locationState) {
                Log.i(TAG, "locationState");
                return false;
            }
            if (bluetoothAdapter != null && isBluetoothEnabled) {
                Log.i(TAG, "isBluetoothEnabled");
                return false;
            }
            //Check virbrate
            if (!checkVibrate(id)) {
//            if (soundMode != 0 | ringerMode != AudioManager.RINGER_MODE_VIBRATE) {
//                Log.i(TAG, "RINGER_MODE_VIBRATE");
//                checkVibrate(id);
                return false;
            }
        } catch (SettingNotFoundException e) {
            Log.i(TAG, "SettingNotFoundException");
            e.printStackTrace();
            return false;
        }
        if (tempConMode != null) {
            mConMode = tempConMode;
        }
        tempConMode = null;
        return true;
    }

    private boolean checkVibrate(int id) {

        final AudioManager audioManager = (AudioManager) mContext
                .getSystemService(Context.AUDIO_SERVICE);
        final ContentResolver resolver = mContext.getContentResolver();
        Vibrator vibrator = (Vibrator) mContext
                .getSystemService(Context.VIBRATOR_SERVICE);
        boolean hasVibrator = vibrator == null ? false : vibrator.hasVibrator();
        int ringerMode = audioManager.getRingerModeInternal();

        if (AudioManager.RINGER_MODE_SILENT == ringerMode) {
            if (hasVibrator) {
//                boolean isEffects = Settings.System.getInt(resolver,
//                        Settings.System.SOUND_EFFECTS_ENABLED, 0) == 1;
                 audioManager.setRingerModeInternal(AudioManager.RINGER_MODE_VIBRATE);

                 Settings.System.putInt(resolver,
                         Settings.System.VIBRATE_WHEN_RINGING, 1);
                 Settings.System.putInt(resolver, "vibrate_when_message", 1);
                 audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER,
                         AudioManager.VIBRATE_SETTING_ONLY_SILENT);
                 audioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION,
                         AudioManager.VIBRATE_SETTING_ONLY_SILENT);
            }
            return true;
            //Don't have Vibrator in this phone
        } else if (AudioManager.RINGER_MODE_VIBRATE == ringerMode) {

            if (hasVibrator) {
                if (Settings.System.getInt(resolver,
                        Settings.System.VIBRATE_WHEN_RINGING, 0) != 0) {
                    Log.i(TAG, "7");
//                    return false;
                }
                try {
                    if (/*Settings.System.getInt(resolver, "vibrate_when_message") != null && */Settings.System.getInt(resolver, "vibrate_when_message") != 0) {
                        Log.i(TAG, "8");
//                        return false;
                    }
                } catch (SettingNotFoundException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            Log.i(TAG, "9");
//            return false;
            return true;
        } else {
                if (audioManager.getVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER) != AudioManager.VIBRATE_SETTING_OFF) {
                    Log.i(TAG, "11");
                    return false;
                }
                if (audioManager.getVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION) != AudioManager.VIBRATE_SETTING_OFF) {
                    Log.i(TAG, "12");
                    return false;
                }
                if (Settings.System.getInt(resolver,
                        Settings.System.VIBRATE_WHEN_RINGING, 0) != 0) {
                    Log.i(TAG, "13");
                    return false;
                }

                try {
                    if (Settings.System.getInt(resolver, "vibrate_when_message") != 0) {
                        Log.i(TAG, ""+Settings.System.getInt(resolver, "vibrate_when_message"));
                        Log.i(TAG, "14");
                        return false;
                    }
                } catch (SettingNotFoundException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        Log.i(TAG, "15");
        return true;
    }
    private float getAutoBrightnessLevel(int percent) {
        return getAutoBrightnessLevel((float) percent);
    }

    private float getAutoBrightnessLevel(float percent) {
        final float autoLevel = (float) ((percent * 10) * (2/100.0)) - 1;
        Log.i(TAG,"autoLevel:"+autoLevel);
        return autoLevel;
    }

    private float getAutoBrightnessPercent(float autoLevel) {
        final float percent = (int)((autoLevel + 1) / (2/100.0) / 10.0);
        Log.i(TAG,"percent:"+percent);
        return percent;
    }

    private class CommonMode {
        private float brightnessLevel;
        private int screenOffTimeout;
        private boolean isWifiEnabled;
        private int locationState;
        private boolean isBluetoothEnabled;
        private int soundMode;
        private int messageMode;
        private int ringerMode;
        private int isAirplaneMode;
        private boolean isMasterSyncAutomatically;
        private boolean isDataEnabled;
        private boolean isAutoBrightness;
        private int touchEffects;
        private int touchFeedback;
        private boolean isCloseNFC;
        private boolean isClosedWifiHotspot;

        public CommonMode(float brightnessLevel, int screenOffTimeout,
                boolean isWifiEnabled, int locationState,
                boolean isBluetoothEnabled, int soundMode, int messageMode, int ringerMode,
                int isAirplaneMode, boolean isMasterSyncAutomatically,
                boolean isDataEnabled, boolean isAutoBrightness, int touchEffects, int touchFeedback, boolean isClosedWifiHotspot) {
            super();
            this.brightnessLevel = brightnessLevel;
            this.screenOffTimeout = screenOffTimeout;
            this.isWifiEnabled = isWifiEnabled;
            this.locationState = locationState;
            this.isBluetoothEnabled = isBluetoothEnabled;
            this.soundMode = soundMode;
            this.messageMode = messageMode;
            this.ringerMode = ringerMode;
            this.isAirplaneMode = isAirplaneMode;
            this.isMasterSyncAutomatically = isMasterSyncAutomatically;
            this.isDataEnabled = isDataEnabled;
            this.isAutoBrightness = isAutoBrightness;
            this.touchEffects = touchEffects;
            this.touchFeedback = touchFeedback;
            this.isClosedWifiHotspot = isClosedWifiHotspot;
            Log.i(TAG, "brightnessLevel:" + brightnessLevel
                    + "\t\t screenOffTimeout:" + screenOffTimeout
                    + "\t\t isWifiEnabled:" + isWifiEnabled
                    + "\t\t locationState:" + locationState
                    + "\t\t isBluetoothEnabled:" + isBluetoothEnabled
                    + "\t\t soundMode:" + soundMode
                    + "\t\t messageMode:" + messageMode +"\t\t ringerMode:"
                    + ringerMode + "\t\t isAirplaneMode:" + isAirplaneMode
                    + "\t\t isMasterSyncAutomatically:"
                    + isMasterSyncAutomatically + "\t\t isDataEnabled:"
                    + isDataEnabled+"\t\t touchEffects"+touchEffects+"\t\t touchFeedback:"+touchFeedback
                    +"\t\tisClosedWifiHotspot"+isClosedWifiHotspot);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mSoftwareListCategory.removeAll();
        mHardwareListCategory.removeAll();
        addCategoryList();
    }

    private boolean getWifiHotspotInfo(boolean isAirplaneMode) {
        boolean isClosedWifiHotspot = true;
        if(!isAirplaneMode) {
            if (mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLING
                    || mWifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_ENABLED) {
                isClosedWifiHotspot = false;
            }
        }
        return isClosedWifiHotspot;
    }
}
