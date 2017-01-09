
package com.sprd.engineermode.debuglog;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.HashMap;
import java.util.Map.Entry;
import java.util.Iterator;

import com.sprd.engineermode.R;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.content.pm.ApplicationInfo;
import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;
import android.preference.Preference.OnPreferenceClickListener;

@SuppressLint("HandlerLeak")
public class MTBFActivity extends PreferenceActivity implements OnClickListener {
    private static final String TAG = "MTBFActivity";
    static final String NAMELIST = "namelist";
    static final String EMAIL_SIMTYPE = "emailsimtype";
    static final int EMAIL_SIMTYPE_CMCC = 0;
    static final int EMAIL_SIMTYPE_CUCC = 1;
    static final String EMAIL_ADDRS = "emailaddrs";
    static final String EMAIL_PSW = "emailpsw";
    static final String CMCC_ADDRESS_SUFFIX = "@139.com";
    static final String CUCC_ADDRESS_SUFFIX = "@163.com";
    static final String CMCC_DEFAULT_PSW = "Comcat123";
    static final String CUCC_DEFAULT_PSW = "Comcat123";
    private static final int mShortcutSelReqCode = 0;
    private static final int mEmailInfoReqCode = 1;
    private static final int STOP_AND_SHOW_RESULT = 1;
    private static final int TIMEROUT_AND_RETRY = 2;
    private static final int TIMEOUT_MS = 30000;
    private static final int RETRY_MS = 3000;
    private static final String RESULT_NA = "na";
    private static final String RESULT_TIMEOUT = "Timeout";
    private static final String RESULT_OK = "Ok";
    private static final String RESULT_FAIL = "Fail";
    private Button mbtnStartMTBF = null;
    private Button mbtnFactorySet = null;
    private MTBFRSPReceiver mReceiver = new MTBFRSPReceiver();
    private ArrayList<String> mPackageNameArray = new ArrayList<String>();
    private int mEmailSimType = 0;
    private String mEmailAddrs = null;
    private String mEmailPsw = null;
    private ProgressDialog mProgressDialog;
    private Handler mUiHandler = null;
    private HashMap<String, String> mResult = new HashMap<String, String>();
    private boolean mIsRetry = false;
    private AlertDialog mAlertDialog = null;

    class MTBFRSPReceiver extends BroadcastReceiver {
        private static final int LAUNCHER_SHORTCURSET = 0;

        private static final int SETTING_CONNSET = 0;
        private static final int SETTING_TIMESET = 1;
        private static final int SETTING_USBSET = 2;
        private static final int SETTING_IMSET = 3;
        private static final int SETTING_SCREENSET = 4;

        private static final int MMS_NOTIFYSET = 0;

        private static final int CALENDAR_NOTIFYSET = 0;

        private static final int BROSWER_SET = 0;
        private static final int BROSWER_BOOKMARKRESET = 1;

        private static final int EMAIL_ADDRSET = 0;

        private static final int EM_LOGRESET = 0;

        private static final String KEY_PACKAGE_NAME = "PACKAGE NAME";
        private static final String KEY_SETITEM = "SETITEM";
        private static final String KEY_RESULT = "RESULT";

        private static final String LAUNCHER_PACKAGE_NAME = "com.android.launcher";
        private static final String LAUNCHER_PACKAGE_NAME1 = "com.android.sprdlauncher2";
        private static final String LAUNCHER_PACKAGE_NAME2 = "com.android.launcher3";
        private static final String LAUNCHER_PACKAGE_NAME3 = "com.android.sprdlauncher1";
       
        private static final String SETTINGS_PACKAGE_NAME = "com.android.settings";
        private static final String MMS_PACKAGE_NAME = "com.android.mms";
        private static final String CALENDAR_PACKAGE_NAME = "com.android.calendar";
        private static final String BROSWER_PACKAGE_NAME = "com.android.browser";
        private static final String EMAIL_PACKAGE_NAME = "com.android.email";
        private static final String EM_PACKAGE_NAME = "com.sprd.engineermode";

        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle extras = intent.getExtras();
            String packageName = (String) extras.get(KEY_PACKAGE_NAME);
            Integer setItem = (Integer) extras.get(KEY_SETITEM);
            String result = (String) extras.get(KEY_RESULT);
            Preference p = null;
            String[] launcherPreKeys = {
                    "launcher_sc_set"
            };
            String[] settingsPreKeys = {
                    "setting_conn_set", "setting_time_set", "setting_usb_set", "setting_im_set",
                    "setting_screen_set"
            };
            String[] mmsPreKeys = {
                    "setting_mms_set"
            };
            String[] calendarPreKeys = {
                    "setting_calendar_set"
            };
            String[] broswerPreKeys = {
                    "setting_broswer_set", "setting_bookmark_set"
            };
            String[] emailPreKeys = {
                    "setting_email_addr_set"
            };
            String[] emPreKeys = {
                    "setting_em_logs_set"
            };

