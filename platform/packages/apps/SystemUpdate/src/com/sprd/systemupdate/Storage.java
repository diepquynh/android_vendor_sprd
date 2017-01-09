package com.sprd.systemupdate;

import java.io.File;

import android.R.bool;
import android.R.integer;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.EnvironmentEx;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;
import android.view.Gravity;
import android.widget.Toast;

public class Storage {
    interface State {
        int NIL = -1;

        int NIL_2_DOWNLOADING = 7;
        int DOWNLOADING_2_PAUSE = 8;
        int DOWNLOADING_2_NIL = 9;
        int DOWNLOADING_2_DOWNLOADED = 10;
        int PAUSE_2_DOWNLOADING = 11;
        int PAUSE_2_NIL = 12;
        int PAUSE_2_PAUSE = 13;

        int DOWNLOADED = 14;
        // tmp: debug http://bugzilla.spreadtrum.com/bugzilla/show_bug.cgi?id=565658
        int DOWNLOADED_SPECIAL = 16;
        int DOWNLOADED_DAILY = 17;

        int WAIT_UPDATE = 15;

    }

    interface fromWhere {
        int NIL = 0;
        int NOTIFI_NEW = 1;
        int NOTIFI_OLD = 2;
    }

    public static final int SDCARD_AVALIABLE = 1;
    public static final int SDCARD_NOT_MOUNTED = 2;
    public static final int SDCARD_LACK_SPACE = 3;

    // nand
    public static final int SECOND_STORAGE_TYPE_NAND = 0;
    // built-in cannot be moved
    public static final int SECOND_STORAGE_TYPE_INTERNAL = 1;
    // external removable
    public static final int SECOND_STORAGE_TYPE_EXTERNAL = 2;

    public static final String UPDATE_FILE_NAME = "update.zip";

    private static Storage sPreference;
    private Context mContext;
    private SharedPreferences mSharedPreferences;
    private File storagePath;
    private File sdCardDirectory;

    public boolean IsSdCardMounted() {
        return  (EnvironmentEx.getExternalStoragePathState().
                equals(Environment.MEDIA_MOUNTED));
    }

    public File getSdCardDirectory() {
        return  EnvironmentEx.getExternalStoragePath();
    }

    public boolean checkSdCardState() {
        if (IsSdCardMounted() == false) {
            showCenterToast(R.string.sd_card_not_mounted, Toast.LENGTH_LONG);
            return false;
        }

        sdCardDirectory = getSdCardDirectory();

        if (isAvaliableSpace(sdCardDirectory)) {
            storagePath = sdCardDirectory;
            return true;
        } else {
            showCenterToast(R.string.sd_card_lack_space, Toast.LENGTH_LONG);
            return false;
        }
    }

    public int getStorageState() {
        int state = getSdCardState();
        switch (state) {
        case SDCARD_NOT_MOUNTED:
            showCenterToast(R.string.sd_card_not_mounted, Toast.LENGTH_LONG);
            break;
        case SDCARD_LACK_SPACE:
            showCenterToast(R.string.sd_card_lack_space, Toast.LENGTH_LONG);
            break;
        default:
            break;
        }
        return state;
    }

    private void showCenterToast(int hint, int duration) {
        Toast toast = Toast.makeText(mContext, hint, duration);
        toast.setGravity(Gravity.CENTER, 0, 0);
        toast.show();
    }

    public String getStoragePath() {
        if (storagePath != null) {
            return storagePath.toString();
        } else {
            return null;
        }
    }

    public int getSdCardState() {
        if (IsSdCardMounted() == false) {
            return SDCARD_NOT_MOUNTED;
        }
        storagePath = getSdCardDirectory();
        if (isAvaliableSpace(storagePath)) {
            return SDCARD_AVALIABLE;
        } else {
            return SDCARD_LACK_SPACE;
        }
    }

    public boolean isAvaliableSpace(File path) {

        if (getLatestVersion().mSize > getStorageFreeSize(path)) {
            return false;
        }
        return true;
    }

    // bytes
    public long getStorageFreeSize(File path) {

        StatFs sf = new StatFs(path.getPath());
        long blockSize = sf.getBlockSize();
        long freeBlocks = sf.getAvailableBlocks();

        return blockSize * freeBlocks;
    }

    private Storage(Context context) {
        mContext = context;
        mSharedPreferences = mContext.getSharedPreferences("pref",
                Context.MODE_PRIVATE);
    }

    synchronized public static Storage get(Context context) {
        if (sPreference == null) {
            sPreference = new Storage(context.getApplicationContext());
        }
        return sPreference;
    }

    public String getStorageFilePath() {
        return mSharedPreferences.getString("storage_file_path",
                "/storage/sdcard0/update.zip");
    }
///mnt/sdcard/update.zip
    public void setStorageFilePath(String storageFilePath) {
        mSharedPreferences.edit()
                .putString("storage_file_path", storageFilePath).commit();
    }

    public String getDeviceId() {
        return mSharedPreferences.getString("device_id", null);
    }

    public void setDeviceId(String deviceId) {
        mSharedPreferences.edit().putString("device_id", deviceId).commit();
    }

    public VersionInfo getLatestVersion() {
        String json = mSharedPreferences.getString("latest_version", null);
        return VersionInfo.fromJson(json);
    }

    public void setLatestVersion(String json) {
        mSharedPreferences.edit().putString("latest_version", json).commit();
    }

    public int getState() {
        return mSharedPreferences.getInt("state", State.NIL);
    }

    public void setState(int state) {
        mSharedPreferences.edit().putInt("state", state).commit();
    }

    public int getSize() {
        return mSharedPreferences.getInt("size", 0);
    }

    public void setSize(int size) {
        mSharedPreferences.edit().putInt("size", size).commit();
    }

    public long getPretime() {
        return mSharedPreferences
                .getLong("pretime", System.currentTimeMillis());
    }

    public void setPretime(long pretime) {
        mSharedPreferences.edit().putLong("pretime", pretime).commit();
    }

    public long getInterval() {
        return mSharedPreferences.getLong("interval", 0);
    }

    public void setInterval(long interval) {
        mSharedPreferences.edit().putLong("interval", interval).commit();
    }

    public int getTimeOut() {
        return mSharedPreferences.getInt("timeout", 10000);
    }

    public String getToken() {
        return mSharedPreferences.getString("token", null);
    }

    public void setToken(String token) {
        mSharedPreferences.edit().putString("token", token).commit();
    }

    public String getStatus(String status) {
        return mSharedPreferences.getString(status, null);
    }

    public int getSelectId() {
        return mSharedPreferences.getInt("selectId", 0);
    }

    public void setSelectId(int selectId) {
        mSharedPreferences.edit().putInt("selectId", selectId).commit();
    }

    public boolean getUpgrade() {
        return mSharedPreferences.getBoolean("upgrade", false);
    }

    public void setUpgrade(boolean upgrade) {
        mSharedPreferences.edit().putBoolean("upgrade", upgrade).commit();
    }

    public VersionInfo getTmpLatestVersion() {
        String json = mSharedPreferences.getString("tmp_latest_version", null);
        return VersionInfo.fromJson(json);
    }

    public String getTmpLatestVersionString() {
        return mSharedPreferences.getString("tmp_latest_version", null);
    }

    public void setTmpLatestVersion(String json) {
        mSharedPreferences.edit().putString("tmp_latest_version", json).commit();
    }

}

