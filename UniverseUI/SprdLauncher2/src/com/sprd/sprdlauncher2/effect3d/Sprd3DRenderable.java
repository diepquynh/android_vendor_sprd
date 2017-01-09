package com.sprd.sprdlauncher2.effect3d;

import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.util.Log;

public class Sprd3DRenderable {
    private static final String TAG = "Sprd3DRenderable";
    protected static final float LIMIT_ANGLE = 20.0F;
    protected static final float FLING_VELOCITY_LIMIT = 200.0F;
    protected static final int ANIMATION_COUNT = 15;

    protected byte mPageDelta;
    protected boolean mLimitLeft;
    protected boolean mLimitRight;
    protected float mLastX;
    protected float mRotateYValue;
    protected float mTranslate;

    protected float[] mVerticesData;
    protected short[] mIndexData;
    protected float[] mColorData;
    protected int[] mTextureIDs;
    protected byte mBindTexture;
    protected FloatBuffer mVerticesBuffer;
    protected ShortBuffer mIndexBuffer;
    protected FloatBuffer mColorBuffer;

    protected int mProgram;
    protected int maPositionHandle;
    protected int muMMatrixHandle;
    protected int muVMatrixHandle;
    protected int muPMatrixHandle;

    protected float mRatio = 1.0f;
    protected float mZoom = 1.0f;
    protected float mAlpha = 1.0f;
    protected boolean mAnimation;
    protected boolean mFling;
    protected boolean mLightFling;

    public interface Sprd3DCallback {
        void onPreClose(int nextPage);
        void onClosing(int nextPage);
        void onClose(int nextPage);
    }
    protected Sprd3DCallback mCallback = null;

    public Sprd3DRenderable() {
        this(1.0f, 1.0f, 1.0f);
    }

    public Sprd3DRenderable(float ratio, float zoom, float alpha) {
        mZoom = zoom;
        mRatio = ratio;
        mAlpha = alpha;
        init();
    }

    public void setScaleRatio(float zoom) {
        mZoom = zoom;
    }

    public void setAspectRatio(float ratio) {
        mRatio = ratio;
    }

    public float getAlpha() {
        return mAlpha;
    }

    public void setAlpha(float alpha) {
        mAlpha = alpha;
    }

    public void setCallback(Sprd3DCallback callback) {
        mCallback = callback;
    }

    public void setRotateLimit(boolean limitLeft, boolean limitRight) {
        mLimitLeft = limitLeft;
        mLimitRight = limitRight;
    }

    protected void init() {
        createMesh();

        createProgram();

        handleVariables();
    }

    protected void createMesh() {
        if (null != mColorData) {
            mColorBuffer = Sprd3DUtilities.getFloatBuffer(mColorData);
        }
        mVerticesBuffer = Sprd3DUtilities.getFloatBuffer(mVerticesData);
        if (null != mIndexData) {
            mIndexBuffer = Sprd3DUtilities.getShortBuffer(mIndexData);
        }
    }

    protected void createProgram() {
        mProgram = Sprd3DUtilities.createProgram(getVertexShader(), getFragmentShader());
        if (mProgram == -1) {
            throw new RuntimeException("Could not create program");
        }
    }

    protected void handleVariables() {
        maPositionHandle = GLES20.glGetAttribLocation(mProgram, "aPosition");
        checkGlError("glGetAttribLocation");
        if (maPositionHandle == -1) {
            throw new RuntimeException("Could not get attrib location for aPosition");
        }

        muMMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMMatrix");
        checkGlError("glGetUniformLocation");
        if (muMMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uMMatrix");
        }

        muVMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uVMatrix");
        checkGlError("glGetUniformLocation");
        if (muVMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uVMatrix");
        }

        muPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uPMatrix");
        checkGlError("glGetUniformLocation");
        if (muPMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uPMatrix");
        }
    }

    protected String getVertexShader() {
        return null;
    }

    protected String getFragmentShader() {
        return null;
    }

    protected void useProgram(int programID) {
        GLES20.glUseProgram(programID);
    }

