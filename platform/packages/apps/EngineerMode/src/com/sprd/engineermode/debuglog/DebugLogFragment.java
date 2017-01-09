
package com.sprd.engineermode.debuglog;

import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.content.ComponentName;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.SystemProperties;
import android.app.Fragment;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.preference.TwoStatePreference;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.Switch;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.CheckBox;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.EditText;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextUtils;
import android.util.Log;
import android.os.FileObserver;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.FileObserver;
import android.os.SystemProperties;
import android.app.ProgressDialog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Properties;

import android.os.Environment;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.EMFileUtils;

import android.os.FileUtils;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences.Editor;
import android.app.AlertDialog.Builder;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import android.content.Context;
import com.sprd.engineermode.utils.SocketUtils;
import android.net.LocalSocketAddress;
import android.provider.Settings;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.preference.PreferenceManager;
import android.content.BroadcastReceiver;

import com.sprd.engineermode.debuglog.slogui.SlogAction;
import org.apache.http.entity.mime.*;
import org.apache.http.entity.mime.content.*;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.UserHandle;

public class DebugLogFragment extends PreferenceFragment implements
        Preference.OnPreferenceClickListener, Preference.OnPreferenceChangeListener {
    private static final String TAG = "DebugLogFragment";
    private static final String SYSDUMP = "/proc/sys/kernel/sysdump_enable";
    public static final String APR_UPLOAD_HISTORY_PATH = "/data/sprdinfo/apr_upload_history.txt";
    private static final String APR_SERVER_CONFIG_PATH = "/data/apr.conf";
    private static final String HARDWARE_WATCHDOG = "/sys/module/sprd_wdt_sys/parameters/enabled";
    private static final String ACTION_BOOT_COMPLETED = "android.intent.action.BOOT_COMPLETED";
    private static final String ACTION_REFRESH_UI = "com.sprd.engineermode.debuglog.refresh_ui";
    private static final String KEY_APR_SERVER_NAME = "com.sprd.engineermode.debuglog.APRService";
    private static final String KEY_SYSDUMP = "sysdump";
    private static final String KEY_HPROF = "hprof";
    private static final String KEY_COREFILE = "corefile";
    private static final String KEY_HARDWARE = "hardware_watchdog";
    private static final String KEY_NONSLEEP = "non_sleep";
    private static final String KEY_CMCC = "cmcc";
    private static final String KEY_CUCC = "cucc";
    private static final String KEY_FACTORY_SET = "factory_set";
    private static final String KEY_FLASH_INFO = "sprd_flashblockinfo";
    private static final String KEY_GPS_CONFIG = "gps_config";
    private static final String KEY_AGPS_LOG = "agps_log";
    private static final String KEY_SYSTEM_SETTINGS = "system_settings";
    // Browser debug
    private static final String KEY_BROWSER_LOG_ENABELED = "log_enabled";
    private static final String KEY_SAVE_BROWSER_RECEIVE = "save_receive";
    private static final String KEY_DUMP_BROWSER_TREE = "dump_tree";
    private static final String KEY_PC_LOG = "pclog";
    private static final String KEY_AVS_LOG = "avs_log";
    private static final String KEY_USEER_MODE = "switch_user_mode";
    private static final String KEY_SCREEN_OFF = "switch_screen_off_never";
    public static final String KEY_APR_SYSTEM_SETTING = "persist.sys.apr.enabled";
    public static final String KEY_APR_CUSTOMER_TIME = "persist.sys.apr.intervaltime";
    public static final String KEY_APR_CUSTOMER_GROUP = "persist.sys.apr.testgroup";
    public static final String KEY_APR_AUTO_UPLOAD = "persist.sys.apr.autoupload";
    private static final String KEY_AUTO_DUMP_MODEMASSERT = "auto_dump_modemassert";
    private static final String KEY_VIEW_APR_UPLOADLOG = "check_apr_log";
    private static final String KEY_SYSTEM_UPDATE = "system_update";

    // VersionInfo
    private static final String KEY_HARDWARE_VERSION = "hardware_version";
    private static final String KEY_HARDWARE_VERSION2 = "hardware_version2";
    private static final String KEY_AP_VERSION = "ap_version";
    private static final String KEY_MODEM_VERSION = "modem_version";
    private static final String KEY_PS_VERSION = "ps_version";
    private static final String KEY_DSP_VERSION = "dsp_version";
    private static final String KEY_CP2_VERSION = "cp2_version";
    private static final String KEY_GPS_VERSION = "gps_version";
    private static final String KEY_TP_VERSION = "tp_version";
    private static final String KEY_CMD_TYPE = "cmd_type";
    private static final String KEY_UPLOAD_FILE = "upload_aprfile";
    private static final String KEY_APR_SERVER = "apr_server";
    private static final String KEY_TEST_TOOLS = "others_test_tools";
    private static final String KEY_ADC_TEST="adc_test";
    private static final String KEY_SLEEP_TEST="sleep_test";
    private static final String KEY_PERFORMANCE_TEST="performance_test";
    private static final String KEY_WORK_TEST="work_test";
    private static final String KEY_MTBF_SWITCH = "mtbf_switch";

    private static final String HARDWARE_VERSION_KEYWORD = "sys.hardware.version";
    private static final String HPROFDIR = "/data/misc/hprofs/";
    private static final String COREFILEDIR = "/data/corefile/";
    private final String MTD_PATH = "/proc/mtdbdblk";
    private final String SPRD_MONITOR_CONFIG = "/data/local/tmp/sprd_monitor.conf";
    private final boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");

    private static final int GET_AVS_LOG = 0;
    private static final int OPEN_AVS_LOG = 1;
    private static final int CLOSE_AVS_LOG = 2;
    private static final int GET_USER_MODE = 3;
    private static final int OPEN_USER_MODE = 4;
    private static final int CLOSE_USER_MODE = 5;
    private static final int OPEN_AUTO_UPLOAD_APR = 6;
    private static final int CLOSE_AUTO_UPLOAD_APR = 7;
    private static final int APR_SERVER_SWITCH = 8;
    private static final int APR_UPLOAD_HISTORY = 9;
    private static final int APR_DETECT_TIME_DIALOG = 10;
    private static final int APR_GROUP_DIALOG = 11;
    private static final int APR_SWITCH_SETTING_DIALOG = 12;
    private static final int APR_UPLOAD_HISTORY_DIALOG = 13;
    private static TwoStatePreference mAutoUploadAPR;
    private static Preference mDetectTime;
    private static Preference mGroup;
    private static boolean isValid = true;

    private TwoStatePreference mUserMode;
    private TwoStatePreference mAVSLogSwitch;

    private TwoStatePreference mScreenOff;
    private TwoStatePreference mAutoDumpAssert;
    private Preference mAPRServer;
    private Preference mCMCCSet;
    private Preference mCUCCSet;
    private Preference mFactorySet;
    private Preference mFlashPref;
    private Preference mGpsConfig;
    private Preference mAGpsLog;
    private Preference mSystemSet;
    private Preference mAdcTest;
    private Preference mSleepTest;
    private Preference mPerformanceTest;
    private Preference mWorkTest;
    private Preference mSystemUpdate;
    private TwoStatePreference mBrowserLogEnabled;
    private TwoStatePreference mSaveBrowserReceive;
    private TwoStatePreference mDumpBrowserTree;
    private TwoStatePreference mMtbfSwitch;

    // VersionInfo
    private Preference mHardwareVersion;
    private Preference mHardwareVersion2;
    private Preference mAPVersion;
    private Preference mUploadFile;

    private Preference mUploadHistory;
    private PreferenceScreen mOtherTestTools;
    private String mSysdump;
    private String mHardware_watchdog;
    private WakeLock mWakeLock;
    private Context mContext;
    private DEGHandler mDEGHandler;
    private Handler mUiThread;

    private static final int SCREEN_OFF_NEVER_TIME = 360000000;
    private static final int FALLBACK_SCREEN_TIMEOUT_VALUE = 30000;
    private long mSettingsTimeout;
    public final static String SCREEN_OFF_TIME = "screen_off_time";
    private SharedPreferences mPrefs;
    private DeFileObserver mSprdConfigObserver;
    private ProgressDialog mConfiProgressDiag;
    private ProgressDialog mUploadDialog;
    private Intent mService;
    private EditText mHour;
    private EditText mInputGroup;
    private String[] mAprGroup = new String[] {
            "CSSLAB", "CSFLAB", "CSFT", "Beta"
    };
    private int mGroupIndex;
    private AlertDialog mAlertDialog;
    private static Switch mAprSwitch;
    private ArrayList<CheckBox> mAprViewList;
    private int mAprDialogType;
    private APRReceiver mReceiver;
    private boolean mIsCanHint = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_debuglogtab);
        mContext = getActivity();
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mDEGHandler = new DEGHandler(ht.getLooper());
        mSprdConfigObserver = new DeFileObserver(SPRD_MONITOR_CONFIG);
        mCMCCSet = (Preference) findPreference(KEY_CMCC);
        mCMCCSet.setOnPreferenceClickListener(this);
        mCUCCSet = (Preference) findPreference(KEY_CUCC);
        mCUCCSet.setOnPreferenceClickListener(this);
        mFactorySet = (Preference) findPreference(KEY_FACTORY_SET);
        mFactorySet.setOnPreferenceClickListener(this);
        mGpsConfig = (Preference) findPreference(KEY_GPS_CONFIG);
        mGpsConfig.setOnPreferenceClickListener(this);
        mAGpsLog = (Preference) findPreference(KEY_AGPS_LOG);
        mAGpsLog.setOnPreferenceClickListener(this);
        mUploadHistory = (Preference) findPreference(KEY_VIEW_APR_UPLOADLOG);
        mUploadHistory.setOnPreferenceClickListener(this);
        // LocationGpsConfig & LocationAgpsLogShow in Setting has not ported
        mGpsConfig.setEnabled(false);
        mAGpsLog.setEnabled(false);

        mFlashPref = (Preference) findPreference(KEY_FLASH_INFO);
        mSystemSet = (Preference) findPreference(KEY_SYSTEM_SETTINGS);

        mBrowserLogEnabled = (TwoStatePreference) findPreference(KEY_BROWSER_LOG_ENABELED);
        mBrowserLogEnabled.setOnPreferenceChangeListener(this);
        mSaveBrowserReceive = (TwoStatePreference) findPreference(KEY_SAVE_BROWSER_RECEIVE);
        mSaveBrowserReceive.setOnPreferenceChangeListener(this);
        mDumpBrowserTree = (TwoStatePreference) findPreference(KEY_DUMP_BROWSER_TREE);
        mDumpBrowserTree.setOnPreferenceChangeListener(this);

        mAdcTest=(Preference)findPreference(KEY_ADC_TEST);
        mAdcTest.setOnPreferenceClickListener(this);
        mSleepTest=(Preference)findPreference(KEY_SLEEP_TEST);
        mSleepTest.setOnPreferenceClickListener(this);
        mPerformanceTest=(Preference)findPreference(KEY_PERFORMANCE_TEST);
        mPerformanceTest.setOnPreferenceClickListener(this);
        mWorkTest=(Preference)findPreference(KEY_WORK_TEST);
        mWorkTest.setOnPreferenceClickListener(this);


        mAVSLogSwitch = (TwoStatePreference) findPreference(KEY_AVS_LOG);
        mAVSLogSwitch.setOnPreferenceChangeListener(this);
        mAutoDumpAssert = (TwoStatePreference) findPreference(KEY_AUTO_DUMP_MODEMASSERT);
        mAutoDumpAssert.setOnPreferenceChangeListener(this);
        mMtbfSwitch = (TwoStatePreference) findPreference(KEY_MTBF_SWITCH);
        mMtbfSwitch.setOnPreferenceChangeListener(this);
        if (isUser) {
            mMtbfSwitch.setEnabled(true);
            mMtbfSwitch.setSummary(R.string.mtbf_switch_stop);
        }


        mUserMode = (TwoStatePreference) findPreference(KEY_USEER_MODE);

        mUserMode.setOnPreferenceChangeListener(this);
        mUserMode.setEnabled(false);
        mUserMode.setSummary(R.string.feature_not_support);

        mScreenOff = (TwoStatePreference) findPreference(KEY_SCREEN_OFF);
        mScreenOff.setOnPreferenceChangeListener(this);
        mSystemUpdate = (Preference) findPreference(KEY_SYSTEM_UPDATE);

        mHardwareVersion = (Preference) findPreference(KEY_HARDWARE_VERSION);
        mHardwareVersion2 = (Preference) findPreference(KEY_HARDWARE_VERSION2);
        mAPVersion = (Preference) findPreference(KEY_AP_VERSION);
        mAPVersion.setSummary(Build.DISPLAY);

        Preference psVersion = (Preference) findPreference(KEY_PS_VERSION);
        psVersion.setOnPreferenceClickListener(this);

        Preference dspVersion = (Preference) findPreference(KEY_DSP_VERSION);
        dspVersion.setOnPreferenceClickListener(this);

        Preference cp2Version = (Preference) findPreference(KEY_CP2_VERSION);
        cp2Version.setOnPreferenceClickListener(this);

        Preference gpsVersion = (Preference) findPreference(KEY_GPS_VERSION);
        gpsVersion.setOnPreferenceClickListener(this);

        Preference tpVersion = (Preference) findPreference(KEY_TP_VERSION);
        tpVersion.setOnPreferenceClickListener(this);
        Preference modemVersion = (Preference) findPreference(KEY_MODEM_VERSION);
        modemVersion.setOnPreferenceClickListener(this);

        // Non Spreadtrum chip does not support this feature
        if (SystemProperties.get("ro.modem.wcn.enable", "0").equals("0")) {
            cp2Version.setSummary(R.string.feature_not_support);
            cp2Version.setEnabled(false);
        }

        File file = new File(MTD_PATH);
        if (!file.exists()) {
            mFlashPref.setSummary(R.string.feature_not_support);
            mFlashPref.setEnabled(false);
        }

        mPrefs = PreferenceManager.getDefaultSharedPreferences(getActivity());
        long screenTime = mPrefs.getLong(SCREEN_OFF_TIME, -1);
        mSettingsTimeout = Settings.System.getLong(getActivity().getContentResolver(),
                Settings.System.SCREEN_OFF_TIMEOUT, FALLBACK_SCREEN_TIMEOUT_VALUE);
        if (screenTime == -1 || (screenTime != -1 && mSettingsTimeout != screenTime)) {
            saveScreenTime();
        }
        mUploadFile = (Preference) findPreference(KEY_UPLOAD_FILE);
        mUploadFile.setOnPreferenceClickListener(this);

        mAPRServer = (Preference) findPreference(KEY_APR_SERVER);
        mAPRServer.setOnPreferenceClickListener(this);

        mAutoUploadAPR = (TwoStatePreference) findPreference("auto_upload_apr");
        if (SystemProperties.get(KEY_APR_AUTO_UPLOAD, "0").equals("1")) {
            mAutoUploadAPR.setChecked(true);
        }
        mAutoUploadAPR.setOnPreferenceChangeListener(this);

        mDetectTime = (Preference) findPreference("detect_time");
        String intervalTime = SystemProperties.get(KEY_APR_CUSTOMER_TIME, "0");
        if (!intervalTime.equals("0")) {
            mDetectTime.setSummary(Integer.parseInt(intervalTime) > 1 ? (intervalTime + " hours")
                    : (intervalTime + " hour"));
        }
        mDetectTime.setOnPreferenceClickListener(this);

        mGroup = (Preference) findPreference("group");
        String group = SystemProperties.get(KEY_APR_CUSTOMER_GROUP, "");
        if (!group.equals("")) {
            mGroup.setSummary(group);
        }
        boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
        mOtherTestTools = (PreferenceScreen) findPreference(KEY_TEST_TOOLS);
        if (isUser && mOtherTestTools != null) {
            mOtherTestTools.setEnabled(false);
            mOtherTestTools.setSummary(R.string.feature_not_support);
        }

        mGroup.setOnPreferenceClickListener(this);
        mUiThread = new Handler() {
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case 0:
                        mUploadDialog = ProgressDialog.show(mContext, "UpLoading...",
                                "please wait...");
                        mUploadDialog.setCancelable(false);
                        mUploadDialog.setCanceledOnTouchOutside(false);
                        break;
                    case 1:
                        if (mUploadDialog != null && mUploadDialog.isShowing()) {
                            mUploadDialog.dismiss();
                        }
                        break;
                    case 2:
                        Toast.makeText(mContext, "Upload success!", Toast.LENGTH_SHORT).show();
                        break;
                    case 3:
                        Toast.makeText(mContext, "Upload fail!", Toast.LENGTH_SHORT).show();
                        break;
                }
            }
        };
    }

    public void saveScreenTime() {
        if ((int) mSettingsTimeout == SCREEN_OFF_NEVER_TIME) {
            return;
        }
        SharedPreferences.Editor editor = mPrefs.edit();
        editor.putLong(SCREEN_OFF_TIME, mSettingsTimeout);
        editor.apply();
    }

    @Override
    public void onStart() {
        if(!checkApkExist("com.sprd.systemupdate")) {
            mSystemUpdate.setEnabled(false);
            mSystemUpdate.setSummary(R.string.apk_not_exist);
        }
        if(!checkApkExist("com.hw_v_text")){
            mAdcTest.setEnabled(false);
            mAdcTest.setSummary(R.string.apk_not_exist);
        }
        if(!checkApkExist("com.spreadtrum.sleepactivity")){
            mSleepTest.setEnabled(false);
            mSleepTest.setSummary(R.string.apk_not_exist);
        }
        if(!checkApkExist("com.sprdsrt.performance")){
            mPerformanceTest.setEnabled(false);
            mPerformanceTest.setSummary(R.string.apk_not_exist);
        }
        if(!checkApkExist("com.sprd.workstate")){
            mWorkTest.setEnabled(false);
            mWorkTest.setSummary(R.string.apk_not_exist);
        }

        if (SystemProperties.get("ro.product.board.customer", "none").equalsIgnoreCase("cgmobile")) {
            mHardwareVersion.setSummary(SystemProperties.get("ro.product.hardware", "P1"));
        } else {
            mHardwareVersion.setSummary(SystemProperties.get("ro.product.hardware", "SPREADTRUM"));
        }
        if (mHardwareVersion2 != null) {
            mHardwareVersion2.setSummary(SystemProperties.get(HARDWARE_VERSION_KEYWORD, "UNKNOWN"));
        }
        if (mAVSLogSwitch != null && mAVSLogSwitch.isEnabled()) {
            Message getAvslog = mDEGHandler.obtainMessage(GET_AVS_LOG);
            mDEGHandler.sendMessage(getAvslog);
        }

        if (mUserMode != null && mUserMode.isEnabled()) {
            Message getUserMode = mDEGHandler.obtainMessage(GET_USER_MODE);
            mDEGHandler.sendMessage(getUserMode);
        }

        if (!mScreenOff.isChecked()) {
            mScreenOff.setSummary(R.string.close_screen_off_never);
        } else {
            mScreenOff.setSummary(null);
        }

        if (mAutoDumpAssert != null && mAutoDumpAssert.isEnabled()) {
            mAutoDumpAssert.setChecked(SystemProperties.getBoolean("persist.sys.modemassertdump",
                    false));
        }
        if (!mAutoUploadAPR.isChecked()) {
            mAutoUploadAPR.setSummary(null);
        } else {
            if (!isServiceRunning(mContext, KEY_APR_SERVER_NAME)) {
                Message openAutoUpload = mDEGHandler.obtainMessage(OPEN_AUTO_UPLOAD_APR);
                mDEGHandler.sendMessage(openAutoUpload);
            }
            mAutoUploadAPR.setSummary(null);
        }
        if (mBrowserLogEnabled != null) {
            boolean log_enabled = SystemProperties.getBoolean("persist.sys.br.log",false);
            mBrowserLogEnabled.setChecked(log_enabled);
        }
        if (mSaveBrowserReceive != null) {
            boolean save_receive = SystemProperties.getBoolean("persist.sys.br.save",false);
            mSaveBrowserReceive.setChecked(save_receive);
        }
        if (mDumpBrowserTree != null) {
            boolean dump_tree = SystemProperties.getBoolean("persist.sys.br.dump",false);
            mDumpBrowserTree.setChecked(dump_tree);
        }
        if (mMtbfSwitch != null && mMtbfSwitch.isEnabled()) {
            boolean isOpen = SystemProperties.getBoolean("persist.sys.mtbf.enable", false);
            Log.d(TAG, "mtbf switch current status is: " + isOpen);
            mMtbfSwitch.setChecked(isOpen);
        }
        super.onStart();
    }

    @Override
    public void onResume() {
        /* Modify sprd_res_monitor service whether to restart for CTS */
		long mCurrentTime = Settings.System.getLong(getActivity()
				.getContentResolver(), Settings.System.SCREEN_OFF_TIMEOUT,
				FALLBACK_SCREEN_TIMEOUT_VALUE);
		if ((int) mCurrentTime != SCREEN_OFF_NEVER_TIME) {
			mScreenOff.setChecked(false);
			mScreenOff.setSummary(R.string.close_screen_off_never);
		}

            mSystemSet.setEnabled(true);
            mSystemSet.setSummary("");

        if (SystemProperties.get("init.svc.apr", "stopped").equals("stopped")) {
            mAutoUploadAPR.setChecked(false);
        }
        super.onResume();
    }

    @Override
    public void onDestroy() {
        if (mDEGHandler != null) {
            mDEGHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        closeDialog();
        super.onDestroy();
    }

    /* SPRD:Add for bug 473442,because WindowLeaked. {@ */
    private void closeDialog() {
        if (mUploadDialog != null && mUploadDialog.isShowing()) {
            mUploadDialog.dismiss();
        }
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
    }

    /* @} */

    @Override
    public boolean onPreferenceClick(Preference pref) {
        int mUserId = UserHandle.myUserId();
        if (pref == mCMCCSet) {
            AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                    .setTitle(getString(R.string.alert_cmcc_set))
                    .setMessage(getString(R.string.alert_cmcc_set_message))
                    .setPositiveButton(getString(R.string.alertdialog_ok),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent = new Intent(getActivity(), CMCCActivity.class);
                                    startActivity(intent);
                                }
                            })
                    .setNegativeButton(R.string.alertdialog_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                }
                            }).create();
            alertDialog.show();
        }
        if (pref == mCUCCSet) {
            AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                    .setTitle(getString(R.string.alert_cucc_set))
                    .setMessage(getString(R.string.alert_cucc_set_message))
                    .setPositiveButton(getString(R.string.alertdialog_ok),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent intent = new Intent(getActivity(), CUCCActivity.class);
                                    startActivity(intent);
                                }
                            })
                    .setNegativeButton(R.string.alertdialog_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                }
                            }).create();
            alertDialog.show();
        }

        if(pref == mAdcTest){
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                return false;
            }

            Intent intent = new Intent();
            ComponentName cn = new ComponentName("com.hw_v_text",
                             "com.hw_v_text.TestActivity");
            intent.setComponent(cn);
            startActivity(intent);
        }
        if(pref == mSleepTest){
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                return false;
            }

            Intent intent = new Intent();
            ComponentName cn = new ComponentName("com.spreadtrum.sleepactivity",
                             "com.spreadtrum.sleepactivity.SleepActivity");
            intent.setComponent(cn);
            startActivity(intent);
        }
        if(pref == mPerformanceTest){
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                return false;
            }

            Intent intent = new Intent();
            ComponentName cn = new ComponentName("com.sprdsrt.performance",
                             "com.sprdsrt.performance.MainActivity");
            intent.setComponent(cn);
            startActivity(intent);
        }
        if(pref == mWorkTest){
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                return false;
            }

            Intent intent = new Intent();
            ComponentName cn = new ComponentName("com.sprd.workstate",
                             "com.sprd.workstate.WorkStateActivity");
            intent.setComponent(cn);
            startActivity(intent);
        }
        if (pref == mFactorySet) {
            /* SPRD: bug457894 ,Multi user mode, set button is not click. {@ */
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.multi_user_hint, Toast.LENGTH_SHORT).show();
                return false;
            }
              /* @} */
            Intent intent = new Intent();
            intent.setAction("android.settings.BACKUP_AND_RESET_SETTINGS");
            startActivity(intent);
        } else if (pref == mGpsConfig) {
            Intent intent = new Intent();
            intent.setAction("android.settings.GPS_CONFIG");
            startActivity(intent);
        } else if (pref == mAGpsLog) {
            Intent intent = new Intent();
            intent.setAction("android.settings.AGPS_LOG_SHOW");
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_MODEM_VERSION)) {
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_MODEM_VERSION);
            intent.putExtras(bundle);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_PS_VERSION)) {
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_PS_VERSION);
            intent.putExtras(bundle);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_DSP_VERSION)) {
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_DSP_VERSION);
            intent.putExtras(bundle);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_CP2_VERSION)) {
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_CP2_VERSION);
            intent.putExtras(bundle);
            intent.setFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_GPS_VERSION)) {
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_GPS_VERSION);
            intent.putExtras(bundle);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        } else if (pref.getKey().equals(KEY_TP_VERSION)){
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CMD_TYPE, KEY_TP_VERSION);
            intent.putExtras(bundle);
            intent.setClass(mContext, VersionInfoActivity.class);
            startActivity(intent);
        }else if(pref == mUploadFile) {
            if (isConnected()) {
                uploadFile();
            } else {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.upload_aprfile))
                        .setMessage(getString(R.string.aler_upload_aprfile))
                        .setPositiveButton(getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                    }
                                }).create();
                alertDialog.show();
            }
        } else if (pref == mDetectTime) {
            mHour = new EditText(mContext);
            mHour.setFilters(new InputFilter[]{new InputFilter.LengthFilter(5)});
            mHour.setInputType(InputType.TYPE_CLASS_NUMBER);
            mHour.setHint(getString(R.string.input_detect_time));
            mHour.setPadding(20, 20, 5, 20);

            popAprDialog(mHour, APR_DETECT_TIME_DIALOG);
        } else if (pref == mGroup) {
            mInputGroup = new EditText(mContext);
            mInputGroup.setHint("Input group manually");
            mInputGroup.setPadding(20, 10, 5, 20);

            popAprDialog(mInputGroup, APR_GROUP_DIALOG);
        } else if (pref == mAPRServer) {
            if (mUserId != 0) {
                Toast.makeText(mContext, R.string.not_support_visitor_or_user_mode, Toast.LENGTH_SHORT).show();
                return false;
            }
            if (mAprViewList == null) {
                mAprViewList = new ArrayList<CheckBox>();
            } else {
                mAprViewList.clear();
            }
            View view = LayoutInflater.from(mContext).inflate(R.layout.apr_server, null);
            mAprSwitch = (Switch) view.findViewById(R.id.open_aprserver);

            mAprViewList.add((CheckBox) view.findViewById(R.id.apr_item_anr));
            mAprViewList.add((CheckBox) view.findViewById(R.id.apr_item_nativecrash));
            mAprViewList.add((CheckBox) view.findViewById(R.id.apr_item_javacrash));
            mAprViewList.add((CheckBox) view.findViewById(R.id.apr_item_modemassert));
            mAprViewList.add((CheckBox) view.findViewById(R.id.apr_item_crashclass));
            mAprSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

                    if (isChecked && isValid) {
                        int index = 0;
                        for (int i = 0; i < mAprViewList.size(); i++) {
                            if (mAprViewList.get(i).isChecked()) {
                                index++;
                            }
                        }
                        if (index > 0) {
                            setAprDataToFile();
                            if (SystemProperties.get("init.svc.apr", "stopped").equals("stopped")) {
                                SystemProperties.set(KEY_APR_SYSTEM_SETTING, "1");
                            }
                            setAllViewClickable(false);
                        } else {
                            mAprSwitch.setChecked(false);
                            Toast.makeText(mContext, R.string.apr_save_error, Toast.LENGTH_SHORT)
                                    .show();
                            return;
                        }

                        if (SystemProperties.get("init.svc.apr", "stopped").equals("stopped")) {
                            SystemProperties.set(KEY_APR_SYSTEM_SETTING, "1");
                        }

                    } else if (!isChecked && isValid) {
                        SystemProperties.set(KEY_APR_SYSTEM_SETTING, "0");
                        if (SystemProperties.get("init.svc.apr", "stopped").equals("running")) {
                            SystemProperties.set(KEY_APR_SYSTEM_SETTING, "0");
                        }
                        setAllViewClickable(true);
                    }

                    isValid = true;
                    if (mAlertDialog != null && mAlertDialog.isShowing()) {
                        if (SystemProperties.get("init.svc.apr", "stopped").equals("running")) {
                            Toast.makeText(mContext, R.string.success_number, Toast.LENGTH_SHORT)
                                    .show();
                        }
                        mAlertDialog.dismiss();
                    }

                }
            });

            getAprDataFromFile(APR_SERVER_CONFIG_PATH, APR_SERVER_SWITCH);
            if (SystemProperties.get("init.svc.apr", "unknown").equals("running")) {
                isValid = false;
                mAprSwitch.setChecked(true);
                setAllViewClickable(false);
            } else {
                mAprSwitch.setChecked(false);
            }

            popAprDialog(view, APR_SWITCH_SETTING_DIALOG);
        } else if (pref == mUploadHistory) {

            View view = LayoutInflater.from(mContext).inflate(R.layout.apr_upload_history, null);
            TextView historyView = (TextView) view.findViewById(R.id.apr_history);
            String data = getAprDataFromFile(APR_UPLOAD_HISTORY_PATH, APR_UPLOAD_HISTORY);
            if (TextUtils.isEmpty(data)) {
                historyView.setText(R.string.empty_view_hint);
                historyView.setGravity(Gravity.CENTER);
            } else {
                historyView.setGravity(Gravity.LEFT);
                historyView.setText(data);
            }

            popAprDialog(view, APR_UPLOAD_HISTORY_DIALOG);
        }
        return true;
    }

    private void popAprDialog(View view, int dialogType) {

        mAprDialogType = dialogType;
        AlertDialog.Builder alertBuilder = new AlertDialog.Builder(mContext)
                .setView(view)
                .setOnKeyListener(new DialogInterface.OnKeyListener() {
                    @Override
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if ((keyCode == KeyEvent.KEYCODE_BACK)
                                && (event.getAction() == KeyEvent.ACTION_UP)) {
                            destroyDialog(dialog);
                        }
                        return false;
                    }
                })
                .setNegativeButton(getString(R.string.alertdialog_cancel),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                destroyDialog(dialog);
                            }
                        });

        if (mAprDialogType != APR_SWITCH_SETTING_DIALOG) {

            if (mAprDialogType == APR_DETECT_TIME_DIALOG) {
                alertBuilder.setTitle(getString(R.string.detect_time));
            } else if (mAprDialogType == APR_GROUP_DIALOG) {
                mGroupIndex = getSingleChoiceIndex(SystemProperties.get(KEY_APR_CUSTOMER_GROUP, ""));
                alertBuilder.setTitle(getString(R.string.group)).setSingleChoiceItems(mAprGroup,
                        mGroupIndex, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if (which >= 0) {
                                    mGroupIndex = which;
                                    mGroup.setSummary(mAprGroup[mGroupIndex]);
                                    SystemProperties.set(KEY_APR_CUSTOMER_GROUP,
                                            mAprGroup[mGroupIndex]);
                                }
                                dialog.cancel();
                            }
                        });
            }

            alertBuilder.setPositiveButton(
                    (mAprDialogType != APR_UPLOAD_HISTORY_DIALOG) ? R.string.button_ok
                            : R.string.clear_apr_log, new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            if (mAprDialogType == APR_UPLOAD_HISTORY_DIALOG) {
                                File file = new File(APR_UPLOAD_HISTORY_PATH);
                                if (file.exists()) {
                                    file.delete();
                                }
                                SharedPreferences prefs = mContext.getSharedPreferences(APRService.SHARED_PREFS_APR_FILE,
                                        Context.MODE_PRIVATE);
                                SharedPreferences.Editor editor = prefs.edit();
                                editor.putLong("apr_history_count", 0);
                                editor.apply();
                            } else if (mAprDialogType == APR_DETECT_TIME_DIALOG) {
                                if (TextUtils.isEmpty(mHour.getText().toString().trim())) {
                                    Toast.makeText(mContext, "Please input a number!",
                                            Toast.LENGTH_SHORT).show();
                                    keepDialog(dialog);
                                    return;
                                } else if (!TextUtils.isEmpty(mHour.getText())) {
                                    String intervalTime = mHour.getText().toString();
                                    if (Long.parseLong(intervalTime) > 48) {
                                        Toast.makeText(mContext,
                                                getString(R.string.apr_detect_time_hint),
                                                Toast.LENGTH_SHORT).show();
                                        keepDialog(dialog);
                                        return;
                                    }
                                    mDetectTime.setSummary(Integer.parseInt(intervalTime) > 1 ? (intervalTime + " hours")
                                            : (intervalTime + " hour"));
                                    SystemProperties.set(KEY_APR_CUSTOMER_TIME, intervalTime);
                                }
                            } else if (mAprDialogType == APR_GROUP_DIALOG) {
                                if (!TextUtils.isEmpty(mInputGroup.getText())) {
                                    String group = mInputGroup.getText().toString();
                                    if (group.length() > 30) {
                                        Toast.makeText(mContext,
                                                getString(R.string.apr_group_hint),
                                                Toast.LENGTH_SHORT).show();
                                        keepDialog(dialog);
                                        return;
                                    }
                                    mGroup.setSummary(mInputGroup.getText().toString());
                                    SystemProperties.set(KEY_APR_CUSTOMER_GROUP, mInputGroup
                                            .getText().toString());
                                }
                            }
                            destroyDialog(dialog);

                        }
                    });
        }
        mAlertDialog = alertBuilder.create();
        mAlertDialog.setCanceledOnTouchOutside(false);
        mAlertDialog.show();
    }

    private String getAprDataFromFile(String path, int dataType) {
        FileInputStream fis = null;
        ByteArrayOutputStream bos = null;
        try {
            File file = new File(path);
            fis = new FileInputStream(file);
            byte[] buffer = new byte[1024];
            int len = 0;
            bos = new ByteArrayOutputStream();
            while ((len = fis.read(buffer)) != -1) {
                bos.write(buffer, 0, len);
            }
            byte[] dataByte = bos.toByteArray();
            String data = new String(dataByte, 0, dataByte.length);
            if (dataType == APR_UPLOAD_HISTORY) {
                return data;
            }

            String array[] = data.split("=");
            if (array.length == 6) {
                for (int i = 0; i < 5; i++) {
                    mAprViewList.get(i).setChecked(array[i + 1].contains("yes"));
                }
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (bos != null) {
                try {
                    bos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return null;
    }

    private void setAprDataToFile() {
        if (mAprViewList == null || mAprViewList.size() != 5) {
            return;
        }

        FileOutputStream fos = null;
        try {
            File file = new File(APR_SERVER_CONFIG_PATH);
            if (!file.exists()) {
                file.createNewFile();
            }
            fos = new FileOutputStream(file);

            StringBuffer buffer = new StringBuffer();
            buffer.append("[exceptions]\nanr=" + (mAprViewList.get(0).isChecked() ? "yes" : "no"));
            buffer.append("\nnativeCrash=" + (mAprViewList.get(1).isChecked() ? "yes" : "no"));
            buffer.append("\njavaCrash=" + (mAprViewList.get(2).isChecked() ? "yes" : "no"));
            buffer.append("\nmodemAssert=" + (mAprViewList.get(3).isChecked() ? "yes" : "no"));
            buffer.append("\ncrashClass=" + (mAprViewList.get(4).isChecked() ? "yes" : "no"));

            fos.write(buffer.toString().getBytes());
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fos != null) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void setAllViewClickable(boolean clickable) {
        if (mAprViewList != null) {
            for (int i = 0; i < mAprViewList.size(); i++) {
                mAprViewList.get(i).setClickable(clickable);
                if (clickable) {
                    mAprViewList.get(i).setTextColor(Color.BLACK);
                } else {
                    mAprViewList.get(i).setTextColor(Color.GRAY);
                }
            }
        }
    }

    private static void keepDialog(DialogInterface dialog) {
        try {
            if (dialog != null) {
                Field field = dialog.getClass().getSuperclass().getDeclaredField("mShowing");
                if (field != null) {
                    field.setAccessible(true);
                    field.set(dialog, false);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void destroyDialog(DialogInterface dialog) {
        try {
            if (dialog != null) {
                Field field = dialog.getClass().getSuperclass().getDeclaredField("mShowing");
                if (field != null) {
                    field.setAccessible(true);
                    field.set(dialog, true);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // TODO Auto-generated method stub
    /*
     * NumberPickerDialog.OnNumberSetListener mNumberPickerListener = new
     * NumberPickerDialog.OnNumberSetListener() { public void onNumberSet(int
     * number) { Intent service = new Intent(APRService.ACTION_START, null,
     * mContext, APRService.class); service.putExtra(APRService.KEY_INTERNAL,
     * number); mContext.startService(service); } };
     */

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        if (pref == mAVSLogSwitch) {
            if (!mAVSLogSwitch.isChecked()) {
                Message openAvslog = mDEGHandler.obtainMessage(OPEN_AVS_LOG);
                mDEGHandler.sendMessage(openAvslog);
            } else {
                Message closeAvslog = mDEGHandler.obtainMessage(CLOSE_AVS_LOG);
                mDEGHandler.sendMessage(closeAvslog);
            }
        } else if (pref == mUserMode) {
            if (!mUserMode.isChecked()) {
                Message openUserMode = mDEGHandler.obtainMessage(OPEN_USER_MODE);
                mDEGHandler.sendMessage(openUserMode);
            } else {
                Message closeUserMode = mDEGHandler.obtainMessage(CLOSE_USER_MODE);
                mDEGHandler.sendMessage(closeUserMode);
            }
        } else if (pref == mAutoDumpAssert) {
            SystemProperties.set("persist.sys.modemassertdump",
                    mAutoDumpAssert.isChecked() ? "false" : "true");
            Log.d(TAG,
                    "auto dump assert is "
                            + SystemProperties.getBoolean("persist.sys.modemassertdump", false));
        } else if (pref == mScreenOff) {
            if (!mScreenOff.isChecked()) {
                mSettingsTimeout = Settings.System.getLong(getActivity().getContentResolver(),
                        Settings.System.SCREEN_OFF_TIMEOUT, FALLBACK_SCREEN_TIMEOUT_VALUE);

                saveScreenTime();
                Settings.System.putInt(getActivity().getContentResolver(),
                        Settings.System.SCREEN_OFF_TIMEOUT, SCREEN_OFF_NEVER_TIME);
                mScreenOff.setSummary(null);
            } else {
                Settings.System.putInt(getActivity().getContentResolver(),
                        Settings.System.SCREEN_OFF_TIMEOUT, (int) mSettingsTimeout);
                mScreenOff.setSummary(R.string.close_screen_off_never);
            }

        } else if (pref == mAutoUploadAPR) {
            if (!mAutoUploadAPR.isChecked()) {
                mIsCanHint = true;
                Message openAutoUpload = mDEGHandler.obtainMessage(OPEN_AUTO_UPLOAD_APR);
                mDEGHandler.sendMessage(openAutoUpload);
                mAutoUploadAPR.setSummary(null);
            } else {
                mIsCanHint = false;
                Message closeAutoUpload = mDEGHandler.obtainMessage(CLOSE_AUTO_UPLOAD_APR);
                mDEGHandler.sendMessage(closeAutoUpload);
                mAutoUploadAPR.setSummary(null);
            }
        } else if (pref == mBrowserLogEnabled) {
            if (!mBrowserLogEnabled.isChecked()) {
                SystemProperties.set("persist.sys.br.log", "true");
            } else {
                SystemProperties.set("persist.sys.br.log", "false");
            }
            return true;
        } else if (pref == mSaveBrowserReceive) {
            if (!mSaveBrowserReceive.isChecked()) {
                SystemProperties.set("persist.sys.br.save", "true");
            } else {
                SystemProperties.set("persist.sys.br.save", "false");
            }
            return true;
        } else if (pref == mDumpBrowserTree) {
            if (!mDumpBrowserTree.isChecked()) {
                SystemProperties.set("persist.sys.br.dump", "true");
            } else {
                SystemProperties.set("persist.sys.br.dump", "false");
            }
            return true;
        } else if (pref == mMtbfSwitch) {
            AlertDialog alertDialog = new AlertDialog.Builder(mContext)
            .setTitle(getString(R.string.mtbf_switch))
            .setMessage(
                    getString(R.string.wiq_switch_meaasge))
            .setPositiveButton(getString(R.string.alertdialog_ok),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,
                                int which) {
                           if (SystemProperties.getBoolean("persist.sys.mtbf.enable", false)) {
                               SystemProperties.set("persist.sys.mtbf.enable", "0");
                               mMtbfSwitch.setChecked(false);
                           } else {
                               SystemProperties.set("persist.sys.mtbf.enable", "1");
                               mMtbfSwitch.setChecked(true);
                           }
                           PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
                           pm.reboot("mtbf");
                        }
                    })
            .setNegativeButton(R.string.alertdialog_cancel,
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog,
                                int which) {
                            mMtbfSwitch.setChecked(SystemProperties.getBoolean("persist.sys.mtbf.enable", false));
                        }
                    }).create();
    alertDialog.show();
        }
        return true;
    }

    class DEGHandler extends Handler {

        public DEGHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case GET_AVS_LOG:
                    // /sys/power/avs_log does not exist on 5.0
                    String avsStatus = readFile("/sys/power/avs_log");
                    Log.d(TAG, "AVS Log status is " + avsStatus);
                    if (!"readError".equals(avsStatus) && avsStatus != null) {
                        if (avsStatus.contains("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mAVSLogSwitch.setChecked(true);
                                }
                            });
                        } else if (avsStatus.contains("0")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mAVSLogSwitch.setChecked(false);
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mAVSLogSwitch.setEnabled(false);
                                    mAVSLogSwitch.setSummary(R.string.feature_abnormal);
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAVSLogSwitch.setEnabled(false);
                                mAVSLogSwitch.setSummary(R.string.feature_abnormal);
                            }
                        });
                    }
                    break;
                case OPEN_AVS_LOG:
                    String res = execShellStr("echo 1 > /sys/power/avs_log");
                    String curstatus = readFile("/sys/power/avs_log");
                    Log.d(TAG, "openavslog result is " + res + ", curstatus is " + curstatus);
                    if (curstatus.contains("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAVSLogSwitch.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAVSLogSwitch.setChecked(false);
                            }
                        });
                    }

                    break;
                case CLOSE_AVS_LOG:
                    String clores = execShellStr("echo 0 > /sys/power/avs_log");
                    String curstatus1 = readFile("/sys/power/avs_log");
                    Log.d(TAG, "closeavslog result is " + clores + ", curstatus is " + curstatus1);
                    if (curstatus1.contains("0")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAVSLogSwitch.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAVSLogSwitch.setChecked(true);
                            }
                        });
                    }
                    break;
                case GET_USER_MODE:
                    res = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn at+cp2_enter_user?");
                    Log.d(TAG, "UserMode status is " + res);
                    if (res != null
                            && (res.contains("OK") || res.contains("Ok") || res.contains("oK") || res
                                    .contains("ok"))) {
                        if (res.contains("0")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mUserMode.setChecked(false);
                                }
                            });
                        } else if (res.contains("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mUserMode.setChecked(true);
                                }
                            });
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mUserMode.setEnabled(false);
                                mUserMode.setSummary(R.string.feature_abnormal);
                            }
                        });
                    }
                    break;
                case OPEN_USER_MODE:
                    res = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn at+cp2_enter_user=1");
                    Log.d(TAG, "open UserMode Res is " + res);
                    if (res != null
                            && (res.contains("OK") || res.contains("Ok") || res.contains("oK") || res
                                    .contains("ok"))) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mUserMode.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mUserMode.setChecked(false);
                            }
                        });
                    }
                    break;
                case CLOSE_USER_MODE:
                    res = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn at+cp2_enter_user=0");
                    Log.d(TAG, "close UserMode Res is " + res);
                    if (res != null
                            && (res.contains("OK") || res.contains("Ok") || res.contains("oK") || res
                                    .contains("ok"))) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mUserMode.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mUserMode.setChecked(true);
                            }
                        });
                    }
                    break;
                case OPEN_AUTO_UPLOAD_APR:
                    if (!isConnected()
                            || SystemProperties.get("init.svc.apr", "stopped").equals("stopped")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAutoUploadAPR.setChecked(false);
                                // Add for bug486943 ,limit toast hint.
                                if (mIsCanHint) {
                                    Toast.makeText(
                                            mContext,
                                            (!SystemProperties.get("init.svc.apr", "stopped")
                                                    .equals("stopped")) ? "No network ..."
                                                    : getString(R.string.apr_switch_close_hint),
                                            Toast.LENGTH_SHORT).show();
                                }
                            }
                        });
                    } else {
                        mService = new Intent(APRService.ACTION_START, null, mContext,
                                APRService.class);
                        mContext.startService(mService);
                        SystemProperties.set(KEY_APR_AUTO_UPLOAD, "1");
                    }
                    break;
                case CLOSE_AUTO_UPLOAD_APR:
                    if (mService == null) {
                        mService = new Intent(APRService.ACTION_START, null, mContext,
                                APRService.class);
                    }
                    mContext.stopService(mService);
                    SystemProperties.set(KEY_APR_AUTO_UPLOAD, "0");
                    break;
                default:
                    break;
            }
        }
    }

    private boolean isConnected() {
        boolean isConnected = false;
        Activity act = this.getActivity();
        if ( act == null ) {
            return isConnected;
        }
        ConnectivityManager connectManager = (ConnectivityManager) act.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netinfo = connectManager.getActiveNetworkInfo();
        if (netinfo != null) {
            if (netinfo.getType() == ConnectivityManager.TYPE_MOBILE
                    || netinfo.getType() == ConnectivityManager.TYPE_WIFI) {
                isConnected = true;
            }
        }
        return isConnected;
    }

    private void uploadFile() {
        new Thread() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                super.run();
                mUiThread.sendEmptyMessage(0);
                String softwore = android.os.Build.VERSION.INCREMENTAL;
                String phoneName = android.os.Build.DEVICE;
                try {
                    MultipartEntity mEntity = new MultipartEntity();
                    String phoneModel = SystemProperties.get("ro.product.model");
                    StringBody sbody = new StringBody(phoneModel);
                    StringBody sbody1 = new StringBody(phoneName);
                    StringBody sbody2 = new StringBody(softwore);
                    String phoneSN = SystemProperties.get("ro.serialno");
                    StringBody sbody3 = new StringBody(phoneSN);
                    String testGroup = SystemProperties.get("persist.sys.apr.testgroup", "CSSLAB");
                    StringBody sbody4 = new StringBody(testGroup);
                    String projectInfo = SystemProperties.get("ro.product.name")
                            +"_"+SystemProperties.get("ro.build.type")
                            +"_"+SystemProperties.get("ro.build.version.release");
                    StringBody sbody5 = new StringBody(projectInfo);
                    mEntity.addPart("phoneModel", sbody);
                    mEntity.addPart("phoneName", sbody1);
                    mEntity.addPart("phoneSoftwore", sbody2);
                    mEntity.addPart("phoneSN", sbody3);
                    mEntity.addPart("group", sbody4);
                    mEntity.addPart("projectInfo", sbody5);
                    File aprFile = new File("/data/sprdinfo/apr.xml");
                    if (aprFile.exists()) {
                        FileBody file = new FileBody(aprFile);
                        mEntity.addPart("apr", file);
                        HttpPost post = new HttpPost(
                                "http://222.66.158.137:8080/sendfile1/sendfile.do");
                        post.setEntity(mEntity);
                        DefaultHttpClient dhc = new DefaultHttpClient();
                        HttpResponse response = dhc.execute(post);
                        int status = response.getStatusLine().getStatusCode();
                        if (status == 200) {
                            APRService.saveStringToFile(mContext, "success");
                            Log.e(TAG, "response success! , upload data phoneModel: " + phoneModel
                                    + ",phoneName:" + phoneName + ",phoneSN:" + phoneSN + ",testGroup:"
                                    + testGroup);
                            mUiThread.sendEmptyMessage(2);
                        } else {
                            APRService.saveStringToFile(mContext, "upload failed!!");
                            Log.e(TAG, "response " + status);
                            mUiThread.sendEmptyMessage(3);
                        }
                    } else {
                        APRService.saveStringToFile(mContext, "data apr.xml file not exists!");
                        Log.e(TAG, "data apr.xml file not exists!");
                    }
                } catch (ClientProtocolException e) {
                    // TODO Auto-generated catch block
                    mUiThread.sendEmptyMessage(3);
                    APRService.saveStringToFile(mContext, "upload failed!!");
                    e.printStackTrace();
                } catch (IOException e) {
                    mUiThread.sendEmptyMessage(3);
                    APRService.saveStringToFile(mContext, "upload failed!!");
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                mUiThread.sendEmptyMessage(1);
            }
        }.start();
    }

    public String readFile(String path) {
        File file = new File(path);
        String str = new String("");
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                str = str + line;
            }
        } catch (Exception e) {
            Log.d(TAG, "Read file error!!!");
            str = "readError";
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
        }
        Log.d(TAG, "read " + path + " value is " + str.trim());
        return str.trim();
    }

    public String execShellStr(String cmd) {
        String[] cmdStrings = new String[] {
                "sh", "-c", cmd
        };
        StringBuffer retString = new StringBuffer("");

        try {
            Process process = Runtime.getRuntime().exec(cmdStrings);
            BufferedReader stdout = new BufferedReader(new InputStreamReader(
                    process.getInputStream(), "UTF-8"), 7777);
            BufferedReader stderr = new BufferedReader(new InputStreamReader(
                    process.getErrorStream(), "UTF-8"), 7777);

            String line = null;

            while ((null != (line = stdout.readLine())) || (null != (line = stderr.readLine()))) {
                if ("" != line) {
                    retString = retString.append(line).append("\n");
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, cmd + ":" + retString.toString() + "");
        return retString.toString();
    }

    class DeFileObserver extends FileObserver {
        public DeFileObserver(String path) {
            super(path);
        }

        @Override
        public void onEvent(int event, String path) {
            Log.v(TAG, "observer " + path + " event = 0x" + Integer.toHexString(event));
            if (event == FileObserver.CREATE) {
                stopWatching();
                if (mConfiProgressDiag != null) {
                    mConfiProgressDiag.dismiss();
                    Intent intent = new Intent(getActivity(), SystemSettingActivity.class);
                    startActivity(intent);
                }
                return;
            }
        }
    }

    public static boolean isServiceRunning(Context mContext, String className) {
        boolean isRunning = false;
        ActivityManager activityManager = (ActivityManager) mContext
                .getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningServiceInfo> serviceList = activityManager
                .getRunningServices(Integer.MAX_VALUE);

        if (!(serviceList.size() > 0)) {
            return false;
        }

        for (int i = 0; i < serviceList.size(); i++) {
            if (serviceList.get(i).service.getClassName().equals(className)) {
                isRunning = true;
                break;
            }
        }

        return isRunning;
    }

    private int getSingleChoiceIndex(String itemName){

        if(itemName.equals("CSSLAB")){
            return 0;
        }else if(itemName.equals("CSFLAB")){
            return 1;
        }else if(itemName.equals("CSFT")){
            return 2;
        }else if(itemName.equals("Beta")){
            return 3;
        }else{
            return 0;
        }

    }

    public static class APRReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
			if (action == null)
				return;
            Log.d(TAG, "APRReceiver,action:" + action);
           /*if (action.equals(APRService.ACTION_APR_SERVER_START)) {
                if (mAutoUploadAPR != null && !mAutoUploadAPR.isChecked()) {
                    mAutoUploadAPR.setChecked(true);
                }
            } else if (action.equals(APRService.ACTION_APR_SERVER_END)) {
                if (mAutoUploadAPR != null && mAutoUploadAPR.isChecked()) {
                    mAutoUploadAPR.setChecked(false);
                }
            } else */if (action.equals(ACTION_BOOT_COMPLETED)) {
                if (SystemProperties.get(KEY_APR_AUTO_UPLOAD, "0").equals("1")
                        && SystemProperties.get("init.svc.apr", "stopped").equals("running")) {
                    Intent mService = new Intent(APRService.ACTION_START, null, context,
                            APRService.class);
                    context.startService(mService);
                }
            } else if (action.equals(APRService.ACTION_APR_SERVICE_RESTART)) {
                APRService.ServiceHandler mServiceHandler = APRService.getServiceHandler();
                if (mServiceHandler != null) {
                    mServiceHandler.sendEmptyMessage(APRService.EVENT_REQUEST_UPLOAD);
                }
            } else if (action.equals(ACTION_REFRESH_UI)) {
                if (mAprSwitch != null) {
                    isValid = false;
                    if (SystemProperties.get("init.svc.apr", "stopped").equals("stopped")) {
                        mAprSwitch.setChecked(false);
                    } else {
                        mAprSwitch.setChecked(true);
                    }
                }
                if (mDetectTime != null && mGroup != null) {
                    String intervalTime = SystemProperties.get(KEY_APR_CUSTOMER_TIME, "0");
                    mDetectTime
                            .setSummary(Integer.parseInt(intervalTime) > 1 ? (intervalTime + " hours")
                                    : (intervalTime + " hour"));
                    mGroup.setSummary(SystemProperties.get(KEY_APR_CUSTOMER_GROUP, ""));
                }
            }
        }

    }

    public boolean checkApkExist(String packageName) {
        if (packageName == null || "".equals(packageName))
           return false;
        try {
           ApplicationInfo info = DebugLogFragment.this.getActivity().getPackageManager()
             .getApplicationInfo(packageName,
           PackageManager.GET_UNINSTALLED_PACKAGES);
           return true;
        } catch (Exception e) {
           return false;
        }
    }

}
