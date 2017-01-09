package com.sprd.quickcamera;

import android.content.Context;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;

import java.io.File;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

public class StorageUtil {

    private static final boolean DEBUG = true;
    private static final String TAG = "StorageUtil";

    private static StorageUtil mInstance;

    public static synchronized StorageUtil newInstance() {
        if (mInstance == null) {
            mInstance = new StorageUtil();
        }
        return mInstance;
    }

    private static final String DIRECTORY_DCIM = Environment.DIRECTORY_DCIM;
    private static final String DIRECTORY_CAMERA = "/Camera";
    private static final String DIRECTORY_DEFAULT = "/".concat(DIRECTORY_DCIM)
            .concat(DIRECTORY_CAMERA);
    private static final String CAMERA_DEFAULT_STORAGE = Environment
            .getExternalStoragePublicDirectory(DIRECTORY_DCIM).toString();

    private static final int VAL_DEFAULT_ROOT_DIRECTORY_SIZE = 2;
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";

    public static final String TRUE = "true";
    public static final String FALSE = "false";

    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;
    public static final long LOW_STORAGE_THRESHOLD_BYTES = 20 * 1024 * 1024;

    // default construct
    public StorageUtil() {
        Log.v(TAG, "create StorageUtil instance");
    }

    public long getAvailableSpace(String path) {
        String state = null;
        Map<String, String> roots = supportedRootDirectory();
        String internal = roots.get(KEY_DEFAULT_INTERNAL);
        String external = roots.get(KEY_DEFAULT_EXTERNAL);
        if (internal == null && external == null) {
            return UNAVAILABLE;
        } else if (path != null && internal != null && path.contains(internal)) {
            state = StorageUtilProxy.getInternalStoragePathState();
        } else if (path != null && external != null && path.contains(external)) {
            state = StorageUtilProxy.getExternalStoragePathState();
        }
        if (Environment.MEDIA_CHECKING.equals(state)) {
            return PREPARING;
        }

        File dir = new File(path);
        dir.mkdirs();
        if (!dir.isDirectory() || !dir.canWrite()) {
            return UNAVAILABLE;
        }
        try {
            StatFs stat = new StatFs(path);
            return (stat.getAvailableBlocksLong() * stat.getBlockSizeLong());
        } catch (Exception e) {
            Log.i(TAG, "Fail to access storage", e);
        }
        return UNKNOWN_SIZE;
    }

    public long getInternalAvailableSpace() {

        String spath = StorageUtilProxy.getInternalStoragePath().getAbsolutePath();
        String state = null;

        File dir = new File(spath);
        dir.mkdirs();
        if (!dir.isDirectory() || !dir.canWrite()) {
            return UNAVAILABLE;
        }
        try {
            StatFs stat = new StatFs(spath);
            return (stat.getAvailableBlocksLong() * stat.getBlockSizeLong());
        } catch (Exception e) {
            Log.i(TAG, "Fail to access storage", e);
        }
        return UNKNOWN_SIZE;
    }

    public Map<String, String> supportedRootDirectory() {
        Map<String, String> result = null;
        String internal = null;
        String external = null;
        boolean isStoragepathUnified = getSystemProperties("ro.storagepath.change.support");
        Log.v(TAG, "isStoragepathUnified = " + isStoragepathUnified);

        if (!isStoragepathUnified) {
            String internal_state = StorageUtilProxy.getInternalStoragePathState(), external_state = StorageUtilProxy
                    .getExternalStoragePathState();
            boolean internal_mounted = (Environment.MEDIA_MOUNTED
                    .equals(internal_state)), external_mounted = (Environment.MEDIA_MOUNTED
                    .equals(external_state));
            internal = (internal_mounted ? StorageUtilProxy.getInternalStoragePath()
                    .getAbsolutePath() : null);
            external = (external_mounted ? StorageUtilProxy.getExternalStoragePath()
                    .getAbsolutePath() : null);

            String message = String.format(
                    "supported internal{%s, %s}, external{%s, %s}",
                    new Object[] { internal_mounted, internal,
                            external_mounted, external });
            Log.v(TAG, message);
        } else {
            String storagePath = Environment.getExternalStorageDirectory().getAbsolutePath();
            Log.v(TAG, "storagePath = " + storagePath);
            if (StorageUtilProxy.getInternalStoragePath().getAbsolutePath()
                    .equals(storagePath)) {
                internal = storagePath;
            } else if (StorageUtilProxy.getExternalStoragePath().getAbsolutePath()
                    .equals(storagePath)) {
                external = storagePath;
            }

            String message = String.format(
                    "supported internal{%s}, external{%s}", new Object[] {
                            internal, external });
            Log.v(TAG, message);
        }

        result = new HashMap<String, String>(VAL_DEFAULT_ROOT_DIRECTORY_SIZE);
        result.put(KEY_DEFAULT_INTERNAL, internal);
        result.put(KEY_DEFAULT_EXTERNAL, external);
        return result;
    }

    /* SPRD: add fix the bug 521329 quick capture picture won't save with the path
       settings which defined by camera2 app @{ */
    public String getDefaultSaveDirectory(String defaultValue) {
        String result = null;
        Map<String, String> roots = supportedRootDirectory();
        String internal = roots.get(KEY_DEFAULT_INTERNAL);
        String external = roots.get(KEY_DEFAULT_EXTERNAL);
        if (defaultValue.equals("External") && external != null) {
            result = (external.concat(DIRECTORY_DEFAULT));
        } else if (defaultValue.equals("Internal") && internal != null){
            result = (internal.concat(DIRECTORY_DEFAULT));
        } else {
            result = CAMERA_DEFAULT_STORAGE;
        }
        Log.d(TAG,"the default save directory is " + result);
        return result;
    }
    /* @} */

    public static final boolean getSystemProperties(String key) {
        boolean result = true;
        try {
            @SuppressWarnings("rawtypes")
            Class classz = Class.forName("android.os.SystemProperties");
            @SuppressWarnings("unchecked")
            Method method = classz.getMethod("get", new Class[] { String.class,
                    String.class });
            Object obj = method.invoke(null, new Object[] { key, FALSE });
            result = (TRUE.equals((String) obj));
        } catch (Exception e) {
        }
        return result;
    }

}
