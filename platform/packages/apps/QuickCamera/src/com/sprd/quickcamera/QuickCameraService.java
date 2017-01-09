package com.sprd.quickcamera;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Intent;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.os.PowerManager;
import android.content.Context;

public class QuickCameraService extends Service {
    private static final String TAG = "QuickCameraService";

    private QuickCamera mQuickCamera;
    //private String mStoragePath;
    private Messenger mCallback;

    public static final int MODE_START_CAMERA_AND_CAPTURE = 0;
    public static final int MODE_START_CAMERA_ONLY = 1;
    public static final int MODE_CAPTURE_WITH_BACK_CAMERA = 2;
    public static final int MODE_CAPTURE_WITH_FRONT_CAMERA = 3;
    public static final int MODE_CAPTURE_OFF = 4;
    public static final String PATH_EXTERNAL = "External";
    public static final String PATH_INTERNAL = "Internal";

    public static int mMode = MODE_CAPTURE_WITH_BACK_CAMERA;
    public static String mStoragePath = PATH_INTERNAL;

//    private PowerManager.WakeLock mWakeLock;

    public static int getMode() {
        return mMode;
    }

    public static String getStoragePath() {
        return mStoragePath;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        /* SPRD: Fix bug 567394 when switch user the state of the quickcamera is wrong @{ */
        String mode = Settings.Global.getString(getContentResolver()
                , "camera_quick_capture_userid_" + ActivityManager.getCurrentUser());
        /* @} */
        if (!TextUtils.isEmpty(mode)) {
            mMode = Integer.parseInt(mode);
        }
        Log.i(TAG, "current mode " + modeToString(mMode));

        /* SPRD: add fix the bug 521329 quick capture picture won't save with the path
           settings which defined by camera2 app @{ */
        String storagePath = Settings.Global.getString(getContentResolver()
                ,"camera_quick_capture_storage_path");
        if (!TextUtils.isEmpty(storagePath)) {
            if (storagePath.equals("External")) {
                mStoragePath = PATH_EXTERNAL;
            } else if (storagePath.equals("Internal")) {
                mStoragePath = PATH_INTERNAL;
            }
            Log.d(TAG,"the camera storage path is " + mStoragePath);
        } else {
            // SPRD: Fix bug 577390, pictures captured from QuickCamera are always saved in built-in storage
            if (Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getExternalStoragePathState())) {
                mStoragePath = PATH_EXTERNAL;
            }
        }
        /* @} */
    }

    private String modeToString(int mode) {
        String modeString = null;
        switch (mode) {
            case MODE_START_CAMERA_AND_CAPTURE:
                modeString = "MODE_START_CAMERA_AND_CAPTURE";
                break;
            case MODE_START_CAMERA_ONLY:
                modeString = "MODE_START_CAMERA_ONLY";
                break;
            case MODE_CAPTURE_WITH_BACK_CAMERA:
                modeString = "MODE_CAPTURE_WITH_BACK_CAMERA";
                break;
            case MODE_CAPTURE_WITH_FRONT_CAMERA:
                modeString = "MODE_CAPTURE_WITH_FRONT_CAMERA";
                break;
            case MODE_CAPTURE_OFF:
                modeString = "MODE_CAPTURE_OFF";
                break;
            default :
                modeString = "UNKNOWN";
                break;
        }
        return modeString;
    }

    private boolean isModeValid(int mode) {
        // we haven't implemented MODE_START_CAMERA_ONLY, maybe do in future.
        if (mode == MODE_START_CAMERA_AND_CAPTURE
                || mode == MODE_CAPTURE_WITH_BACK_CAMERA
                || mode == MODE_CAPTURE_WITH_FRONT_CAMERA) {
            return true;
        }
        return false;
    }

    private Runnable mRunnable = new Runnable() {
        @Override
        public void run() {
            Message reply = Message.obtain(null, 1);
            try {
                mCallback.send(reply);
            } catch (RemoteException e) {
            }
        }
    };

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case 1:
                mCallback = msg.replyTo;

                if (!isModeValid(mMode)) {
                    mRunnable.run();
                    return;
                }

                boolean surfaceVisible = false;
                if (mMode == MODE_START_CAMERA_AND_CAPTURE) {
                    surfaceVisible = true;
                } else {
                    surfaceVisible = false;
                }
                mQuickCamera = new QuickCamera(QuickCameraService.this, surfaceVisible /* surfaceVisible */);
                mQuickCamera.takeQuickShot(mRunnable);
                break;
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return new Messenger(mHandler).getBinder();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return false;
    }
}
