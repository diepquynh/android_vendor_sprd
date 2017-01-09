package com.dream.camera.util;

import android.hardware.Camera;
import android.media.MediaRecorder;
import android.os.Build;
import android.util.Log;

import java.lang.reflect.Method;

/**
 * Created by zf on 16-8-24.
 */

public class DreamProxy {
    public static final String TAG = "DreamProxy";
    private static Class<?> mediaRecoderExClazz = null;
    private static Class<?> mediaRecoderClazz = null;
    private static Object mMediaRecorder;

    private static boolean OLD_SDK_VERSION = Build.VERSION.SDK_INT <= Build.VERSION_CODES.M ? true : false;
    static {
        try {
            mediaRecoderClazz = Class.forName("android.media.MediaRecorder");
            mediaRecoderExClazz = Class.forName("android.media.MediaRecorderEx");
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

    }

    public static Object getMediaRecoder() {
        try {
            if (OLD_SDK_VERSION) {
                mMediaRecorder = mediaRecoderClazz.newInstance();
            } else {
                mMediaRecorder = mediaRecoderExClazz.newInstance();
            }
            return mMediaRecorder;
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
            return null;
        }
    }

    public static void setParam64BitFileOffset(boolean use64BitFlag) {
        Method method;
        try {
            if (mMediaRecorder != null) {
                if (OLD_SDK_VERSION) {
                    method = mediaRecoderClazz.getMethod("setParam64BitFileOffset", boolean.class);
                    method.invoke(mMediaRecorder, use64BitFlag);
                } else {
                    method = mediaRecoderExClazz.getMethod("setParam64BitFileOffset", boolean.class);
                    method.invoke(mMediaRecorder, use64BitFlag);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

}
