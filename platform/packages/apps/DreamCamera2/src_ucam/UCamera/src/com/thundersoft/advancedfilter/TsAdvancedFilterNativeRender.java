/**
* Copyright (C) 2010,2012 Thundersoft Corporation
* All rights Reserved
*/
package com.thundersoft.advancedfilter;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.hardware.Camera;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.MagiclensCallback;

import java.io.FileOutputStream;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class TsAdvancedFilterNativeRender implements GLSurfaceView.Renderer {
    private static final String TAG = "TsAdvancedFilterNativeRender";
    private GLSurfaceView mGLView;
    private byte []mYUVBuffer;
    private int mYUVFrameWidth = 0;
    private int mYUVFrameHeight = 0;
    private Object sync = new Object();
    private int mSurfaceWidth = 0;
    private int mSurfaceHeight = 0;
    private int mPicWidth = 0;
    private int mPicHeight = 0;
    private String mSavePicturePath;
    private boolean mIsTackPic;

    private MagiclensCallback mCallback;
    /* SPRD: Fix bug 574388 the preview screen will be black in filter to switch @{ */
    private final long WAIT_TIMEOUT_MS = 3000;
    private final long WAIT_INTERVAL_MS = 20;
    private boolean mRenderOnceFlg = false;
    private Bitmap mBitmap = null;
    /* @} */

    public TsAdvancedFilterNativeRender(GLSurfaceView glSurfaceView) {
        mGLView = glSurfaceView;
    }

    /*
     * render loop, in render thread
     *
     * @see android.opengl.GLSurfaceView
     * @see @see android.opengl.GLSurfaceView.Renderer
     */
    @Override
    public void onDrawFrame(GL10 glUnused) {
        if (mYUVBuffer == null) {
            if (mRenderOnceFlg) {
                synchronized (this) {
                    mRenderOnceFlg = false;
                    this.notifyAll();
                }
            }
            return;
        }
        int cameraFacing = TsAdvancedFilterNative.CAMERA_FACING_BACK;
        if(FilterTranslationUtils.getInstance().getCameraId()==Camera.CameraInfo.CAMERA_FACING_FRONT){
            cameraFacing = TsAdvancedFilterNative.CAMERA_FACING_FRONT;
        }

        synchronized (sync) {
            /* SPRD: Fix bug 574388 the preview screen will be black in filter to switch @{ */
            if (mRenderOnceFlg) {
                TsAdvancedFilterNative.onDrawFrame(mYUVBuffer, mYUVFrameWidth, mYUVFrameHeight, FilterTranslationUtils.getInstance().getMMatrix(),
                        cameraFacing, mPicWidth, mPicHeight);

                initializePreviewBitmap();
                synchronized (this) {
                    mRenderOnceFlg = false;
                    this.notifyAll();
                }
            } else {
                //long start = System.currentTimeMillis();
                TsAdvancedFilterNative.onDrawFrame(mYUVBuffer, mYUVFrameWidth, mYUVFrameHeight, FilterTranslationUtils.getInstance().getMMatrix(),
                        cameraFacing, mPicWidth, mPicHeight);
                //LogUtils.debug(TAG, "onDrawFrame cost: " + (System.currentTimeMillis() - start));
            }
            /* @} */
        }
        //when take picture
        savePicture();
    }

    /*
     * surface changed, in render thread
     *
     * @see android.opengl.GLSurfaceView
     * @see @see android.opengl.GLSurfaceView.Renderer
     */
    @Override
    public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        GLES20.glViewport(0, 0, width, height);
        mSurfaceWidth = width;
        mSurfaceHeight = height;
        LogUtils.debug(TAG, "onSurfaceChanged(): surface.WH is (" + mSurfaceWidth + ", " + mSurfaceHeight + ");");
        mYUVBuffer = null;
        TsAdvancedFilterNative.surfaceChanged(width, height);
        FilterTranslationUtils.getInstance().setMMatrix();
    }

    /*
     * surface created, in render thread
     *
     * @see android.opengl.GLSurfaceView
     * @see @see android.opengl.GLSurfaceView.Renderer
     */
    @Override
    public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {

    }

    /*
     * Set preview YUV frame data
     *
     * @param yuvBuffer  From camera preview YUV frame data
     */
    public boolean setPreviewData(byte[] yuvBuffer) {
        if (yuvBuffer == null) {
            return false;
        }

        synchronized (sync) {
            if(mYUVBuffer == null || (mYUVBuffer.length != yuvBuffer.length)){
                mYUVBuffer = new byte[yuvBuffer.length];
            }
            System.arraycopy(yuvBuffer, 0, mYUVBuffer, 0, yuvBuffer.length);
        }
        return true;
    }

    /*
     * Set preview frame size
     * @param width preview YUV frame width
     * @param height preview YUV frame height
     *
     * Can be called in other thread or other local
     */
    public void setPreviewSize(int width, int height) {
        /*
        //reverse the width and height as taked picture size
        if(height != 0 && height != mPicWidth) {
            mPicWidth = height;       //taked picture width
            mYUVFrameHeight = height; //YUV frame height
        }
        if(width != 0 && width != mPicHeight) {
            mPicHeight = width;       //taked picture height
            mYUVFrameWidth = width;   //YUV frame width
        }
        */
        mYUVFrameWidth = width;   //YUV frame width
        mYUVFrameHeight = height; //YUV frame height
    }

    /*
     * Set picture size
     * @param width picture width
     * @param height picture height
     *
     * Can be called in other thread or other local
     */
    public void setPictureSize(int width, int height) {
        mPicWidth=width;
        mPicHeight=height;
    }

    /*
     * take picture
     * @param fileName The picture saved path and fine name
     */
    public void takePicture(String fileName) {
        mSavePicturePath=fileName;
        mIsTackPic=true;
    }
    /*
     * When take picture called.
     *
     * Must be called in render thread
     */
    public void savePicture() {
        if (mIsTackPic) {
            saveBuffer(0, 0, mSurfaceWidth, mSurfaceHeight);
            mIsTackPic=false;
        }
    }

    /*
     * When take picture called
     *
     * Copy rgba buffer form GLES framebuffer and compatible with Android bitmap
     * @param x  copy rgba pixels offset x. general is 0
     * @param y  copy rgba pixels offset y. general is 0
     * @param w  copy rgba pixels width. general is surface width
     * @param h  copy rgba pixels height. general is surface height
     * @see android.opengl.GLES20
     *
     * Must be called in render thread
     */
    private void saveBuffer(int x, int y, int w, int h) {
        int b[] = new int[w * (y + h)];
        int bt[] = new int[w * h];
        IntBuffer ib = IntBuffer.wrap(b);
        ib.position(0);
        GLES20.glReadPixels(x, 0, w, y + h, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, ib);//@see android.opengl.GLES20

        for (int i = 0, k = 0; i < h; i++, k++) {
            // OpenGLES bitmap is incompatible with Android bitmap
            // and so, some corrections need to be done.
            for (int j = 0; j < w; j++) {
                int pix = b[i * w + j];
                int pb = (pix >> 16) & 0xff;
                int pr = (pix << 16) & 0x00ff0000;
                int pix1 = (pix & 0xff00ff00) | pr | pb;
                bt[(h - k - 1) * w + j] = pix1;
            }
        }

        final String picPath=mSavePicturePath;

        int picWidth = mYUVFrameHeight;
        int picHeight = mYUVFrameWidth;

        Bitmap bm = getPictureBitmap(w, h, picWidth, picHeight, bt);

        ///////Owen.Xiong start///////
        if(bm == null || mCallback == null) {
            return;
        }

        mCallback.captureComplete(bm, picWidth, picHeight);
        ///////////END///////////////

        /*
        if(bm!=null){
            saveBitmapToFile(bm,picPath);
            bm.recycle();
            mGLView.post(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(mGLView.getContext(), "Success,Path:"+picPath, Toast.LENGTH_LONG).show();
                }
            });
        }
        */
    }

    /* SPRD: Fix bug 574388 the preview screen will be black in filter to switch @{ */
    private void initializePreviewBitmap() {
        int x = 0;
        int y = 0;
        int w = mSurfaceWidth;
        int h = mSurfaceHeight;
        int b[] = new int[w * (y + h)];
        int bt[] = new int[w * h];
        IntBuffer ib = IntBuffer.wrap(b);
        ib.position(0);
        GLES20.glReadPixels(x, 0, w, y + h, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, ib);//@see android.opengl.GLES20

        for (int i = 0, k = 0; i < h; i++, k++) {
            // OpenGLES bitmap is incompatible with Android bitmap
            // and so, some corrections need to be done.
            for (int j = 0; j < w; j++) {
                int pix = b[i * w + j];
                int pb = (pix >> 16) & 0xff;
                int pr = (pix << 16) & 0x00ff0000;
                int pix1 = (pix & 0xff00ff00) | pr | pb;
                bt[(h - k - 1) * w + j] = pix1;
            }
        }
        if (mBitmap != null && !mBitmap.isRecycled()) {
            mBitmap.recycle();
            mBitmap = null;
        }
        mBitmap = Bitmap.createBitmap(bt, w, h, Config.ARGB_8888);
    }

    public Bitmap getPreviewData() {
        /*
         * render once to trigger preview bitmap initialization which should be done
         * in process of TsAdvancedFilterNativeRender.onDrawFrame @{
         */
        mRenderOnceFlg = true;
        mGLView.requestRender();
        /* @} */

        long startMs = System.currentTimeMillis();
        synchronized (this) {
            while (mRenderOnceFlg) {
                try {
                    this.wait(WAIT_INTERVAL_MS);

                    if (System.currentTimeMillis() - startMs > WAIT_TIMEOUT_MS) {
                        Log.w(TAG, "Timeout waiting");
                        break;
                    }
                } catch (InterruptedException ex) {
                }
            }
        }
        Log.i(TAG, "getPreviewData cost: " + (System.currentTimeMillis() - startMs));
        return mBitmap;
    }
    /* @} */

    /*
     * Change filter type
     * @param effectType  filter type
     *
     * Can be called in other thread or other local
     */
    public void setEffectType(int effectType) {
        TsAdvancedFilterNative.setEffectType(effectType);// Must be call
        if(effectType == TsAdvancedFilterNative.ADVANCEDFILTER_MOVIED_COLOR){
           // Example, ADVANCEDFILTER_MOVIED_COLOR filter set parameter
            String moviedColorParam = String.format("selectedRGB=%f,%f,%f", (float)0.1, (float)0.9, (float)0.1);
            TsAdvancedFilterNative.setEffectParam(moviedColorParam);
        }else if(effectType == TsAdvancedFilterNative.ADVANCEDFILTER_DREAMLAND){
           // Example, ADVANCEDFILTER_DREAMLAND filter set parameter
            float center[] = {0.5f, 0.5f};
            float whratio = (float)mYUVFrameWidth/(float)mYUVFrameHeight;
            float radius = 0.3f;
            String dreamLandParam = String.format("center=%f,%f;whratio=%f;radius=%f",center[0],center[1], whratio, radius);
            TsAdvancedFilterNative.setEffectParam(dreamLandParam);
        }
    }

    public void setMagiclensCallback(MagiclensCallback callback) {
        mCallback = callback;
    }

    /*
     * Adjust Bitmap size to picture size
     */
    private Bitmap getPictureBitmap(int w, int h, int picWidth, int picHeight, int[] bt) {
        Bitmap bitmap = null, tempBitmap = null;
        try {
            bitmap = Bitmap.createBitmap(bt, w, h, Config.ARGB_8888);
            tempBitmap = Bitmap.createBitmap(picWidth, picHeight, Config.ARGB_8888);
        } catch (OutOfMemoryError e) {
            System.gc();
            return null;
        } catch (IllegalArgumentException e) {
            LogUtils.debug(TAG, "w= "+w +", h = "+h+", picWidth = "+picWidth+", picHeight = "+picHeight);
            return null;
        }
        Canvas canvas = new Canvas(tempBitmap);
        canvas.drawBitmap(bitmap, null, new Rect(0, 0, picWidth, picHeight), new Paint());
        bitmap.recycle();
        return tempBitmap;
    }

    /*
     * save bitmap to file
     */
    private void saveBitmapToFile(Bitmap bmp, String fileName){
        FileOutputStream fos=null;
        try {
            fos=new FileOutputStream(fileName);
            bmp.compress(Bitmap.CompressFormat.JPEG, 100, fos);
        } catch (Exception e) {
            e.printStackTrace();
        }finally{
            try {
                if(fos!=null){
                    fos.close();
                }
            } catch (Exception e2) {
                e2.printStackTrace();
            }
        }
    }
}
