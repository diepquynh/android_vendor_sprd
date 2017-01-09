
package com.sprd.engineermode.debuglog;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.content.Context;
import android.os.EnvironmentEx;
import android.preference.PreferenceManager;
import android.content.SharedPreferences;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import com.sprd.engineermode.R;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;
import com.sprd.engineermode.utils.EMFileUtils;

import java.io.FileNotFoundException;
import java.io.IOException;
import android.widget.Toast;
import android.widget.ListView;
import java.nio.channels.FileChannel;
import android.os.StatFs;

public class SwitchBaseActivity extends Activity {
    private static final String TAG = "SwitchBaseActivity";

    public static final String POWER_ON_PREF_NAME = "power_on";
    public static final String POWER_OFF_PREF_NAME = "power_off";
    public static final String MODEM_ASSERT_PREF_NAME = "modem_assert";
    public static final String BATTERY_LIFE_TIME = "battery_life_time";

    public static final String INFO_COUNT = "info_count";

    public static SharedPreferences.Editor mEditor;
    public static SharedPreferences mPowerOnPref, mPowerOffPref, mModemAssertPref,
            mBatteryLifeTime;

    public static long mPowerOnCount = 0;
    public static long mPowerOffCount = 0;
    public static long mModemAssertCount = 0;
    public ListView mListView;
    private static Context mContext;

    public boolean mIsMounted = false;
    public static final String POWER_ON_PATH = "/data/data/com.sprd.engineermode/shared_prefs/power_on.xml";
    public static final String POWER_ON_NAME = "power_on_info.xml";
    public static final String POWER_OFF_PATH = "/data/data/com.sprd.engineermode/shared_prefs/power_off.xml";
    public static final String POWER_OFF_NAME = "power_off_info.xml";
    public static final String MODEM_ASSERT_PATH = "/data/com.sprd.engineermode/shared_prefs/modem_assert.xml";
    public static final String MODEM_ASSERT_NAME = "modem_assert_info.xml";
    public static final File INTERNAL_STORAGE_PATH =EnvironmentEx.getInternalStoragePath();
    public static String MODEM_ASSERT_FILE=INTERNAL_STORAGE_PATH.getAbsolutePath() + "/EmInfo"+"/"+MODEM_ASSERT_NAME;

    public static final String PREF_INFO_NUM = "info_";
    public static final String PREF_INFO_TIME = "_time";
    public static final String PREF_INFO_MODE = "_mode";
    public static final String PREF_MODEM_INFO = "_info";

    public static final String PREF_OPEN_TIME = "open_time";
    public static final String PREF_OPEN_BATTERY = "open_battery";
    public static final String PREF_SHUT_DOWN_INFO = "shut_down_info";
    public static final String PREF_TOTAL_TIME = "total_time";
    public static final String PREF_IS_FIRST_SHUTDOWN = "first_shutdown";

    public static final int MODE_POWER_ON = 1;
    public static final int MODE_POWER_OFF = 2;
    public static final int MODE_MODEM_ASSERT = 3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.switch_machine);
        mContext=this;
    }

    @Override
    protected void onResume() {
        super.onResume();
        mListView = (ListView) findViewById(R.id.list);
        checkSDCard();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.switch_machine_menu, menu);
        return true;
    }

    public void clearInfo(){
        EMFileUtils.write(MODEM_ASSERT_FILE, "",false);
    }
    public static void saveToInternal(String modemData){
        if (getInternalFreeSize() <= 2) {
            Log.d(TAG, "Internal size is too small");
            Toast.makeText(mContext, R.string.no_space_toast, Toast.LENGTH_SHORT).show();
            return;
        }
        String fileDir=INTERNAL_STORAGE_PATH
                .getAbsolutePath() + "/EmInfo";
        Log.d(TAG,"the internal file dir path is "+fileDir);
        EMFileUtils.newFolder(fileDir);
        EMFileUtils.write(MODEM_ASSERT_FILE, modemData+"\n",true);
    }
    public void saveToSd(String preferenceName, String name) {
        if (!mIsMounted) {
            Log.d(TAG, "no SD");
            return;
        }
        if (getSDFreeSize() <= 2) {
            Log.d(TAG, "SD size is not too small");
            Toast.makeText(this, R.string.no_space_toast, Toast.LENGTH_SHORT).show();
            return;
        }
        File powerInfo = new File(preferenceName);
        File powerDir = new File(StorageUtil.getExternalStorage() + "/EmInfo");
        if (!powerDir.exists())
        {
            Log.d(TAG, "!powerDir.exists()");
            powerDir.mkdirs();
        }

        File newFile = new File(powerDir, name);
        try {
            newFile.createNewFile();
            copyFile(powerInfo, newFile);
            Log.d(TAG, "sucess dump file to sd");
        } catch (IOException e) {
            Log.d(TAG, "dump fail because IOException");

        }
    }

    public void copyFile(File src, File dst) throws IOException {
        FileChannel inChannel = new FileInputStream(src).getChannel();
        FileChannel outChannel = new FileOutputStream(dst).getChannel();
        try {
            inChannel.transferTo(0, inChannel.size(), outChannel);
            Toast.makeText(this, R.string.dump_file_toast, Toast.LENGTH_SHORT).show();
        } finally {
            if (inChannel != null)
                inChannel.close();
            if (outChannel != null)
                outChannel.close();
        }
    }

    protected void checkSDCard() {
        if (StorageUtil.getExternalStorageState()) {
            mIsMounted = true;
        }else {
            mIsMounted = false;
        }
        Log.d(TAG, "bool mIsMounted= " + mIsMounted);
    }

    public long getSDFreeSize() {
        File path = StorageUtil.getExternalStorage();
        StatFs sf = new StatFs(path.getPath());
        long blockSize = sf.getBlockSize();
        long freeBlocks = sf.getAvailableBlocks();
        // return freeBlocks * blockSize; //Byte
        // return (freeBlocks * blockSize)/1024; //KB
        Long freeSize = (freeBlocks * blockSize) / 1024 / 1024; // MB
        Log.d(TAG, "freeSize = " + freeSize);
        return freeSize;
    }

    public static long getInternalFreeSize() {
        File path = INTERNAL_STORAGE_PATH;
        Log.d(TAG,"path.getPath() is "+path.getPath());
        StatFs sf = new StatFs(path.getPath());
        long blockSize = sf.getBlockSize();
        long freeBlocks = sf.getAvailableBlocks();
        // return freeBlocks * blockSize; //Byte
        // return (freeBlocks * blockSize)/1024; //KB
        Long freeSize = (freeBlocks * blockSize) / 1024 / 1024; // MB
        Log.d(TAG, "internal storage freeSize = " + freeSize);
        return freeSize;
    }

    public void makeMenuItemVisible(Menu menu, int itemId, boolean visible) {
        MenuItem item = menu.findItem(itemId);
        if (item != null) {
            item.setVisible(visible);
        }
    }

    public void makeMenuItemEnabled(Menu menu, int itemId, boolean enable) {
        MenuItem item = menu.findItem(itemId);
        if (item != null) {
            item.setEnabled(enable);
        }
    }

    @Override
    public void finish() {
        super.finish();
    }

}