            Log.d(TAG,"receive a rsp from:" + packageName + "result=" + result);
            if (0 == packageName.compareTo(LAUNCHER_PACKAGE_NAME)
                    || 0 == packageName.compareTo(LAUNCHER_PACKAGE_NAME1)
                    || 0 == packageName.compareTo(LAUNCHER_PACKAGE_NAME2)
                    || 0 == packageName.compareTo(LAUNCHER_PACKAGE_NAME3)){
                p = findPreference(launcherPreKeys[setItem]);
            } else if (0 == packageName.compareTo(SETTINGS_PACKAGE_NAME)) {
                p = findPreference(settingsPreKeys[setItem]);
            } else if (0 == packageName.compareTo(MMS_PACKAGE_NAME)) {
                p = findPreference(mmsPreKeys[setItem]);
            } else if (0 == packageName.compareTo(CALENDAR_PACKAGE_NAME)) {
                p = findPreference(calendarPreKeys[setItem]);
            } else if (0 == packageName.compareTo(BROSWER_PACKAGE_NAME)) {
                p = findPreference(broswerPreKeys[setItem]);
            } else if (0 == packageName.compareTo(EMAIL_PACKAGE_NAME)) {
                p = findPreference(emailPreKeys[setItem]);
            } else if (0 == packageName.compareTo(EM_PACKAGE_NAME)) {
                p = findPreference(emPreKeys[setItem]);
            }

