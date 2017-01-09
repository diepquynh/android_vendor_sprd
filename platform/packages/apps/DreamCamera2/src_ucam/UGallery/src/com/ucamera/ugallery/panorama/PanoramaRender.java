/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.panorama;

import java.nio.IntBuffer;
import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.opengl.GLUtils;
import android.opengl.GLSurfaceView.Renderer;
import android.util.FloatMath;
import android.view.GestureDetector;
import android.view.MotionEvent;

import com.ucamera.ugallery.panorama.PanoramaTexture;

public class PanoramaRender extends GLSurfaceView implements Renderer {

    public static final float RADIUS = 8f;

    public static final float LENGTH = 3.14f;

    public static final int PRECISION = 30;

    public static final float EYE_ANGLE = 60;

    public static final float MIN_EYE_Z = 0f;

    public static final float MAX_EYE_Z = RADIUS - 2f;

    private PanoramaTexture pTexture;

    private GestureDetector mGestureDetector;

    private float mEyeZ = 0;

    private float mEyeY = 0;

    private float minEyeY = -0.02f;

    private float maxEyeY = 0.02f;

    private float image_angle = 360;

    private float rotation = 0.0f;

    private ArrayList<Texture> textures;

    private PanoramaManager[] managers;

    private int textureCount;

    private GL11 mGl = null;

    public PanoramaRender(Context context, Bitmap bitmap, int nAngle) throws OutOfMemoryError{
        super(context);
        image_angle = nAngle;
        pTexture = new PanoramaTexture(context, bitmap, image_angle);
        textures = pTexture.getTextureList();
        textureCount = textures.size();
        managers = new PanoramaManager[textureCount];

        for (int i = 0; i < textureCount; i++) {
            PanoramaManager manager = new PanoramaManager(RADIUS, LENGTH,
                    PRECISION, textures.get(i));
            managers[i] = manager;
        }

        setRenderer(this);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        mGestureDetector = new GestureDetector(context, new MyGestureListener());
    }

    public void onDrawFrame(GL10 gl1) {
        /**
         * FIX BUG: 435
         * BUG CAUSE: i don't know why
         * FIX COMMENT: add this will cause Ucam exiting
         * Date: 2012-01-18
         */
//        if (mPausing) {
//            return ;
//        }

        GL11 gl = (GL11) gl1;

        gl.glClear(GL11.GL_COLOR_BUFFER_BIT | GL11.GL_DEPTH_BUFFER_BIT);

        gl.glMatrixMode(GL11.GL_MODELVIEW);

        gl.glLoadIdentity();

        GLU.gluLookAt(gl, 0, mEyeY, mEyeZ, 0, 0, mEyeZ + 1, 0, 1, 0);

        gl.glRotatef(-rotation, 0, -1.0f, 0);

        for (int i = 0; i < textureCount; i++) {
            managers[i].doDraw(gl);
        }

    }
    // CID 109306 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // int screenWidtdh;
    int screenHeight;

    public void onSurfaceChanged(GL10 gl1, int width, int height) {
        mGl = (GL11) gl1;
        // CID 109306 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // screenWidtdh = width;
        screenHeight = height;
        float ratio = (float) width / height;
        mGl.glViewport(0, 0, width, height);
        mGl.glMatrixMode(GL11.GL_PROJECTION);
        mGl.glLoadIdentity();
        GLU.gluPerspective(mGl, EYE_ANGLE, ratio, 1, 100);
        mGl.glMatrixMode(GL11.GL_MODELVIEW);

        GLU.gluLookAt(mGl, 0, 0, 0, 1, 0, 0, 0, 1, 0);
        mGl.glLoadIdentity();
    }

    public void onSurfaceCreated(GL10 gl1, EGLConfig config) {
        mGl = (GL11) gl1;
        mGl.glDisable(GL11.GL_DITHER);
        mGl.glShadeModel(GL11.GL_SMOOTH);
        mGl.glClearDepthf(1.0f);
        mGl.glEnable(GL11.GL_DEPTH_TEST);
        mGl.glDepthFunc(GL11.GL_LEQUAL);

        mGl.glEnable(GL11.GL_CULL_FACE);
        mGl.glFrontFace(GL11.GL_CCW);
        mGl.glCullFace(GL11.GL_FRONT);

        mGl.glHint(GL11.GL_PERSPECTIVE_CORRECTION_HINT, GL11.GL_FASTEST);

        mGl.glLoadIdentity();
        mGl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        mGl.glEnable(GL11.GL_POINT_SMOOTH);
        mGl.glEnable(GL11.GL_LINE_SMOOTH);
        mGl.glHint(GL11.GL_POLYGON_SMOOTH_HINT, GL11.GL_NICEST);
        mGl.glHint(GL11.GL_POINT_SMOOTH_HINT, GL11.GL_NICEST);
        mGl.glHint(GL11.GL_LINE_SMOOTH_HINT, GL11.GL_NICEST);
        for (int i = 0; i < textures.size(); i++) {
            createTexture(mGl, textures.get(i));
        }
    }

