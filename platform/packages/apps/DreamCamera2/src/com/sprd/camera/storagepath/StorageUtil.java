package com.sprd.camera.storagepath;

import android.content.ContentResolver;
import android.net.Uri;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;

import com.android.camera.app.CameraServices;
import com.android.camera.data.FilmstripItemData;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/**
 * Sprd added: functions with Storage path, add Image, and so on.
 *
 */
public class StorageUtil {
    private static final String TAG = "StorageUtil";
    private static final String DEFAULT_DIR = "/DCIM/Camera";
    public static final String DCIM = Environment
            .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();

    public static final String DIRECTORY = DCIM + "/Camera";
    //public static final String INTERNALDIR = Environment
    //        .getInternalStoragePath().toString() + DEFAULT_DIR;
    //public static final String EXTERNALDIR = Environment
    //        .getExternalStoragePath().toString() + DEFAULT_DIR;
    private static final int VAL_DEFAULT_ROOT_DIRECTORY_SIZE = 2;
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";
    public static final String JPEG_POSTFIX = ".jpg";
    public static final String GIF_POSTFIX = ".gif";
    /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
//    public static String INTERNALDIR = getStoragePathState(KEY_DEFAULT_INTERNAL) ?
//            Environment.getInternalStoragePath().toString() + DEFAULT_DIR : null;
//    public static String EXTERNALDIR = getStoragePathState(KEY_DEFAULT_EXTERNAL) ?
//            Environment.getExternalStoragePath().toString() + DEFAULT_DIR : null;

    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;

    private static StorageUtil mInstance;
    private CameraServices mServices;
    private String mStorage;

    public static synchronized StorageUtil getInstance() {
        if (mInstance == null) {
            mInstance = new StorageUtil();
        }
        return mInstance;
    }

    public void initialize(CameraServices service) {
        mServices = service;
    }

