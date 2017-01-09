package com.sprd.camera.storagepath;

import android.content.ContentResolver;
import android.net.Uri;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;

import com.android.camera.data.FilmstripItemData;
import com.android.camera.settings.Keys;
import com.dream.camera.settings.DataModuleBasic;
import com.sprd.camera.storagepath.StorageUtilProxy;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

/*Sprd added: functions with Storage path, add Image, and so on.
 *
 */
public class StorageUtil {
    public static final String DCIM = Environment
            .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();
    public static final String DIRECTORY = DCIM + "/Camera";
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";
    public static final String JPEG_POSTFIX = ".jpg";
    public static final String GIF_POSTFIX = ".gif";
    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;
    /*
     * For dream camera side panel icon.
     * @{
     */
    public final static int UNKNOW = -1;
    public final static int EXTERNAL = 0;
    public final static int INTERNAL = 1;
    public final static int USB = 2;
    private static final String TAG = "StorageUtil";
    private static final String DEFAULT_DIR = "/DCIM/Camera";
    /* @} */
    /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
    private static StorageUtil mInstance;
    private DataModuleBasic mDataModuleCamera;
    private String mStorage;

    public static synchronized StorageUtil getInstance() {
        if (mInstance == null) {
            mInstance = new StorageUtil();
        }
        return mInstance;
    }

