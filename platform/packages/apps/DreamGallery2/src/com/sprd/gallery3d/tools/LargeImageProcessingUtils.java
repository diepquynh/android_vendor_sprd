
package com.sprd.gallery3d.tools;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory.Options;
import android.graphics.BitmapRegionDecoder;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.net.Uri;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Video;
import android.util.Log;

/**
 * This class contains some public methods for processing large images which are
 * easy to cause OOM issues.
 */
public class LargeImageProcessingUtils {
    private static final String TAG = "Gallery2/LargeImageProcessingUtils";
    private static final int MAX_BITMAP_RESIZE_RATIO = 4;
    private static final int MAX_BITMAP_RESIZE_ATTEMPTS = 5;
    private static int mRatio = 1;
    private static final String DATA = "_data";
    private static final String MIME_TYPE = "mime_type";

    public static Bitmap createBitmapWithFixedSize(Bitmap source, int x, int y, int width,
            int height) {
        boolean noBitmap = true;
        int ratio = 1;
        Bitmap bitmap = null;
        while (noBitmap) {
            try {
                bitmap = Bitmap.createBitmap(source, x / (ratio * getResizeRatio()), y
                        / (ratio * getResizeRatio()), width / (ratio * getResizeRatio()), height
                        / (ratio * getResizeRatio()));
                noBitmap = false;
            } catch (OutOfMemoryError e) {
                if (ratio > MAX_BITMAP_RESIZE_RATIO) {
                    throw e;
                }
                bitmap = null;
                System.gc();
                ratio *= 2;
                source = resizeBitmapByScale(source, 1.0f / ratio, true);
                if (source == null) {
                    return null;
                }
            } catch (Exception e) {
                noBitmap = false;
                bitmap = null;
                Log.w(TAG, "Create bitmap failed.");
                e.printStackTrace();
            }
        }
        return bitmap;
    }

    public static Bitmap createBitmapWithFixedSize(int width, int height, Config config) {
        boolean noBitmap = true;
        int ratio = 1;
        Bitmap bitmap = null;
        while (noBitmap) {
            try {
                bitmap = Bitmap.createBitmap(width / ratio, height / ratio, config);
                noBitmap = false;
                mRatio = ratio;
            } catch (OutOfMemoryError e) {
                if (ratio > MAX_BITMAP_RESIZE_RATIO) {
                    throw e;
                }
                bitmap = null;
                System.gc();
                ratio *= 2;
            } catch (Exception e) {
                bitmap = null;
                noBitmap = false;
                Log.w(TAG, "Create bitmap failed. ");
                e.printStackTrace();
            }
        }
        return bitmap;
    }

    public static int getResizeRatio() {
        return mRatio;
    }

    public static Bitmap decodeRegion(BitmapRegionDecoder decoder, Rect rect, Options options) {
        boolean noBitmap = true;
        int sampleSize = 1;
        int num_tries = 0;
        Bitmap bitmap = null;
        while (noBitmap) {
            try {
                options.inSampleSize = sampleSize;
                bitmap = decoder.decodeRegion(rect, options);
                noBitmap = false;
            } catch (OutOfMemoryError e) {
                // try 5 times before failing.
                if (++num_tries >= MAX_BITMAP_RESIZE_ATTEMPTS) {
                    throw e;
                }
                System.gc();
                sampleSize *= 2;
                bitmap = null;
            }
        }
        return bitmap;
    }