    private void createTexture(GL10 gl1, Texture texture) {
        GL11 gl = (GL11) gl1;
        IntBuffer intBuffer = IntBuffer.allocate(1);
        gl.glGenTextures(1, intBuffer);
        int mTextureID = intBuffer.array()[0];
        texture.setId(mTextureID);
        gl.glBindTexture(GL11.GL_TEXTURE_2D, mTextureID);

        GLUtils.texImage2D(GL11.GL_TEXTURE_2D, 0, texture.getBitmap(), 0);

        gl.glTexParameterf(GL11.GL_TEXTURE_2D, GL11.GL_TEXTURE_MAG_FILTER,
                GL11.GL_NEAREST);
        gl.glTexParameterf(GL11.GL_TEXTURE_2D, GL11.GL_TEXTURE_MIN_FILTER,
                GL11.GL_NEAREST);

        gl.glTexParameterf(GL11.GL_TEXTURE_2D, GL11.GL_TEXTURE_WRAP_S,
                GL11.GL_CLAMP_TO_EDGE);
        gl.glTexParameterf(GL11.GL_TEXTURE_2D, GL11.GL_TEXTURE_WRAP_T,
                GL11.GL_CLAMP_TO_EDGE);

        gl.glTexEnvf(GL11.GL_TEXTURE_ENV, GL11.GL_TEXTURE_ENV_MODE,
                GL11.GL_REPLACE);

        gl.glShadeModel(GL11.GL_SMOOTH_POINT_SIZE_RANGE);

    }

    public static final int NONE = 0;
    public static final int DRAG = 1;
    public static final int ZOOM = 2;

    private float mPrevTouchPosX;
    private float mPrevTouchPosY;
    private float mPrevDist;
    private int mTouchMode;

    public boolean onTouchEvent(MotionEvent event) {
        if (mGl == null) {
            return false;
        }
        mGestureDetector.onTouchEvent(event);
        int x = (int) event.getX();
        int y = (int) event.getY();
        int pCount = event.getPointerCount();
        if (pCount > 2)
            return true;
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN :
                mPrevTouchPosX = x;
                mPrevTouchPosY = y;
                mTouchMode = DRAG;
                isStop = true;
                break;
            case MotionEvent.ACTION_UP :
                mPrevTouchPosX = x;
                mPrevTouchPosY = y;
                mTouchMode = NONE;
                break;
            case MotionEvent.ACTION_POINTER_DOWN :
                mPrevDist = spacing(event);
                mTouchMode = ZOOM;
                break;
            case MotionEvent.ACTION_POINTER_UP :
                mPrevDist = spacing(event);
                mTouchMode = NONE;
                break;
            case MotionEvent.ACTION_MOVE :
                if (mTouchMode == DRAG) {
                    float deltaX = mPrevTouchPosX - x;
                    touchMoveX(deltaX);
                    touchMoveY(y);
                    mPrevTouchPosX = x;
                }
                else if (mTouchMode == ZOOM) {
                    touchMoveZ(event);
                }
                break;
        }
        return true;
    }

    private float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void touchMoveX(float deltaX) {
        float n = 2f * (RADIUS - mEyeZ)
                * (float) Math.tan(EYE_ANGLE / 2.0f / 180 * Math.PI) * deltaX
                / (screenHeight * RADIUS);
        n = (float) (n / Math.PI * 180);
        this.rotation(n);
        this.requestRender();
    }

    private void touchMoveY(float posY) {
        float deltaY = mPrevTouchPosY - posY;
        if (Math.abs(deltaY) < 5)
            return;
        float min = minEyeY * (mEyeZ + 1);
        float max = maxEyeY * (mEyeZ + 1);
        mEyeY += (deltaY * 0.005f * (mEyeZ + 1));
        if (mEyeY < min) {
            mEyeY = min;
            return;
        }
        else if (mEyeY > max) {
            mEyeY = max;
            return;
        }
        mPrevTouchPosY = posY;
        this.requestRender();
    }

    private void touchMoveZ(MotionEvent event) {
        float dist = spacing(event);
        float delta = mPrevDist - dist;
        if (Math.abs(delta) < 5)
            return;
        mEyeZ -= (delta * 0.025);
        if (mEyeZ >= MAX_EYE_Z) {
            mEyeZ = MAX_EYE_Z;
            return;
        }
        if (mEyeZ <= MIN_EYE_Z) {
            mEyeZ = MIN_EYE_Z;
            return;
        }
        mPrevDist = dist;
        this.requestRender();
    }

    public static final float ACCELERATION = 4f;

    private synchronized void compute(float velocityX, float posX) {
        float a = velocityX > 0 ? -1 * ACCELERATION : ACCELERATION;
        while (velocityX * a < 0) {
            if (isStop)
                break;
            velocityX += a;
            try {
                Thread.sleep(4);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            touchMoveX(-velocityX * 0.005f);
        }

    }

    class MyThread extends Thread {
        private float mVeloctity;
        private float mX;

        public MyThread(float velocity, float x) {
            mVeloctity = velocity;
            mX = x;
        }

        public void run() {
            compute(mVeloctity, mX);
        }
    }

    private boolean isStop = false;

    private class MyGestureListener
            extends
                GestureDetector.SimpleOnGestureListener {

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
                float velocityY) {
            if (e1 == null || e2 == null) {
                return false;
            }
            isStop = false;
            MyThread t = new MyThread(velocityX, e1.getX());
            t.start();
            return true;
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2,
                float distanceX, float distanceY) {
            return true;
        }

        @Override
        public void onShowPress(MotionEvent e) {

        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            return true;
        }
    }

    public void setRotation(float rotation) {
        this.rotation = rotation;
    }

    public void rotation(float rotation) {
        this.rotation += rotation;
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    public void onPause() {
        super.onPause();
        if (pTexture != null) {
            pTexture.destroy();
        }
    }
}