    private String getCurrentStorage() {
        if (mServices != null ) {
            SettingsManager settingsManager = mServices.getSettingsManager();
            return settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_STORAGE_PATH);
        }
        return null;
    }

    public long getAvailableSpace() {
        String path = getFileDir();
        String state = null;

        Map<String, String> roots = supportedRootDirectory();
        String internal = roots.get(KEY_DEFAULT_INTERNAL);
        String external = roots.get(KEY_DEFAULT_EXTERNAL);

        if (external != null && !isStorageSetting()) {//bug521124 there is no edited photo
            forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
            path = getFileDir();
        }
        // if external storage is available but internal storage disable, force change the storage to external
        if (path == null && external != null) {
            forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
            path = getFileDir();
        }
        if (path == null) {
            return UNAVAILABLE;
        }

        // judge the path state
        if (internal != null && path.contains(internal)) {
            state = Environment.getInternalStoragePathState();
        } else if (external != null && path.contains(external)) {
            state = Environment.getExternalStoragePathState();
        }
        if (Environment.MEDIA_CHECKING.equals(state)) {
            return PREPARING;
        }

        File dir = new File(path);
        dir.mkdirs();

        /*Bug 549528 insert SD card with memory space is insufficient. @{ */
        if (dir.exists()&&(!dir.isDirectory() || !dir.canWrite())) {
            return UNAVAILABLE;
        }
        try {
            StatFs stat = new StatFs(path.replace(DEFAULT_DIR,"")); /*Bug 549528 @} */
            return (stat.getAvailableBlocksLong() * stat.getBlockSizeLong());
        } catch (Exception e) {
            Log.i(TAG, "Fail to access storage", e);
        }
        return UNKNOWN_SIZE;
    }

    /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
    public static boolean getStoragePathState(String storage) {
        Log.i(TAG, "getStoragePathState");
        if (KEY_DEFAULT_EXTERNAL.equals(storage)) {
            return Environment.MEDIA_MOUNTED.equals(Environment.getExternalStoragePathState());
        } else if (KEY_DEFAULT_INTERNAL.equals(storage)) {
            return Environment.MEDIA_MOUNTED.equals(Environment.getInternalStoragePathState());
        } else {
            /* SPRD: Fix bug 572473 add for usb storage support @{ */
            String usbStoragePath = MultiStorage.getUsbStoragePath(storage);
            if (usbStoragePath != null) {
                return Environment.MEDIA_MOUNTED.equals(
                        Environment.getUsbdiskVolumeState(new File(usbStoragePath)));
            } else {
                return false;
            }
            /* @} */
        }
    }

    public static Map<String, String> supportedRootDirectory() {
        Map<String, String> result = null;
        String internal = (getStoragePathState(KEY_DEFAULT_INTERNAL)
                ? Environment.getInternalStoragePath().getAbsolutePath() : null);
        String external = (getStoragePathState(KEY_DEFAULT_EXTERNAL)
                ? Environment.getExternalStoragePath().getAbsolutePath() : null);

//        result = new HashMap<String, String>(VAL_DEFAULT_ROOT_DIRECTORY_SIZE);
        result = new HashMap<String, String>();
        result.put(KEY_DEFAULT_INTERNAL, internal);
        result.put(KEY_DEFAULT_EXTERNAL, external);

        // SPRD: Fix bug 572473 add for usb storage support
        Map<String,String> usbMap = MultiStorage.getUsbStorageMap();
        if (usbMap != null) {
            result.putAll(usbMap);
        }
        /* @} */
        return result;
    }

    public String getFileDir() {
        Log.i(TAG, "getFileDir");
        String currentStorage = getCurrentStorage();
        if (KEY_DEFAULT_INTERNAL.equals(currentStorage)) {
            /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
            // return INTERNALDIR;
            return getStoragePathState(KEY_DEFAULT_INTERNAL) ?
                    Environment.getInternalStoragePath().toString() + DEFAULT_DIR : null;
        } else if (KEY_DEFAULT_EXTERNAL.equals(currentStorage)) {
            /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
            // return EXTERNALDIR;
            if (getStoragePathState(KEY_DEFAULT_EXTERNAL)) {
                return Environment.getExternalStoragePath().toString() + DEFAULT_DIR;
            } else {
                forceUpdateStorageSetting(KEY_DEFAULT_INTERNAL);
                return getFileDir();
            }
        } else {
            /* SPRD: Fix bug 494188 No SD card, and can not open Camera and show error message */
            /* SPRD: Fix bug 572473 add for usb storage support */
            if (getStoragePathState(currentStorage)) {
                String FileDir = MultiStorage.getUsbStoragePath(currentStorage);
                if (FileDir != null) {
                    return FileDir + DEFAULT_DIR;
                } else {
                    forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
                    return getFileDir();
                }
            } else {
                forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
                return getFileDir();
            }
        }
    }

    public String generateFilePath(String title, String mimeType) {
        String extension = null;
        if (FilmstripItemData.MIME_TYPE_JPEG.equals(mimeType)) {
            extension = JPEG_POSTFIX;
        } else if (FilmstripItemData.MIME_TYPE_GIF.equals(mimeType)) {
            extension = GIF_POSTFIX;
        } else {
            throw new IllegalArgumentException("Invalid mimeType: " + mimeType);
        }
        Log.i(TAG, "For_Test generateFilePath getFileDir() = "
                + getFileDir() + " title = " + title
                + "path = " + (new File(getFileDir(), title + extension)).getAbsolutePath());
        return (new File(getFileDir(), title + extension)).getAbsolutePath();
    }

    public void updateStorage() {
        String storage = getCurrentStorage();
        mStorage = storage;
    }

    public boolean isStorageUpdated() {
        if (mStorage == null) {
            mStorage = getCurrentStorage();
            return true;
        }
        return (!mStorage.equals(getCurrentStorage()));
    }

    public void forceUpdateStorageSetting(String storage) {
        if (mServices != null) {
            SettingsManager settingsManager = mServices.getSettingsManager();
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_STORAGE_PATH, storage);
        }
    }

    public static synchronized final String getImageBucketId(String filePath) {
        return String.valueOf(filePath.toLowerCase().hashCode());
    }

    public static void deleteImage(ContentResolver resolver, Uri uri) {
        try {
            resolver.delete(uri, null, null);
        } catch (Throwable th) {
            Log.i(TAG, "Failed to delete image: " + uri);
        }
    }

    /*SPRD:fix bug521124 there is no edited photo*/
    private boolean isStorageSetting(){
        if (mServices != null) {
            SettingsManager settingsManager = mServices.getSettingsManager();
            return settingsManager.isSet(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_STORAGE_PATH);
        }
        return false;
    }

    //SPRD:fix bug537451 pull sd card, edit and puzzle can not work.
    public String getStorageState() {
        String state = Environment.MEDIA_UNMOUNTED;
        /*SPRD: Fix bug 540799 @{ */
        String currentStorage = getCurrentStorage();
        if (currentStorage == null) {
            return state;
        }
        /* @} */
        switch(currentStorage){
            case KEY_DEFAULT_INTERNAL:
                state = Environment.getInternalStoragePathState();
                break;
            case KEY_DEFAULT_EXTERNAL:
                state = Environment.getExternalStoragePathState();
                break;
            default:
                // SPRD: Fix bug 572473 add for usb storage support
                String usbStoragePath = MultiStorage.getUsbStoragePath(currentStorage);
                if (usbStoragePath != null) {
                    state = Environment.getUsbdiskVolumeState(new File(usbStoragePath));
                }
                break;
                /* @} */
        }
        return state;
    }

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public String getPhotoVoiceDirectory(){
        String tmp = getFileDir();
        return tmp.substring(0, tmp.lastIndexOf("Camera")) + ".PhotoVoice";
    }
    /* @} */
}