/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.thundersoft.advancedfilter;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import com.android.ex.camera2.portability.Size;
import android.graphics.Bitmap;

import com.ucamera.ucam.modules.utils.MagiclensCallback;

public class TsAdvancedFilterGLView extends GLSurfaceView {
    private TsAdvancedFilterNativeRender mRender;

    public TsAdvancedFilterGLView(Context context) {
        super(context);
        init(context);
    }

    public TsAdvancedFilterGLView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    /*
     * init
     * @see android.opengl.GLSurfaceView
     */
    private void init(Context context) {
        this.setEGLContextClientVersion(2);// @see android.opengl.GLSurfaceView
        this.mRender = new TsAdvancedFilterNativeRender(this);
        this.setRenderer(this.mRender);//@see android.opengl.GLSurfaceView
        this.setZOrderMediaOverlay(true);//@see android.opengl.GLSurfaceView
        this.setRenderMode(RENDERMODE_WHEN_DIRTY);//@see android.opengl.GLSurfaceView
    }

    /*
     * Set preview frame size
     *
     * @param size preview YUV frame size
     *
     * Can be called in other thread or other local
     */
    public void setPreviewSize(Size size) {
        if (mRender != null && size != null) {
            mRender.setPreviewSize(size.width(), size.height());
        }
    }

    /*
     * Set picture size
     * @param size picture size
     *
     * Can be called in other thread or other local
     */
    public void setPictureSize(Size size) {
        if (mRender != null && size != null) {
            mRender.setPictureSize(size.width(), size.height());
        }
    }

    /*
     * Change filter type
     * @param effect  filter type
     */
    public boolean setEffectType(final int effect) {
        if (mRender != null) {
            mRender.setEffectType(effect);
        }
        return true;
    }

    /*
     * take picture
     * @param fileName The picture saved path and fine name
     */
    public void takePicture(String fileName) {
        if (mRender != null) {
            mRender.takePicture(fileName);
            requestRender();
        }
    }
    /*
     * Set preview YUV frame data
     * @param yuvBuffer  From camera preview YUV frame data
     *
     * Can be called in other thread or other local
     */
    public void setPreviewData(byte[] yuvBuffer) {
        if (mRender != null) {
            if (mRender.setPreviewData(yuvBuffer)) {
                /*
                 * Receive YUV frame data from camera, and notify render thread to loop.
                 *
                 * According to android.opengl.GLSurfaceView.setRenderMode(RENDERMODE_WHEN_DIRTY).
                 *
                 * @see android.opengl.GLSurfaceView
                 */
                requestRender();
            }
        }
    }

    public Bitmap getPreviewData() {
        if (mRender != null) {
            return mRender.getPreviewData();
        }
        return null;
    }

    public void setMagiclensCallback(MagiclensCallback callback) {
        if (mRender != null) {
            mRender.setMagiclensCallback(callback);
        }
    }

}
