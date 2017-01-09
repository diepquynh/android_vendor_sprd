package com.sprd.sprdlauncher2.effect3d;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.view.MotionEvent;
import android.view.GestureDetector;
import android.util.Log;

public class SprdSurfaceView extends GLSurfaceView {
    private static String TAG = "SprdSurfaceView";

    SprdRenderer mRenderer;
    private final GestureDetector mGestureDetector;
    private boolean isMoving = false;

    public SprdSurfaceView(Context context) {
        super(context);

        getHolder().setFormat(PixelFormat.TRANSLUCENT);
        setZOrderOnTop(true);
        setEGLContextClientVersion(2);
        setEGLConfigChooser(new SprdFSAAConfigChooser(8, 8, 8, 8, 16, 0));
        if (null == mRenderer) {
            mRenderer = new SprdRenderer();
            setRenderer(mRenderer);
        }
        mGestureDetector = new GestureDetector(context, new SprdGestureListener(mRenderer));
    }

    private static class SprdFSAAConfigChooser implements GLSurfaceView.EGLConfigChooser {
        private static int EGL_OPENGL_ES2_BIT = 4;
        private static int[] s_configAttribs2 = {
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 16,
                EGL10.EGL_STENCIL_SIZE, 0,
                EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL10.EGL_BUFFER_SIZE, 16,
                EGL10.EGL_SAMPLE_BUFFERS, 1,
                EGL10.EGL_SAMPLES, 4,
                EGL10.EGL_NONE
        };

        public SprdFSAAConfigChooser(int r, int g, int b, int a, int depth, int stencil) {
            mRedSize = r;
            mGreenSize = g;
            mBlueSize = b;
            mAlphaSize = a;
            mDepthSize = depth;
            mStencilSize = stencil;
        }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            int[] num_config = new int[1];
            egl.eglChooseConfig(display, s_configAttribs2, null, 0, num_config);

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No configs match configSpec");
            }
            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, s_configAttribs2, configs, numConfigs, num_config);
            return chooseConfig(egl, display, configs);
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
            for (EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config, EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config, EGL10.EGL_STENCIL_SIZE, 0);
                if (d >= mDepthSize && s >= mStencilSize) {
                    int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
                    int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                    int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                    int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);
                    if ((r == mRedSize) && (g == mGreenSize) && (b == mBlueSize)
                            && (a == mAlphaSize)) {
                        return config;
                    }
                }
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config,
                int attribute, int defaultValue) {
            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        // Subclasses can adjust these values:
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;

        private int[] mValue = new int[1];
    }

    public void setCallBack(Sprd3DRenderable.Sprd3DCallback cb) {
        mRenderer.setCallBack(cb);
    }

    public void setRotateLimit(boolean limitLeft, boolean limitRight) {
        mRenderer.setRotateLimit(limitLeft, limitRight);
    }

    public void setCubeRate(float rate) {
        mRenderer.setCubeRate(rate);
    }

    public void setTransferType(int type) {
        mRenderer.setTransferType(type);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
//        Log.e(TAG, "-----onTouchEvent: " + event.getAction());
        if (mGestureDetector.onTouchEvent(event)) {
            isMoving = true;
            return true;
        }
        if (mRenderer.IsFlingOrAnimation()) {
            return true;
        }
        final float x = event.getX();
        final float y = event.getY();
        if (!isMoving && event.getAction() != MotionEvent.ACTION_UP) {
//        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            queueEvent(new Runnable() {
                public void run() {
                    mRenderer.onActionDown(x / getWidth(), y / getHeight());
                }
            });
            isMoving = true;
//            Log.e(TAG, "-----onTouchEvent: isMoving " + event.getAction());
            return true;
        }
        if(event.getAction() == MotionEvent.ACTION_UP) {
            queueEvent(new Runnable() {
                public void run() {
                    mRenderer.onActionUp();
                }
            });
//            Log.w(TAG, "Action_Up");
            return true;
        }
        if (event.getAction() == MotionEvent.ACTION_MOVE) {
            queueEvent(new Runnable() {
                public void run() {
                    mRenderer.onActionMove(x / getWidth(), y / getHeight());
                }
            });
//            Log.w(TAG, "Action_Move");
        }
