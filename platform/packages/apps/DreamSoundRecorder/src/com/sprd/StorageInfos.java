package com.sprd.soundrecorder;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

import android.os.Debug;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.StatFs;
import android.os.SystemProperties;
import android.util.Log;
/* SPRD: fix bug 250201 @{ */
/**
 * Storage access to information related to the class of functions
 * Main fucntion: according to the corresponding access storage shceme for different information
 *
 *
 * @author linying
 *
 */
public class StorageInfos {

    /**
     * judge the internal storage state
     *
     * @return is mounted or not
     */
    public static boolean isInternalStorageMounted() {
        String state = EnvironmentEx.getInternalStoragePathState();
        return Environment.MEDIA_MOUNTED.equals(state);
    }

    /**
     * get the internal storage directory
     *
     * @return internal storage directory
     */
    public static File getInternalStorageDirectory() {
        return EnvironmentEx.getInternalStoragePath();
    }

    /**
     * judge the External storage state
     *
     * @return is mounted or not
     */
    public static boolean isExternalStorageMounted() {
        String state = EnvironmentEx.getExternalStoragePathState();
        return Environment.MEDIA_MOUNTED.equals(state);
    }

    /**
     * get the External storage directory
     *
     * @return External storage directory
     */
    public static File getExternalStorageDirectory() {
        return EnvironmentEx.getExternalStoragePath();
    }

    /**
     *  Get the external storage path wherever sdcard is exist, or not.
     *
     * @return external storage path
     */
    public static String getExternalStorageDir() {
        return getExternalStorageDirectory().getAbsolutePath();
    }

    /**
     * judge whether the scheme for NAND
     *
     * @return true or false
     */
    public static boolean isInternalStorageSupported() {
        boolean support = false;
        if ("1".equals(SystemProperties.get("ro.device.support.nand", "0"))/* ||
            "1".equals(SystemProperties.get("ro.device.support.mmc", "0"))*/) {
            support = true;
        }
        return support;
    }
    /* @} */
    /**
     * judge whether the path is in the external storage, or not
     *
     * @param path
     * @return
     */
    public static boolean isInExternalSDCard(String path) {
        if (path == null) {
            return false;
        }

        return StorageInfos.isExternalStorageMounted() && path.startsWith(StorageInfos.getExternalStorageDir());
    }

    public static boolean isPathExistAndCanWrite(String path) {
        if(path == null) {
            return false;
        }
        File filePath = new File(path);
        return filePath.exists() && filePath.canWrite();
    }

    /**
     * check the storage space is enough
     * internal available space < 5%
     * external availabel space < 50K return false
     * @return true is enough,or false
     */
    public static boolean haveEnoughStorage(String path) {
        boolean isEnough = true;
        boolean isExternalUsed = StorageInfos.isInExternalSDCard(path);

        File savePath = null;
        if (isExternalUsed) {
            savePath = StorageInfos.getExternalStorageDirectory();
        } else {
            savePath = StorageInfos.getInternalStorageDirectory();
        }

/*        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)
                && sdCard) {*/
        /** SPRD:Bug 617209 T-card set out into the internal storage, abnormal sound recorder settings  ( @{ */
        try{
            if (savePath != null) {
                final StatFs stat = new StatFs(savePath.getPath());
                final long blockSize = stat.getBlockSize();
                //final long totalBlocks = stat.getBlockCount();
                final long availableBlocks = stat.getAvailableBlocks();

                //long mTotalSize = totalBlocks * blockSize;
                long mAvailSize = availableBlocks * blockSize;
                // Log.w(TAG, "occupied space is up to "+((mAvailSize * 100) /
                // mTotalSize));
                // isEnough = (mAvailSize * 100) / mTotalSize > 5 ? true : false;
                isEnough = mAvailSize < 50 * 1024 ? false : true;
                //        }
            }
        }catch (IllegalArgumentException e){
            return false;
        }
        /** @} */
        return isEnough;
    }

    /**
     * check the storage space is enough
     * internal available space < 5%
     * external availabel space < 50K return false
     * @return true is enough,or false
     */
    public static Map<String, String> getStorageInfo(String path) {
        Map<String,String> map = null;
        boolean isEnough = true;
        boolean isExternalUsed = StorageInfos.isInExternalSDCard(path);

        File savePath = null;
        if (isExternalUsed) {
            savePath = StorageInfos.getExternalStorageDirectory();
        } else {
            savePath = StorageInfos.getInternalStorageDirectory();
        }

/*        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)
                && sdCard) {*/
        //SPRD:Bug 629593 T-card set out into the internal storage, abnormal sound recorder settings bengin
        try{
            if (savePath != null) {
                map = new HashMap<String,String>();
                final StatFs stat = new StatFs(savePath.getPath());
                final long blockSize = stat.getBlockSize();
                //final long totalBlocks = stat.getBlockCount();
                final long availableBlocks = stat.getAvailableBlocks();
                //long mTotalSize = totalBlocks * blockSize;
                long mAvailSize = availableBlocks * blockSize;
                map.put("availableBlocks", "" + (mAvailSize - 50 * 1024));
                // Log.w(TAG, "occupied space is up to "+((mAvailSize * 100) /
                // mTotalSize));
                // isEnough = (mAvailSize * 100) / mTotalSize > 5 ? true : false;
                isEnough = mAvailSize < 50 * 1024 ? false : true;
                map.put("isEnough", "" + isEnough);
    //        }
            }
        }catch (IllegalArgumentException e){
            Log.d("StorageInfos","StatFs is exception");
        }
        //SPRD:Bug 629593 end
        return map;
    }
}
