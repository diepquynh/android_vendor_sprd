package com.sprd.hz.selfportrait.util;
import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import android.app.Activity;
import android.app.Dialog;
import android.content.ContentResolver;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.YuvImage;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.provider.Settings;
import android.view.Window;
import android.view.WindowManager;

import com.android.camera2.R;
public class Util {
    // The brightness setting used when it is set to automatic in the system.
    // The reason why it is set to 0.7 is just because 1.0 is too bright.
    // Use the same setting among the Camera, VideoCamera and Panorama modes.
    private static final float DEFAULT_CAMERA_BRIGHTNESS = 0.7f;
    public static void initializeScreenBrightness(Window win, ContentResolver resolver) {
        // Overright the brightness settings if it is automatic
        int mode = Settings.System.getInt(resolver, Settings.System.SCREEN_BRIGHTNESS_MODE,
                Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL);
        if (mode == Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC) {
            WindowManager.LayoutParams winParams = win.getAttributes();
            winParams.screenBrightness = DEFAULT_CAMERA_BRIGHTNESS;
            win.setAttributes(winParams);
        }
    }
    public static void Assert(boolean cond) {
        if (!cond) {
            throw new AssertionError();
        }
    }
    public static void closeSilently(Closeable c) {
        if (c == null) return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }
    public static void joinThreadSilently(Thread t) {
        if(t == null) return;
        try {
            t.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    public static void closeSilently(ParcelFileDescriptor c) {
        if (c == null) return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }
    public static void dumpToFile(byte[] data, String path) {
        FileOutputStream os = null;
        try {
            os = new FileOutputStream(path);
            os.write(data);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeSilently(os);
        }
    }
    public static void dumpNv21ToJpeg(byte[] nv21, int width, int height, String path) {
        FileOutputStream os = null;
        try {
            os = new FileOutputStream(path);
            Rect rect = new Rect(0,0,width,height);
            YuvImage img = new YuvImage(nv21, ImageFormat.NV21, width, height, null);
            img.compressToJpeg(rect, 85, os);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeSilently(os);
        }
    }
    public static Rect RectFtoRect(RectF r) {
        return new Rect((int)r.left, (int)r.top, (int)r.right, (int)r.bottom);
    }
    private static class BackgroundJob implements Runnable {
        private final Dialog mDialog;
        private final Runnable mJob;
        private final Handler mHandler;
        private final Runnable mCleanupRunner = new Runnable() {
            public void run() {
                if (mDialog.getWindow() != null)
                    mDialog.dismiss();
            }
        };
        public BackgroundJob(Activity activity, Runnable job,
                Dialog dialog, Handler handler) {
            mDialog = dialog;
            mJob = job;
            mHandler = handler;
        }
        public void run() {
            try {
                mJob.run();
            } finally {
                mHandler.post(mCleanupRunner);
            }
        }
    }
//    public static void startBackgroundJob(Activity activity,
//            String title, String message, Runnable job, Handler handler) {
//        // Make the progress dialog uncancelable, so that we can gurantee
//        // the thread will be done before the activity getting destroyed.
//        Dialog dialog = new Dialog(activity, R.style.Theme_dialog);
//        dialog.setContentView(R.layout.camera_panel_progress);
//        dialog.setCancelable(false);
//        dialog.show();
//        new Thread(new BackgroundJob(activity, job, dialog, handler)).start();
//    }
    public static void deleteDir(File dir) {
        if (dir.isDirectory()) {
            File[] files = dir.listFiles();
            for (File f:files) {
                deleteDir(f);
            }
            dir.delete();
        } else {
            dir.delete();
        }
    }
    public static String decodeUnicode(String theString) {
        char aChar;
        int len = theString.length();
        StringBuffer outBuffer = new StringBuffer(len);
        for (int x = 0; x < len;) {
            aChar = theString.charAt(x++);
            if (aChar == '\\') {
                aChar = theString.charAt(x++);
                if (aChar == 'u') {
                    // Read the xxxx
                    int value = 0;
                    for (int i = 0; i < 4; i++) {
                        aChar = theString.charAt(x++);
                        switch (aChar) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            value = (value << 4) + aChar - '0';
                            break;
                        case 'a':
                        case 'b':
                        case 'c':
                        case 'd':
                        case 'e':
                        case 'f':
                            value = (value << 4) + 10 + aChar - 'a';
                            break;
                        case 'A':
                        case 'B':
                        case 'C':
                        case 'D':
                        case 'E':
                        case 'F':
                            value = (value << 4) + 10 + aChar - 'A';
                            break;
                        default:
                            throw new IllegalArgumentException(
                                    "Malformed   \\uxxxx   encoding.");
                        }
                    }
                    outBuffer.append((char) value);
                } else {
                    if (aChar == 't')
                        aChar = '\t';
                    else if (aChar == 'r')
                        aChar = '\r';
                    else if (aChar == 'n')
                        aChar = '\n';
                    else if (aChar == 'f')
                        aChar = '\f';
                    outBuffer.append(aChar);
                }
            } else
                outBuffer.append(aChar);
        }
        return outBuffer.toString();
    }
}