//        Log.e(TAG, "-----onTouchEvent: " + event.getAction());
        return true;
    }

    @Override
    protected void onAttachedToWindow() {
        Log.w(TAG, "==========onAttachedToWindow==========");
        isMoving = false;
        super.onAttachedToWindow();
    }

    @Override
    protected void onDetachedFromWindow() {
//        Log.w(TAG, "==========onDetachedFromWindow==========");
        super.onDetachedFromWindow();
    }
    
    @Override
    public void onPause() {
        super.onPause();
        isMoving = false;
        Log.w(TAG, "onPause");
    }

    @Override
    public void onResume() {
        isMoving = false;
        super.onResume();
        Log.w(TAG, "onResume");
    }

    public void setBitmaps(Bitmap[] bitmaps) {
        mRenderer.setBitmaps(bitmaps);
    }

    private static class SprdRenderer implements GLSurfaceView.Renderer {
//        private final static float FLING_VELOCITY_LIMIT = 400.0F;
        private float[] mProjMatrix = new float[16];
        private float[] mMMatrix = new float[16];
        private float[] mVMatrix = new float[16];

        private boolean mLimitLeft;
        private boolean mLimitRight;
        private float mRatio = 1.0f;
        private int mTransferType;
        private Sprd3DRenderable.Sprd3DCallback mCallback = null;

        private Sprd3DRenderable mModle;
        public Bitmap[] mTextureBitmap;
        public boolean mIsTextureModified = false;

        protected void checkTexture() {
            if(mIsTextureModified) {
                mModle.genTextures(mTextureBitmap);
                mIsTextureModified = false;
            }
        }

        public void setRotateLimit(boolean limitLeft, boolean limitRight) {
            mLimitLeft = limitLeft;
            mLimitRight = limitRight;
        }

        public boolean IsFlingOrAnimation() {
            if (null == mModle) {
                return false;
            }
            return mModle.IsFlingOrAnimation();
        }

        public void onFling(float velocityX, float velocityY) {
            if (null != mModle) {
                mModle.onFling(velocityX, velocityY);
            } else if (null != mCallback) {
                mCallback.onPreClose(0);
                mCallback.onClosing(0);
                mCallback.onClose(0);
            }
        }

        @Override
        public void onDrawFrame(GL10 gl) {
//            Log.w(TAG, "--------onDrawFrame()--------");
            GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);
            checkTexture();
            mModle.draw(mMMatrix, mVMatrix, mProjMatrix);
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GLES20.glViewport(0, 0, width, height);
            float ratio = (float) width / height;
            Log.w(TAG, "ratio: " + ratio + ", " + width + ", " + height);
            Matrix.frustumM(mProjMatrix, 0, -ratio, ratio, -1, 1, 3, 10);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
//            Log.w(TAG, "--------onSurfaceCreated()--------");
            GLES20.glEnable(GLES20.GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

            Matrix.setLookAtM(mVMatrix, 0, 0, 0f, 5.0f, 0f, 0f, 0f, 0f, 1.0f, 0.0f);
            if (mTransferType == 7) {
                mModle = new Sprd3DCube(mRatio, 4.0f/3, 0.5f);
            } else {
                mModle = new Sprd3DPlexiglass(mRatio, 5/3.0f, 1.0f);
            }
            mModle.setRotateLimit(mLimitLeft, mLimitRight);
            mModle.setCallback(mCallback);
        }

        public void onActionDown(float x, float y) {
            if (null != mModle) {
                Log.w(TAG, "onActionDown " + x);
                mModle.onBeginMoving(x, y);
            }
        }

        public void setBitmaps(Bitmap[] bitmaps) {
            mTextureBitmap = bitmaps;
            mIsTextureModified = true;
        }

        public void setCubeRate(float rate) {
            mRatio = rate;
        }

        public void onActionMove(float x, float y) {
            if (null != mModle) {
                mModle.onMoving(x, y);
            }
        }

        public void onActionUp() {
            if (null != mModle) {
                mModle.onStopMoving();
            } else if (null != mCallback) {
                mCallback.onPreClose(0);
                mCallback.onClosing(0);
                mCallback.onClose(0);
            }
        }

        public void setTransferType(int type) {
            mTransferType = type;
        }

        public void setCallBack(Sprd3DRenderable.Sprd3DCallback cb) {
            mCallback = cb;
        }
    }

    private class SprdGestureListener implements GestureDetector.OnGestureListener {
        private final static String TAG = "SprdGestureListener";
        private SprdRenderer mRenderer;
        public SprdGestureListener(SprdRenderer renderer) {
            mRenderer = renderer;
        }
        @Override
        public boolean onDown(MotionEvent event) {
            // TODO Auto-generated method stub
//            Log.w(TAG, "-----onDown-----");
            return false;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, final float velocityX, final float velocityY) {
            if (e1 != null && e2!=null) {
                final float x1 = e1.getX();
                final float x2 = e2.getX();
                Log.w(TAG, "-----onFling-----VelocityX: " + velocityX + ", VelocityY: " + velocityY + ", X1: " + x1 + ", X2 : " + x2);
                if (x1 - x2 < 300 && x1 - x2 > -300) {
                    Log.w(TAG, "Fling is too small, ignore the fling.");
                }
            } else {
                Log.w(TAG, "-----onFling-----VelocityX: " + velocityX + ", VelocityY: " + velocityY);
            }
            if (mRenderer.IsFlingOrAnimation()) {
                return true;
            }
            queueEvent(new Runnable() {
                @Override
                public void run() {
                    mRenderer.onFling(velocityX, velocityY);
                }
            });
            return true;
        }

        @Override
        public void onLongPress(MotionEvent event) {
            // TODO Auto-generated method stub
//            Log.w(TAG, "-----onLongPress-----");
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            // TODO Auto-generated method stub
//            Log.w(TAG, "-----onScroll-----");
            return false;
        }

        @Override
        public void onShowPress(MotionEvent event) {
            // TODO Auto-generated method stub
//            Log.w(TAG, "-----onShowPress-----");
        }

        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            // TODO Auto-generated method stub
//            Log.w(TAG, "-----onSingleTapUp-----");
            return false;
        }
        
    }
}
