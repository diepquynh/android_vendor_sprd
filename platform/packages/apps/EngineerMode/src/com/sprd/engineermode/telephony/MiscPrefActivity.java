package com.sprd.engineermode.telephony;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.StatFs;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.widget.Toast;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.EMFileUtils;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.ShellUtils;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;

public class MiscPrefActivity extends PreferenceActivity {
    private static final String TAG = "MiscPrefActivity";
    private static final String MOUNTS_FILE = "/proc/mounts";
    private static final String SMS_FILL_MEMORY = "sms_fill_memory";
    private static final String MMS_FILL_MEMORY = "mms_fill_memory";
    private static final String SHOW_MMS_REPLY_CHOICE = "show_mms_reply_choice";
    private static final int MEM_STOP = 1;

    private static final int MSG_SMS_MEM_START = 0;
    private static final int MSG_MMS_MEM_START = 1;
    private static final int MSG_MEM_STOP = 2;
    private static final int RUNNTIME_CMD_FAIL = -126;
    private static final int MSG_MEM_BROADCAST = 1;

    private static final int TYPE_SMS = 0;
    private static final int TYPE_MMS = 1;
    private static boolean mIsProcess = false;
    private int type;

    private CheckBoxPreference mSmsFillMemoryPref;
    private CheckBoxPreference mMmsFillMemoryPref;
    private CheckBoxPreference showMMSReplyChoicePref;
    private ProgressDialog mProgressDialog;

    private MiscHandler mMiscHandler;
    private Handler uiThread = new Handler();