    /**
     * Ridiculous way to read the devices maximum texture size because no other
     * way is provided.
     */
    public static Integer computeEglMaxTextureSize() {
        EGLDisplay eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        int[] majorMinor = new int[2];
        EGL14.eglInitialize(eglDisplay, majorMinor, 0, majorMinor, 1);

        int[] configAttr = {
                EGL14.EGL_COLOR_BUFFER_TYPE, EGL14.EGL_RGB_BUFFER,
                EGL14.EGL_LEVEL, 0,
                EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT,
                EGL14.EGL_SURFACE_TYPE, EGL14.EGL_PBUFFER_BIT,
                EGL14.EGL_NONE
        };
        EGLConfig[] eglConfigs = new EGLConfig[1];
        int[] configCount = new int[1];
        EGL14.eglChooseConfig(eglDisplay, configAttr, 0,
                eglConfigs, 0, 1, configCount, 0);

        if (configCount[0] == 0) {
            Log.w(TAG, "computeEglMaxTextureSize() -> No EGL configurations found!");
            return null;
        }
        EGLConfig eglConfig = eglConfigs[0];

        // Create a tiny surface
        int[] eglSurfaceAttributes = {
                EGL14.EGL_WIDTH, 64,
                EGL14.EGL_HEIGHT, 64,
                EGL14.EGL_NONE
        };
        //
        EGLSurface eglSurface = EGL14.eglCreatePbufferSurface(eglDisplay, eglConfig,
                eglSurfaceAttributes, 0);

        int[] eglContextAttributes = {
                EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL14.EGL_NONE
        };

        // Create an EGL context.
        EGLContext eglContext = EGL14.eglCreateContext(eglDisplay, eglConfig, EGL14.EGL_NO_CONTEXT,
                eglContextAttributes, 0);
        EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

        // Actually read the Gl_MAX_TEXTURE_SIZE into the array.
        int[] maxSize = new int[1];
        GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, maxSize, 0);
        int result = maxSize[0];

        // Tear down the surface, context, and display.
        EGL14.eglMakeCurrent(eglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
                EGL14.EGL_NO_CONTEXT);
        EGL14.eglDestroySurface(eglDisplay, eglSurface);
        EGL14.eglDestroyContext(eglDisplay, eglContext);
        EGL14.eglTerminate(eglDisplay);

        // Return the computed max size.
        return result;
    }

    public static int getProperSampleSize(int width, int height) {
        int sampleSize = 1;
        long maxMemory = Runtime.getRuntime().maxMemory();
        while ((width >> (sampleSize - 1)) * (height >> (sampleSize - 1)) * 4 > maxMemory) {
            if (sampleSize > 5) {
                break;
            }
            sampleSize++;
        }
        return sampleSize;
    }

    /**
     * add for resize bitmap
     */
    public static Bitmap resizeBitmapByScale(
            Bitmap bitmap, float scale, boolean recycle) {
        int width = Math.round(bitmap.getWidth() * scale);
        int height = Math.round(bitmap.getHeight() * scale);
        if (width < 1 || height < 1) {
            Log.i(TAG, "scaled width or height < 1, no need to resize");
            return bitmap;
        }
        if (width == bitmap.getWidth()
                && height == bitmap.getHeight()) {
            return bitmap;
        }
        Bitmap target = null;
        try {
            target = createBitmapWithFixedSize(width, height, getConfig(bitmap));
        } catch (OutOfMemoryError e) {
            Log.w(TAG, "Memory is too low, resize bitmap failed." + e);
            System.gc();
            return null;
        }
        Canvas canvas = new Canvas(target);
        scale = scale * (1.0f / getResizeRatio());
        canvas.scale(scale, scale);
        Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
        canvas.drawBitmap(bitmap, 0, 0, paint);
        if (recycle) {
            bitmap.recycle();
        }
        return target;
    }

    /**
     * add for resize bitmap
     */
    private static Bitmap.Config getConfig(Bitmap bitmap) {
        Bitmap.Config config = bitmap.getConfig();
        if (config == null) {
            config = Bitmap.Config.ARGB_8888;
        }
        return config;
    }

    /* SPRD:Add for bug 500764 @{ */
    public static String getType(Uri uri, Context context) {
        Uri[] table = new Uri[] {
                Images.Media.EXTERNAL_CONTENT_URI, Video.Media.EXTERNAL_CONTENT_URI
        };
        String type = null;
        if (uri.getScheme().equals("file")) {
            String path = uri.getEncodedPath();
            if (path != null) {
                path = Uri.decode(path);
                Cursor cur = null;
                ContentResolver cr = context.getContentResolver();
                StringBuffer buff = new StringBuffer();
                buff.append("(").append(DATA).append("=")
                        .append("'" + path + "'").append(")");
                for (Uri i : table) {
                    cur = cr.query(i, new String[] {MIME_TYPE}, buff.toString(), null, null);
                    for (cur.moveToFirst(); !cur.isAfterLast();) {
                        type = cur.getString(cur.getColumnIndex(MIME_TYPE));
                        cur.moveToNext();
                        break;
                    }
                }
                cur.close();
                return type;
            } else {
                return null;
            }
        } else {
            return context.getContentResolver().getType(uri);
        }
    }
    /*SPRD: @}*/
}
