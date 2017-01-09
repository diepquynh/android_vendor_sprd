package com.ucamera.uphoto.gpuprocess;
/**
 * Copyright (C) 2013 Thundersoft Corporation
 * All rights Reserved
 */

import android.graphics.Bitmap;
import android.util.Log;

import com.ucamera.uphoto.gpuprocess.jni.GpuProcessJni;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
public class EglWorker {
    private EGLContext mEglContext;
    private EGLDisplay mEglDisplay;
    private EGLSurface mEGLSurface;
    private EGL10 mEgl;
    private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    /* SPRD: CID 109255 (#1 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD) @{
    private int nWidth,nHeight;
    */
    private static final int EGL_OPENGL_ES2_BIT = 4;
    /* SPRD: CID 109255 (#3 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD) @{
    private String mCurrentType = null;
    */
    public EglWorker()
    {
        int []version = new int[2];
        int[] num_config = new int[1];
        EGLConfig[] configs = new EGLConfig[1];
        int[] configSpec = {
                EGL10.EGL_RED_SIZE,            8,
                EGL10.EGL_GREEN_SIZE,          8,
                EGL10.EGL_BLUE_SIZE,           8,
                EGL10.EGL_ALPHA_SIZE,          8,
                EGL10.EGL_SURFACE_TYPE,     EGL10.EGL_WINDOW_BIT,
                EGL10.EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES2_BIT,
                EGL10.EGL_NONE
        };
        mEgl = (EGL10) EGLContext.getEGL();

        mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        mEgl.eglInitialize(mEglDisplay, version);
        checkEglError("After eglInitialize", mEgl);

        mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, 1, num_config);
        checkEglError("After eglChooseConfig", mEgl);

        EGLConfig mEglConfig = configs[0];

        int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
        mEglContext = mEgl.eglCreateContext( mEglDisplay,
                mEglConfig,
                EGL10.EGL_NO_CONTEXT,
                attrib_list);
        checkEglError("After eglCreateContext", mEgl);

        int[] glattribList = new int[] {
                EGL10.EGL_WIDTH, 1,
                EGL10.EGL_HEIGHT, 1,
                EGL10.EGL_NONE
        };
        mEGLSurface = mEgl.eglCreatePbufferSurface(mEglDisplay, mEglConfig,  glattribList);
        mEgl.eglMakeCurrent(mEglDisplay, mEGLSurface, mEGLSurface, mEglContext);
        checkEglError("After eglMakeCurrent", mEgl);
        GpuProcessJni.init();
    }

    public void setCurrentType(String type, String fragment) {
        if (fragment == null || type == null) {
            return ;
        }

        /* SPRD: CID 109255 (#3 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD) @{
        mCurrentType = type;
        */
        GpuProcessJni.setType(type, fragment);
    }

    public void setImage(Bitmap image)
    {

        /* SPRD: CID 109255 (#3 of 3): UrF: Unread field (FB.URF_UNREAD_FIELD) @{
        nWidth = image.getWidth();
        nHeight= image.getHeight();
        */
        /* @} */

        mEgl.eglMakeCurrent(mEglDisplay, mEGLSurface, mEGLSurface,
                mEglContext);
        checkEglError("After eglMakeCurrent", mEgl);
    }

    public void finish()
    {
        mEgl.eglMakeCurrent(mEglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE,
                EGL10.EGL_NO_CONTEXT);
        checkEglError("After eglMakeCurrent", mEgl);
        GpuProcessJni.uninit();
        mEgl.eglDestroyContext(mEglDisplay, mEglContext);
        mEgl.eglDestroySurface(mEglDisplay, mEGLSurface);
        mEgl.eglTerminate(mEglDisplay);
    }

    public int process(Bitmap srcBitmap, Bitmap outBitmap, Bitmap[] texture) {
        long start = System.currentTimeMillis();
        mEgl.eglMakeCurrent(mEglDisplay, mEGLSurface, mEGLSurface,
                mEglContext);
        checkEglError("After eglMakeCurrent", mEgl);

        int nRes = GpuProcessJni.gpuProcess(srcBitmap, outBitmap, texture);

        long end = System.currentTimeMillis();
        Log.i("ImageEditControlActivity getImage cost", "" + (end - start));
        return nRes;
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e("checkEglError", String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }
}