    /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
    public static boolean getStoragePathState(String storage) {
        if (KEY_DEFAULT_EXTERNAL.equals(storage)) {
            return Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getExternalStoragePathState());
        } else if (KEY_DEFAULT_INTERNAL.equals(storage)) {
            return Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getInternalStoragePathState());
        } else {
            /* SPRD: Fix bug 572473 add for usb storage support @{ */
            String usbStoragePath = MultiStorage.getUsbStoragePath(storage);
            if (usbStoragePath != null) {
                return Environment.MEDIA_MOUNTED.equals(
                        StorageUtilProxy.getUsbdiskVolumeState(new File(usbStoragePath)));
            } else {
                return false;
            }
            /* @} */
        }
    }

    public static Map<String, String> supportedRootDirectory() {
        Map<String, String> result = null;
        String internal = (getStoragePathState(KEY_DEFAULT_INTERNAL)
                ? StorageUtilProxy.getInternalStoragePath().getAbsolutePath() : null);
        String external = (getStoragePathState(KEY_DEFAULT_EXTERNAL)
                ? StorageUtilProxy.getExternalStoragePath().getAbsolutePath() : null);

        // result = new HashMap<String, String>(VAL_DEFAULT_ROOT_DIRECTORY_SIZE);
        result = new HashMap<String, String>();
        result.put(KEY_DEFAULT_INTERNAL, internal);
        result.put(KEY_DEFAULT_EXTERNAL, external);

        // SPRD: Fix bug 572473 add for usb storage support
        Map<String, String> usbMap = MultiStorage.getUsbStorageMap();
        if (usbMap != null) {
            result.putAll(usbMap);
        }
        /* @} */
        return result;
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

    public void initialize(DataModuleBasic dataModuleCamera) {
        mDataModuleCamera = dataModuleCamera;
    }

    /**
     * get current storage path
     *
     * @return string of current camera storage path
     */
    public String getCurrentStorage() {
        if (mDataModuleCamera != null) {
            return mDataModuleCamera.getString(Keys.KEY_CAMERA_STORAGE_PATH, "");
        }
        return null;
    }

    /**
     * get current storage available space
     *
     * @return long the current storage available space size
     */
    public long getAvailableSpace() {
        String path = getFileDir();
        String state = null;
        Map<String, String> roots = supportedRootDirectory();
        String internal = roots.get(KEY_DEFAULT_INTERNAL);
        String external = roots.get(KEY_DEFAULT_EXTERNAL);
        //bug521124 there is no edited photo
        // if external storage is available but internal storage disable, force change the storage to external
        if (external != null && !isStorageSetting() || external != null && path == null) {
            forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
            path = getFileDir();
        }
//        else if (external == null) {
//            forceUpdateStorageSetting(KEY_DEFAULT_INTERNAL);
//            path = getFileDir();
//        }

        if (path == null) {
            return UNAVAILABLE;
        }

        // judge the path state
        if (internal != null && path.contains(internal)) {
            state = StorageUtilProxy.getInternalStoragePathState();
        } else if (external != null && path.contains(external)) {
            state = StorageUtilProxy.getExternalStoragePathState();
        }

        if (Environment.MEDIA_CHECKING.equals(state)) {
            return PREPARING;
        }
        File dir = new File(path);
        dir.mkdirs();

        /*Bug 549528 insert SD card with memory space is insufficient. @{ */
        if (dir.exists() && (!dir.isDirectory() || !dir.canWrite())) {
            return UNAVAILABLE;
        }
        try {
            StatFs stat = new StatFs(path.replace(DEFAULT_DIR, "")); /*Bug 549528 @} */
            return (stat.getAvailableBlocksLong() * stat.getBlockSizeLong());
        } catch (Exception e) {
            Log.i(TAG, "Fail to access storage", e);
        }
        return UNKNOWN_SIZE;
    }

    public String getFileDir() {
        String currentStorage = getCurrentStorage();
        if (KEY_DEFAULT_INTERNAL.equals(currentStorage)) {
            /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
            return getStoragePathState(KEY_DEFAULT_INTERNAL) ?
                    StorageUtilProxy.getInternalStoragePath().toString() + DEFAULT_DIR : null;
        } else if (KEY_DEFAULT_EXTERNAL.equals(currentStorage)) {
            /* SPRD:fix bug 494188 No SD card, and can not open Camera and show error message */
            if (getStoragePathState(KEY_DEFAULT_EXTERNAL)) {
                return StorageUtilProxy.getExternalStoragePath().toString() + DEFAULT_DIR;
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
//                forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
//                return getFileDir();
                return null;
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

        String fileDir = getFileDir();
        if(fileDir == null){
            throw new IllegalArgumentException("Invalid fileDir ");
        }
        String result = (new File(getFileDir(), title + extension)).getAbsolutePath();
        Log.i(TAG, "generateFilePath " + result);
        return result;
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

    /**
     * force update current storage path when the storage path has changed
     *
     * @param storage the name of path want to change
     */
    public void forceUpdateStorageSetting(String storage) {
        if (mDataModuleCamera != null) {
            mDataModuleCamera.set(Keys.KEY_CAMERA_STORAGE_PATH, storage);
        }
    }

    public void forceChangeStorageSetting(String storage) {
        if (mDataModuleCamera != null) {
            mDataModuleCamera.changeSettings(Keys.KEY_CAMERA_STORAGE_PATH, storage);
        }
    }

    /*SPRD:fix bug521124 there is no edited photo*/
    private boolean isStorageSetting() {
        if (mDataModuleCamera != null) {
            return mDataModuleCamera.isSet(Keys.KEY_CAMERA_STORAGE_PATH);
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
        switch (currentStorage) {
            case KEY_DEFAULT_INTERNAL:
                state = StorageUtilProxy.getInternalStoragePathState();
                break;
            case KEY_DEFAULT_EXTERNAL:
                state = StorageUtilProxy.getExternalStoragePathState();
                break;
            default:
                // SPRD: Fix bug 572473 add for usb storage support
                String usbStoragePath = MultiStorage.getUsbStoragePath(currentStorage);
                if (usbStoragePath != null) {
                    state = StorageUtilProxy.getUsbdiskVolumeState(new File(usbStoragePath));
                }
                break;
                /* @} */
        }
        return state;
    }

    public int getSideStorageState() {
        String currentStorage = getCurrentStorage();
        if (KEY_DEFAULT_INTERNAL.equals(currentStorage)) {
            return getStoragePathState(KEY_DEFAULT_INTERNAL) ? INTERNAL : UNKNOW;
        } else if (KEY_DEFAULT_EXTERNAL.equals(currentStorage)) {
            return getStoragePathState(KEY_DEFAULT_EXTERNAL) ? EXTERNAL :
                    getStoragePathState(KEY_DEFAULT_INTERNAL) ? INTERNAL : UNKNOW;
        } else {
            // for usb storage
            if (getStoragePathState(currentStorage)) {
                return USB;
            } else {
//                forceUpdateStorageSetting(KEY_DEFAULT_EXTERNAL);
//                return getSideStorageState();
                return UNKNOW;
            }
        }
    }
    /* @} */

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public String getPhotoVoiceDirectory() {
        String tmp = getFileDir();
        return tmp.substring(0, tmp.lastIndexOf("Camera")) + ".PhotoVoice";
    }
    /* @} */

}
