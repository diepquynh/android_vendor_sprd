package com.sprd.camera.storagepath;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Sprd added: For Multi Storage External and Internal
 *
 * @apiNote This File Just contains functions with devices,
 * such as sd card, internal card. But no functions with
 * Storage path, add Image, and so on.
 * Take Attention on Note when you Add Functions!
 */
public class MultiStorage {
    //    public static final int VAL_DEFAULT_ROOT_DIRECTORY_SIZE = 2;
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";
    private static final String TAG = "MultiStorage";
    @SuppressLint("StaticFieldLeak")
    // maybe have a memory leak when use activity context, but now we use the application context,
    // so ignore this warning
    private static MultiStorage mInstance;
    private Context mContext;

    /**
     * SPRD: add for feature  bug 572473 OTG storage support<br>
     * <p>
     * in this method we changed this class's design patterns to single instance, the purpose is provide
     * a context for {@link #getUsbStorageMap()} and {@link #checkSupportedUsbStorage(List)}
     *
     * @return a {@link MultiStorage} object which is single instance
     */
    public static synchronized MultiStorage getInstance() {
        if (mInstance == null) {
            mInstance = new MultiStorage();
        }
        return mInstance;
    }

    /**
     * get current supported storage list
     *
     * @return current supported storage list:max len = 2:internal/external,min
     * len = 1:internal
     */
    public static List<String> getSupportedStorage() {
//        List<String> supportedStorage = new ArrayList<String>(VAL_DEFAULT_ROOT_DIRECTORY_SIZE);
        List<String> supportedStorage = new ArrayList<>();

        if (StorageUtil.getStoragePathState(KEY_DEFAULT_INTERNAL)) {
            Log.d(TAG, "internal storage found");
            supportedStorage.add(KEY_DEFAULT_INTERNAL);
        }
        if (StorageUtil.getStoragePathState(KEY_DEFAULT_EXTERNAL)) {
            Log.d(TAG, "external storage found");
            supportedStorage.add(KEY_DEFAULT_EXTERNAL);
        }
        // SPRD: Fix bug 572473 add for usb storage support
        checkSupportedUsbStorage(supportedStorage);

        return supportedStorage;
    }

    /**
     * SPRD: add for feature  bug 572473 OTG storage support<br>
     * <p>
     * this private static method is service to {@link #getSupportedStorage()},in order to get the
     * supported Usb devices by system
     *
     * @param supportedStorage a ArrayList to storage the value of devices name
     * @see #getSupportedStorage()
     */
    private static void checkSupportedUsbStorage(List<String> supportedStorage) {
        if (mInstance == null) {
            Log.d(TAG, "checkSupportedUsbStorage: mInstance is null returned!");
            return;
        }
        StorageManager storageManager = mInstance.mContext.getSystemService(StorageManager.class);
        List<VolumeInfo> volumes = StorageUtilProxy.getUsbdiskVolumes();
        if(volumes == null){
            return;
        }
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        for (VolumeInfo vol : volumes) {
            File file = vol.getPath();
            if (file == null) {
                Log.d(TAG, "checkSupportedUsbStorage: storage is wrong");
                continue;
            }
            if (Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getUsbdiskVolumeState(file))) {
                supportedStorage.add(storageManager.getBestVolumeDescription(vol));
                Log.d(TAG, "checkSupportedUsbStorage: find storage : " + storageManager.getBestVolumeDescription(vol));
            }
        }
    }

    /**
     * SPRD: add for feature  bug 572473 OTG storage support
     *
     * @return a HashMap Object which contains the external Usb storage device's name and the true path,
     * Before calling this method, you need to call {@link #initialize(Context)} to initialize the class,
     * in order to get the USB storage path. Otherwise it will return null.
     * @see #initialize(Context)
     */
    public static Map<String, String> getUsbStorageMap() {
        if (mInstance == null) {
            return null;
        }
        Map<String, String> StoragePathMap = new HashMap<>();
        StorageManager storageManager = mInstance.mContext.getSystemService(StorageManager.class);
        List<VolumeInfo> volumes = StorageUtilProxy.getUsbdiskVolumes();
        if(volumes == null){
            return StoragePathMap;
        }
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        for (VolumeInfo vol : volumes) {
            File file = vol.getPath();
            if (file == null) {
                continue;
            }
            StoragePathMap.put(storageManager.getBestVolumeDescription(vol), file.getPath());
            Log.d(TAG, "getUsbStorageMap: key = " + storageManager.getBestVolumeDescription(vol)
                    + "value = " + file.getPath());
        }
        return StoragePathMap;
    }

    /**
     * SPRD: add for feature bug 572473 OTG storage support
     *
     * @param storageDesc Name used to get the real path of the usb device
     * @return the real path of the usb device. before call this method you need to call
     * {@link #initialize(Context)} to initialize the class, in order to get the USB storage path.
     * Otherwise it will return null.
     * @see #initialize(Context)
     */
    public static String getUsbStoragePath(String storageDesc) {
        Map<String, String> tempMap = getUsbStorageMap();
        if (tempMap != null) {
            return tempMap.get(storageDesc);
        }
        return null;
    }

    /**
     * SPRD: add for feature bug 572473 OTG storage support<br>
     *
     * @param context a context for {@link #getUsbStorageMap()}
     *                and {@link #checkSupportedUsbStorage(List)} use;
     *                must be a application avoid memory leak
     * @see #getUsbStorageMap()
     * @see #checkSupportedUsbStorage(List)
     */
    public void initialize(Context context) {
        mContext = context;
    }
}