            if (p != null) {
                mResult.put(p.getKey(), result);
                // Log.d(TAG, p.getTitle() + " Return " + result);

                if (isAllRsped()) {
                    mUiHandler.sendMessage(mUiHandler.obtainMessage(STOP_AND_SHOW_RESULT));
                }
            }
        }

    }

    class MTBFHandler extends Handler {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case STOP_AND_SHOW_RESULT:
                    try {
                        unregisterReceiver(mReceiver);
                    } catch (IllegalArgumentException e) {
                        if (e.getMessage().contains("Receiver not registered")) {
                            return;
                        } else {
                            throw e;
                        }
                    }

                    // Refrash UI
                    showResult();
                    mProgressDialog.dismiss();
                    mbtnStartMTBF.setEnabled(true);
                    break;
                case TIMEROUT_AND_RETRY:
                    mAlertDialog.cancel();
                    startMTBF(true);
                    break;
                default:
                    break;
            }
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_mtbf);
        setContentView(R.layout.mtbf_main);

        mUiHandler = new MTBFHandler();

        // Init APK list for default selected
        initDefaultApkList();
        if (initDefaultEmailAddr()) {
            Toast.makeText(this, "Email Address init success", Toast.LENGTH_SHORT).show();
            Preference emailPre = findPreference("setting_email_addr_set");
            emailPre.setSummary(mEmailAddrs);
        } else {
            Toast.makeText(this, "Cannot read the ownnumber,you can set the Email address manual!", Toast.LENGTH_SHORT).show();
        }

        // Launcher shortcut Click
        Preference launcherPre = findPreference("launcher_sc_set");
        if(launcherPre != null){
	        launcherPre.setOnPreferenceClickListener(new OnPreferenceClickListener() {
	            @Override
	            public boolean onPreferenceClick(Preference arg0) {
	                Intent intent = new Intent(MTBFActivity.this, MTBFShortcutSelectActivity.class);
	                Bundle bundle = new Bundle();
	                bundle.putStringArrayList(NAMELIST, (ArrayList<String>) mPackageNameArray);
	                intent.putExtras(bundle);
	                startActivityForResult(intent, mShortcutSelReqCode);

	                return false;
	            }
	        });
        }

        // Email addrs Click
        Preference emailPre = findPreference("setting_email_addr_set");
        emailPre.setOnPreferenceClickListener(new OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference arg0) {
                Intent intent = new Intent(MTBFActivity.this, MTBFEmailInitAcitivity.class);
                Bundle bundle = new Bundle();
                bundle.putInt(EMAIL_SIMTYPE, mEmailSimType);
                bundle.putString(EMAIL_ADDRS, mEmailAddrs);
                bundle.putString(EMAIL_PSW, mEmailPsw);
                intent.putExtras(bundle);
                startActivityForResult(intent, mEmailInfoReqCode);

                return false;
            }
        });

        // Reg MTBF Send btn
        mbtnStartMTBF = (Button) findViewById(R.id.btn_startmtbf);
        mbtnStartMTBF.setOnClickListener(this);

        // Reg Factory Reset btn
        mbtnFactorySet = (Button) findViewById(R.id.btn_factoryset);
        mbtnFactorySet.setOnClickListener(this);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (intent == null) {
            return;
        }
        Log.d(TAG, "onActivityResult " + requestCode + " " + resultCode);
        if (requestCode == mShortcutSelReqCode && resultCode == 0) {
            Bundle bundle = intent.getExtras();
            ArrayList<String> packageNameArray = bundle.getStringArrayList(NAMELIST);
            if (packageNameArray != null) {
                mPackageNameArray = packageNameArray;
            }
        } else if (requestCode == mEmailInfoReqCode && resultCode == 0) {
            Bundle bundle = intent.getExtras();
            mEmailSimType = bundle.getInt(EMAIL_SIMTYPE);
            mEmailAddrs = bundle.getString(EMAIL_ADDRS);
            mEmailPsw = bundle.getString(EMAIL_PSW);

            Log.d(TAG, "return email info: " + mEmailSimType + " " + mEmailAddrs + " " + mEmailPsw);
            Preference emailPre = findPreference("setting_email_addr_set");
            emailPre.setSummary(mEmailAddrs);
        }
    }

    private boolean initDefaultEmailAddr() {
        TelephonyManager tm = TelephonyManager.from(this);
        int phoneCount = tm.getPhoneCount();

        mEmailPsw = CUCC_DEFAULT_PSW;
        for (int i = 0; i < phoneCount; i++) {
            String simNumber = tm.getLine1Number();
            String simType = tm.getSubscriberId();

            Log.i(TAG, "simNumber = " + simNumber + "simType = " + simType);
            if (tm.hasIccCard(i)
                    && tm.getSimState(i) == TelephonyManager.SIM_STATE_READY) {

                if (simNumber != null && simType != null) {
                    if (simType.startsWith("46000") || simType.startsWith("46002")) {
                        mEmailSimType = EMAIL_SIMTYPE_CMCC;
                        mEmailAddrs = simNumber + CMCC_ADDRESS_SUFFIX;
                    } else if (simType.startsWith("46001")) {
                        mEmailSimType = EMAIL_SIMTYPE_CUCC;
                        mEmailAddrs = simNumber + CUCC_ADDRESS_SUFFIX;
                    } else {
                        continue;
                    }
                    return true;
                }
            }
        }

        return false;
    }

    private void initDefaultApkList() {
        // Default APK list
        mPackageNameArray.add("com.android.calendar");
        mPackageNameArray.add("com.android.music");
        mPackageNameArray.add("com.android.deskclock");
        mPackageNameArray.add("com.android.soundrecorder");
        mPackageNameArray.add("com.android.email");
        mPackageNameArray.add("com.sprd.fileexplorer");
        mPackageNameArray.add("com.android.providers.downloads.ui");
        mPackageNameArray.add("com.android.settings");
    }

    private boolean isAllRsped() {
        Iterator it = mResult.entrySet().iterator();
        Entry entry = null;
        String result = null;

        while (it.hasNext()) {
            entry = (Entry) it.next();
            result = (String) entry.getValue();
            if (result.compareTo(RESULT_NA) == 0) {
                return false;
            }
        }

        return true;
    }

    private void initResult() {
        PreferenceCategory categroy = null;
        Preference preItem = null;

        for (int index = 0; index < getPreferenceScreen().getPreferenceCount(); index++) {
            categroy = (PreferenceCategory) getPreferenceScreen().getPreference(index);
            for (int childIndex = 0; childIndex < categroy.getPreferenceCount(); childIndex++) {
                preItem = categroy.getPreference(childIndex);
                mResult.put(preItem.getKey(), RESULT_NA);
            }
        }
    }

    private void showResult() {
        AlertDialog.Builder builder = new Builder(MTBFActivity.this);
        String resultStr = new String();
        PreferenceCategory categroy = null;
        Preference preItem = null;
        boolean isTimeout = false;
        boolean isFail = false;
        final Timer timer = new Timer(true);

        // Report string construct
        for (int index = 0; index < getPreferenceScreen().getPreferenceCount(); index++) {
            categroy = (PreferenceCategory) getPreferenceScreen().getPreference(index);
            for (int childIndex = 0; childIndex < categroy.getPreferenceCount(); childIndex++) {
                preItem = categroy.getPreference(childIndex);
                if (mResult.get(preItem.getKey()).compareTo(RESULT_NA) == 0) {
                    resultStr += preItem.getTitle() + "  " + RESULT_TIMEOUT + "\n";
                    isTimeout = true;
                } else if (mResult.get(preItem.getKey()).compareTo(RESULT_OK) != 0) {
                    resultStr += preItem.getTitle() + "  " + mResult.get(preItem.getKey()) + "\n";
                    isFail = true;
                }
            }
        }
        if ((!isTimeout) && (!isFail)) {/* no timeout,no fail */
            resultStr += getResources().getString(R.string.all_done);
        } else if (!mIsRetry) {/*
         * Retry,if it is already the retry
         * showResult,ignor
         */
            builder.setPositiveButton(R.string.btn_retry,
                    new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    timer.cancel();
                    dialog.dismiss();
                    startMTBF(true);
                }
            });

            resultStr += getResources().getString(R.string.retry_notify);
            // Timeout timer init
            timer.schedule(new TimerTask() {
                public void run() {
                    Log.d(TAG, "Timeout and Retry");
                    mUiHandler.sendMessage(mUiHandler.obtainMessage(TIMEROUT_AND_RETRY));
                }
            }, RETRY_MS);
        }

        // Build Dialog and show
        builder.setTitle(R.string.mtbf_result);
        builder.setNegativeButton(R.string.btn_confirm,
                new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                timer.cancel();
                dialog.dismiss();
            }
        });

        builder.setMessage(resultStr);
        mAlertDialog = builder.create();
        mAlertDialog.show();
    }

    private void startMTBF(boolean isRetry) {
        mIsRetry = isRetry;

        // Disable the button
        mbtnStartMTBF.setEnabled(false);

        // Reg MTBFRSP Receiver
        IntentFilter filter = new IntentFilter("com.sprd.engineermode.action.MTBFRSP");
        registerReceiver(mReceiver, filter);

        // Init the result
        initResult();

        // Display progress dialog
        if (isRetry) {
            mProgressDialog = ProgressDialog.show(
                    this,
                    getResources().getString(R.string.mtbf_processing),
                    getResources().getString(R.string.retry) + " "
                            + getResources().getString(R.string.mtbf_processing_wait),
                            true,
                            false);

        } else {
            mProgressDialog = ProgressDialog.show(this,
                    getResources().getString(R.string.mtbf_processing),
                    getResources().getString(R.string.mtbf_processing_wait), true,
                    false);

        }

        // Send MTBF Broadcast
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putStringArrayList(NAMELIST, mPackageNameArray);
        bundle.putInt(EMAIL_SIMTYPE, mEmailSimType);
        bundle.putString(EMAIL_ADDRS, mEmailAddrs);
        bundle.putString(EMAIL_PSW, mEmailPsw);
        intent.setAction("com.sprd.engineermode.action.MTBF");
        intent.putExtras(bundle);
        sendBroadcast(intent);

        // Timeout timer init
        new Timer(true).schedule(new TimerTask() {
            public void run() {
                Log.d(TAG, "Processing timeout");
                mUiHandler.sendMessage(mUiHandler.obtainMessage(STOP_AND_SHOW_RESULT));
            }
        }, TIMEOUT_MS);
    }

    @Override
    public void onClick(View v) {
        if ((v == mbtnStartMTBF)) {
            startMTBF(false);
        } else if (v == mbtnFactorySet) {
            Intent intent = new Intent();
            intent.setAction("android.settings.BACKUP_AND_RESET_SETTINGS");

            startActivity(intent);
        }
    }
}
