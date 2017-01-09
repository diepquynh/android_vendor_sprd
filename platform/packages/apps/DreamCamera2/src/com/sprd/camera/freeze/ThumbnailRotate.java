package com.sprd.camera.freeze;

import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.util.Log;
public class ThumbnailRotate {
    private static final String TAG = "ThumbnailRotate";
    public static Bitmap proxyRotateImage(Bitmap bitmap, int orientation) {
        return rotateImage(bitmap, orientation);
   }

    private static Bitmap rotateImage(Bitmap bitmap, int orientation) {
        if (orientation != 0) {
            // We only rotate the thumbnail once even if we get OOM.
            Matrix m = new Matrix();
            m.setRotate(orientation, bitmap.getWidth() * 0.5f, bitmap.getHeight() * 0.5f);
            try {
                    Bitmap rotated = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(), bitmap.getHeight(), m, true);
                    // If the rotated bitmap is the original bitmap, then it should not be recycled.
                    if (rotated != bitmap) bitmap.recycle();
                    return rotated;
            } catch (Throwable t) {
                    Log.w(TAG, "Failed to rotate thumbnail", t);
            }
        }
        return bitmap;
    }
}
