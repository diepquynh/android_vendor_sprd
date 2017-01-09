
package com.sprd.engineermode.debuglog;

import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import android.content.Context;
import android.content.Intent;
import android.os.EnvironmentEx;
import android.util.Log;
import java.nio.channels.FileChannel;
import android.os.StatFs;
import android.widget.Toast;
import com.sprd.engineermode.R;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;

public class AutoDumpModemAssertReceiver extends BroadcastReceiver {
    // private static String AUTO_DUMP_ASSERT =
    // "com.sprd.engineermode.MODEM_ASSERT_DUMP";
    private static String TAG = "AutoDumpModemAssertReceiver";
    public static final String MODEM_ASSERT_PATH = "/data/com.sprd.engineermode/shared_prefs/modem_assert.xml";
    public static final String MODEM_ASSERT_NAME = "modem_assert_info.xml";
    private String MODEM_ASSERT_FILE=EnvironmentEx.getInternalStoragePath()
            .getAbsolutePath() + "/EmInfo"+"/"+MODEM_ASSERT_NAME;
    private Context mContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "auto dump modem assert to sd");
        mContext = context.getApplicationContext();
        if (SystemProperties.getBoolean("persist.sys.modemassertdump", false)) {
            if (checkSDCard()) {
                saveToSd(MODEM_ASSERT_FILE, MODEM_ASSERT_NAME);
            }
        }
    }

    public void saveToSd(String preferenceName, String name) {
        Log.d(TAG, "auto dump modem assert to sd");

        if (getSDFreeSize() <= 2) {
            Log.d(TAG, "SD size is too small");
            Toast.makeText(mContext, R.string.no_space_toast, Toast.LENGTH_SHORT).show();
            return;
        }
        File powerInfo = new File(preferenceName);
        File powerDir = new File(StorageUtil.getExternalStorage() + "/EmInfo");
        Log.d(TAG,"powerdir is "+powerDir);
        if (!powerDir.exists()) {
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
            // Toast.makeText(mContext, R.string.dump_file_toast,
            // Toast.LENGTH_SHORT).show();
        } finally {
            if (inChannel != null)
                inChannel.close();
            if (outChannel != null)
                outChannel.close();
        }
    }

    private boolean checkSDCard() {
        if (StorageUtil.getExternalStorageState()) {
            return true;
        }
        return false;
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
}