    /* In order to get animation effect, the following 5 functions should be override. */
    public void onBeginMoving(float x, float y) {
    }

    public void onMoving(float x, float y) {
    }

    public void onStopMoving() {
    }

    public void onFling(float velocityX, float velocityY) {
    }

    // The process of animation
    protected void doAnimation() {
    }

    // Fling operation is very light, or the transfer to the orientation is limited.
    protected void onLightFling(float velocityX, float velocityY) {
    }

    public boolean IsFlingOrAnimation() {
        if (mFling | mAnimation | mLightFling) {
            Log.w(TAG, "" + mFling + ", " + mAnimation + ", " + mLightFling);
        }
        return (mFling|mAnimation|mLightFling);
    }

    public void genTextures(Bitmap[] bitmapTexture) {
        int textureNum;

        textureNum = bitmapTexture.length;

        if (mTextureIDs == null) {
            mTextureIDs = new int[textureNum];
            GLES20.glGenTextures(textureNum, mTextureIDs, 0);
        } else if (mTextureIDs.length < textureNum) {
            GLES20.glDeleteTextures(mTextureIDs.length, mTextureIDs, 0);
            mTextureIDs = new int[textureNum];
            GLES20.glGenTextures(textureNum, mTextureIDs, 0);
        }
        mBindTexture = 0;
        for (int i = 0; i < textureNum; i++) {
            if (null == bitmapTexture[i]) {
                Log.w(TAG, "genTextures - bitmap is null: " + i);
                continue;
            }
            mBindTexture += (1 << i);
            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[i]);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
            GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);

//            android.util.Log.w("Sprd3DRenderable", "genTextures: " + i + ", " + bitmapTexture[i]);
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmapTexture[i], 0);
        }
        Log.w(TAG, "genTextures: " + mBindTexture);
//        GLES20.glEnable(GLES20.GL_TEXTURE_2D);
//        checkGlError("glEnable(GLES20.GL_TEXTURE_2D)");
    }

    protected void doTransfer(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
    }

    protected void setUniform(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        GLES20.glUniformMatrix4fv(muMMatrixHandle, 1, false, modelMatrix, 0);
        GLES20.glUniformMatrix4fv(muVMatrixHandle, 1, false, viewMatrix, 0);
        GLES20.glUniformMatrix4fv(muPMatrixHandle, 1, false, projectMatrix, 0);
    }

    protected void setAttrib() {
    }

    public void draw(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        doAnimation();
        useProgram(mProgram);
        doTransfer(modelMatrix, viewMatrix, projectMatrix);
        setAttrib();
        setUniform(modelMatrix, viewMatrix, projectMatrix);

        GLES20.glDrawElements(GLES20.GL_TRIANGLES, mIndexBuffer.capacity(),
                GLES20.GL_UNSIGNED_SHORT, mIndexBuffer);
    }

    protected void checkGlError(String op) {
        int error;
        String strError;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            switch(error) {
                case GLES20.GL_INVALID_ENUM:// 0x0500
                    strError = "GL_INVALID_ENUM";
                    break;
                case GLES20.GL_INVALID_VALUE:// 0x0501
                    strError = "GL_INVALID_VALUE";
                    break;
                case GLES20.GL_INVALID_OPERATION:// 0x0502
                    strError = "GL_INVALID_OPERATION";
                    break;
                case GLES20.GL_OUT_OF_MEMORY:// 0x0505
                    strError = "GL_OUT_OF_MEMORY";
                    break;
                case GLES20.GL_INVALID_FRAMEBUFFER_OPERATION:// 0x0506
                    strError = "GL_INVALID_FRAMEBUFFER_OPERATION";
                    break;
                default:
                    strError = "Unknown error";
                    break;
            }
            throw new RuntimeException(op + ": glError " + error + " - " + strError);
        }
    }

    protected void onClose() {
        if (null != mTextureIDs) {
            GLES20.glDeleteTextures(mTextureIDs.length, mTextureIDs, 0);
            checkGlError("glDeleteTextures" + mTextureIDs);
        }
    }
}