    BroadcastReceiver receiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "receiver : device storage lower !");
            mHandler.removeMessages(MSG_MEM_BROADCAST);
            mHandler.sendEmptyMessage(MSG_MEM_BROADCAST);
        }
    };

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            dismissProgressDialog();
            if (mIsProcess) {
                mIsProcess = false;

                if (mSmsFillMemoryPref.isChecked()) {
                    mSmsFillMemoryPref.setEnabled(true);
                    mMmsFillMemoryPref.setEnabled(false);
                }
                if (mMmsFillMemoryPref.isChecked()) {
                    mMmsFillMemoryPref.setEnabled(true);
                    mSmsFillMemoryPref.setEnabled(false);
                }
                Toast.makeText(MiscPrefActivity.this, R.string.memory_low,
                        Toast.LENGTH_SHORT).show();
            }
        }
    };

    private boolean df_main(int type) {
        String lineContent = null;
        File f = new File(MOUNTS_FILE);
        if (!f.exists()) {
            Log.d(TAG, "the file is not exists");
            return false;
        }
        BufferedReader br = null;
        FileInputStream fi = null;
        try {
            fi = new FileInputStream(f);
            br = new BufferedReader(new InputStreamReader(fi));
            while ((lineContent = br.readLine()) != null) {
                String[] datas = lineContent.split(" ");

                if ((datas.length > 2) && (0 == datas[1].compareTo("/data"))) {
                    StatFs statFs = new StatFs("/data");
                    long total_len = ((long) statFs.getBlockCount() * (long) statFs
                            .getBlockSize()) / 1024;
                    long avail_len = ((long) statFs.getFreeBlocks() * (long) statFs
                            .getBlockSize()) / 1024;

                    long left_len = 0;

                    if (type == TYPE_SMS) {
                        left_len = total_len / 20;
                    } else {
                        left_len = total_len / 10 + 400;
                    }
                    long len = avail_len - left_len;
                    if (len > 0) {
                        showProgressDialog("Filling");
                        String[] cmd = { "dd", "if=/dev/zero",
                                "of=/data/data/stuffing", "bs=1024",
                                "count=" + len };
                        Log.d(TAG, cmd[0]);
                        int runtimeValue = runtimeCmd(cmd);
                        Log.d(TAG, "Runtime " + cmd[0].toString() + ": " + runtimeValue);
                        dismissProgressDialog();
                        if (runtimeValue == 0) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                }
            }
            return false;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            if (br != null) {
                try {
                    br.close();
                } catch (IOException e) {
                }
            }
            if (fi != null) {
                try {
                    fi.close();
                } catch (IOException e) {
                }
            }
        }
    }

    private boolean stop() {
        showProgressDialog("Cleaning");
        String[] cmd = { "rm", "-f", "/data/data/stuffing", "sync" };
        int runtimeValue = runtimeCmd(cmd);
        Log.d(TAG, "Runtime " + cmd[0].toString() + ": " + runtimeValue);
        dismissProgressDialog();
        if (runtimeValue == 0) {
            return true;
        } else {
            return false;
        }
    }

    private int runtimeCmd(String[] cmd) {
        try {
            Process proc = Runtime.getRuntime().exec(cmd);
            proc.waitFor();
            Log.i(TAG,
                    "after proc.waitFor proc.exitValue = " + proc.exitValue());
            return proc.exitValue();
        } catch (IOException ioException) {
            Log.e(TAG, "Catch IOException.\n", ioException.getCause());
            return RUNNTIME_CMD_FAIL;
        } catch (InterruptedException interruptException) {
            Log.e(TAG, "Catch InterruptedException.\n",
                    interruptException.getCause());
            return RUNNTIME_CMD_FAIL;
        } catch (Exception other) {
            Log.e(TAG, "Catch Exception.\n", other.getCause());
            return RUNNTIME_CMD_FAIL;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_misc);
        PreferenceScreen prefSet = getPreferenceScreen();
        mSmsFillMemoryPref = (CheckBoxPreference) findPreference(SMS_FILL_MEMORY);
        mMmsFillMemoryPref = (CheckBoxPreference) findPreference(MMS_FILL_MEMORY);
        showMMSReplyChoicePref = (CheckBoxPreference) findPreference(SHOW_MMS_REPLY_CHOICE);
        prefSet.removePreference(mSmsFillMemoryPref);
        prefSet.removePreference(mMmsFillMemoryPref);
        boolean status = SystemProperties.getBoolean(
                "persist.sys.mms.showreplypath", false);
        if (status) {
            Log.d(TAG, "show showMMSReplyChoicePref");
            showMMSReplyChoicePref.setChecked(true);
            showMMSReplyChoicePref.setSummary("Show MMS Reply Choice");
        } else {
            Log.d(TAG, "Don't show showMMSReplyChoicePref");
            showMMSReplyChoicePref.setChecked(false);
            showMMSReplyChoicePref.setSummary("NO MMS Reply Choice");
        }
        if (mIsProcess) {
            mSmsFillMemoryPref.setEnabled(false);
            mMmsFillMemoryPref.setEnabled(false);
        }
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mMiscHandler = new MiscHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        IntentFilter filter = new IntentFilter(Intent.ACTION_DEVICE_STORAGE_LOW);
        registerReceiver(receiver, filter);
        super.onStart();
    }

    @Override
    protected void onResume() {
        if (mSmsFillMemoryPref.isChecked()) {
            mMmsFillMemoryPref.setEnabled(false);
            mSmsFillMemoryPref.setSummary("uncheck to delete memory");
        }

        if (mMmsFillMemoryPref.isChecked()) {
            mSmsFillMemoryPref.setEnabled(false);
            mMmsFillMemoryPref.setSummary("uncheck to delete memory");
        }
        super.onResume();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();
        if (SMS_FILL_MEMORY.equals(key) || MMS_FILL_MEMORY.equals(key)) {
            CheckBoxPreference checkbox = (CheckBoxPreference) preference;
            preference.setSummary("uncheck to delete memory");
            if (SMS_FILL_MEMORY.equals(key)) {
                type = MSG_SMS_MEM_START;
            } else {
                type = MSG_MMS_MEM_START;
            }
            if (checkbox.isChecked()) {
                AlertDialog alertDialog = new AlertDialog.Builder(
                        MiscPrefActivity.this)
                        .setTitle(getString(R.string.misc_dialog_warn))
                        .setMessage(getString(R.string.dialog_warn_content))
                        .setCancelable(false)
                        .setPositiveButton(getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        mIsProcess = true;
                                        mSmsFillMemoryPref.setEnabled(false);
                                        mMmsFillMemoryPref.setEnabled(false);
                                        Message fillmes = mMiscHandler
                                                .obtainMessage(type);
                                        mMiscHandler.sendMessage(fillmes);
                                    }
                                })
                        .setNegativeButton(
                                getString(R.string.alertdialog_cancel),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog,
                                            int which) {
                                        mSmsFillMemoryPref.setChecked(false);
                                        mMmsFillMemoryPref.setChecked(false);
                                        mSmsFillMemoryPref.setSummary("");
                                        mMmsFillMemoryPref.setSummary("");
                                    }
                                }).create();
                alertDialog.show();
            } else {
                checkbox.setChecked(true);
                Message stopmes = mMiscHandler.obtainMessage(MSG_MEM_STOP);
                mMiscHandler.sendMessage(stopmes);
            }
        } else if (SHOW_MMS_REPLY_CHOICE.equals(key)) {
            CheckBoxPreference checkbox = (CheckBoxPreference) preference;
            if (checkbox.isChecked()) {
                SystemProperties.set("persist.sys.mms.showreplypath", "true");
                showMMSReplyChoicePref.setSummary("Show MMS Reply Choice");
            } else {
                SystemProperties.set("persist.sys.mms.showreplypath", "false");
                showMMSReplyChoicePref.setSummary("NO MMS Reply Choice");
            }
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    protected void onDestroy() {
        if (mMiscHandler != null) {
            mMiscHandler.getLooper().quit();
        }
        unregisterReceiver(receiver);
        dismissProgressDialog();
        super.onDestroy();
    }

    private void showProgressDialog(final String str) {
        uiThread.post(new Runnable() {
            @Override
            public void run() {
                mProgressDialog = ProgressDialog.show(MiscPrefActivity.this,
                        str, "Please wait...", true, false);
            }
        });
    }

    private void dismissProgressDialog() {
        uiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                }
            }
        });
    }

    class MiscHandler extends Handler {
        public MiscHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_SMS_MEM_START:
                if (!df_main(TYPE_SMS)) {
                    if (mIsProcess) {
                        mIsProcess = false;
                    }
                    uiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mSmsFillMemoryPref.setEnabled(true);
                            mMmsFillMemoryPref.setEnabled(true);
                            mSmsFillMemoryPref.setChecked(false);
                            mSmsFillMemoryPref.setSummary("");
                            Toast.makeText(MiscPrefActivity.this, "Fill Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case MSG_MMS_MEM_START:
                if (!df_main(TYPE_MMS)) {
                    if (mIsProcess) {
                        mIsProcess = false;
                    }
                    uiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mSmsFillMemoryPref.setEnabled(true);
                            mMmsFillMemoryPref.setEnabled(true);
                            mMmsFillMemoryPref.setChecked(false);
                            mMmsFillMemoryPref.setSummary("");
                            Toast.makeText(MiscPrefActivity.this, "Fill Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case MSG_MEM_STOP:
                if (!stop()) {
                    uiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(MiscPrefActivity.this, "Clean Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                    } else {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mSmsFillMemoryPref.isChecked()) {
                                    mSmsFillMemoryPref.setChecked(false);
                                    mMmsFillMemoryPref.setEnabled(true);
                                    mSmsFillMemoryPref.setSummary("");
                                }
                                if (mMmsFillMemoryPref.isChecked()) {
                                    mMmsFillMemoryPref.setChecked(false);
                                    mSmsFillMemoryPref.setEnabled(true);
                                    mMmsFillMemoryPref.setSummary("");
                                }
                                Toast.makeText(MiscPrefActivity.this, "Clean Scuccess",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                break;
            }
        }
    }
}
