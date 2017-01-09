/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 */
package com.ucamera.ugallery.util;

import android.app.Activity;
import android.app.ProgressDialog;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.AsyncTask;
import android.view.Window;

/**
 * There are 3 steps of buffers for preview and jpeg data rotation
 * 1. Raw preview(or picture data for jpeg) which is the sensor buffer without rotation
 * 2. Mirrored data on some front sensors with a mirror effect
 * 3. Display data that step2 buffer has been rotated for display
 * * Note : the onPreviewFrame() get the preview frame data without mirror, and callbacked picture so do
 **/

public class RotationUtil {
    // Rotates the bitmap by the specified degree.
    // If a new bitmap is created, the original bitmap is recycled.
    public static Bitmap rotate(Bitmap b, int degrees) {
        return mirrorAndRotate(b,degrees,false);
    }

    // first mirror then rotate the bitmap. If a new bitmap is created, the
    // original bitmap is recycled.
    private static Bitmap mirrorAndRotate(Bitmap b, int degrees, boolean mirror) {
        if (b == null  || b.isRecycled()) return null;

        degrees = degrees % 360;
        if (degrees == 0 && !mirror) {
            return b;
        }

        if (Compatible.instance().mIsMeizuMX){
            // on Meizu MX the rotate(b, 180) is not work
            // so replace it by two rotate 90
            if (degrees == 180 && !mirror){
                b = mirrorAndRotate(b, 90, false);
                b = mirrorAndRotate(b, 90, false);
                return b;
            }
        }

        try {
            final float w = b.getWidth();
            final float h = b.getHeight();

            Matrix m = new Matrix();
            if (mirror) {
                m.setScale(-1, 1);
                m.postTranslate(w, 0);
            }
            m.postRotate(degrees, w/2, h/2);

            Bitmap b2 = Bitmap.createBitmap(b, 0, 0,(int)w, (int)h, m, true);
            Util.recyleBitmap(b);
            return b2;
        } catch (OutOfMemoryError ex) {
            ex.printStackTrace();
            // We have no memory to rotate. Return the original bitmap.
        } catch (NullPointerException ex) {
            android.util.Log.w("NULL", "Why is null raised? b is null?" + (b==null), ex);
        }

        // something unexpected occured, return original bitmap
        return b;
    }

    public static void startBackgroundJob(final Activity activity,
            final String title, final String message, final Runnable job) {
        // Make the progress dialog uncancelable, so that we can gurantee
        // the thread will be done before the activity getting destroyed.
        new AsyncTask<Void, Void, Void> () {
            private ProgressDialog mDialog;
            @Override
            protected void onPreExecute() {
                if(activity != null && !activity.isFinishing()) {
                    mDialog = showProgressDialog(activity, title, message, false, false);
                }
            }

            @Override
            protected void onPostExecute(Void result) {
                /*
                 * FIX BUG: 5192
                 * BUG COMMENT: View not attached to window manager
                 * DATE: 2013-11-12
                 */
                if (activity != null && !activity.isFinishing() &&
                        mDialog != null && mDialog.getWindow() != null) {
                    mDialog.dismiss();
                }
            }
            @Override
            protected Void doInBackground(Void ... args) {
                if (job != null) {
                    job.run();
                }
                return null;
            }
        }.execute();
    }

    private static ProgressDialog showProgressDialog(
            Activity activity, String title, String message, boolean indeterminate, boolean flag) {
        ProgressDialog dialog = new ProgressDialog(activity);
        dialog.requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
//        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setIndeterminate(indeterminate);
        dialog.setCancelable(flag);
        dialog.show();
        return dialog;
    }
}
