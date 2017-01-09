package com.sprd.sprdlauncher2.effect3d;

import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import android.graphics.Bitmap;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

public class Sprd3DCube extends Sprd3DRenderable {
    private static final String TAG = "Sprd3DCube";

    private final String mVertexShader = "uniform mat4 uMMatrix;\n"
            + "uniform mat4 uVMatrix;\n"
            + "uniform mat4 uPMatrix;\n"
            + "attribute vec4 aPosition;\n"
            + "attribute vec4 aColor;\n"
            + "varying  vec4 vColor;\n"
            + "void main() {\n"
            + "  mat4 mvMatrix = uVMatrix * uMMatrix;\n"
            + "  mat4 mvpMatrix = uPMatrix * mvMatrix;\n"
            + "  gl_Position = mvpMatrix * aPosition;\n"
            + "  vColor = aColor;\n"
            + "}\n";

    private final String mFragmentShader = "precision mediump float;\n"
            + "varying  vec4 vColor;\n"
            + "void main() {\n"
              + "  gl_FragColor = vColor;\n"
            + "}\n";

    private final String mTexVertexShader = "uniform mat4 uMMatrix;\n"
            + "uniform mat4 uVMatrix;\n"
            + "uniform mat4 uPMatrix;\n"
            + "attribute vec4 aPosition;\n"
            + "attribute vec2 aTextureCoord;\n"
            + "varying vec2 vTextureCoord;\n"
            + "void main() {\n"
            + "  mat4 mvMatrix = uVMatrix * uMMatrix;\n"
            + "  mat4 mvpMatrix = uPMatrix * mvMatrix;\n"
            + "  gl_Position = mvpMatrix * aPosition;\n"
            + "  vTextureCoord = aTextureCoord;\n"
            + "}\n";

    private final String mTexFragmentShader = "precision mediump float;\n"
            + "varying vec2 vTextureCoord;\n"
            + "uniform sampler2D sTexture;\n"
            + "void main() {\n"
            + "  vec4 color = texture2D(sTexture, vTextureCoord);\n"
            + "  if(gl_FrontFacing) {\n"
            + "      gl_FragColor = color;\n"
            + "  } else {\n"
            + "      gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
            + "  }\n"
            + "}\n";
    private final float Y = 1.0f;
    protected int maColorHandle;
    private FloatBuffer mShortLinesBuffer;
    private FloatBuffer mShortLinesColorsBuffer;
    private FloatBuffer mBorderLinesBuffer;
    private FloatBuffer mBorderLinesColorBuffer;

    private int mTexProgram;
    private int mTexPositionHandle;
    private int mTexMMatrixHandle;
    private int mTexVMatrixHandle;
    private int mTexPMatrixHandle;
    private int mTextureHandle;
    private FloatBuffer mTexLeftVertexBuffer;
    private FloatBuffer mTexVertexBuffer;
    private FloatBuffer mTexRightVertexBuffer;
    private ShortBuffer mTexIndexBuffer;
    protected final int LIGHT_ANIMATION_COUNT = ANIMATION_COUNT / 2;

    // for animation
    private int mAnimationindex = 0;
    private float mAnimationAngle;

    public Sprd3DCube() {
        super();
    }

    public Sprd3DCube(float rate, float zoom, float alpha) {
        super(rate, zoom, alpha);
        init();
    }

    @Override
    protected void createProgram() {
        super.createProgram();

        mTexProgram = Sprd3DUtilities.createProgram(mTexVertexShader, mTexFragmentShader);
        if (mTexProgram == -1) {
            throw new RuntimeException("Could not create texture program");
        }
    }

    @Override
    protected void createMesh() {
        final float X = mRatio;
        final float Z = mRatio;
        final float delta = 0.008f;
        mVerticesData = new float[] {
                delta - X, Y, Z,
                delta - X, -Y, Z,
                X - delta, -Y, Z,
                X - delta, Y, Z,// front
                -X, Y, delta - Z,
                -X, -Y, delta - Z,
                -X, -Y, Z - delta,
                -X, Y, Z - delta,// left
                X - delta, Y, -Z,
                X - delta, -Y, -Z,
                delta - X, -Y, -Z,
                delta - X, Y, -Z,// back
                X, Y, Z - delta,
                X, -Y, Z - delta,
                X, -Y, delta - Z,
                X, Y, delta - Z// right
        };

        mIndexData = new short[] {
//                0, 1, 2, 2, 3, 0,// front
//                4, 5, 6, 6, 7, 4,// left
                8, 9, 10, 10, 11, 8,// back
//                12, 13, 14, 14, 15, 12// right
        };
        createColorData(mAlpha);
        super.createMesh();

        createBorderBuffer();
        createBorderColorBuffer(mAlpha);
        createShortLinesBuffer();
        createShortLinesColorBuffer(mAlpha);
        createTexBuffer();
    }

    private void createColorData(float alpha) {
        final float a = 0.25f * alpha;
        mColorData = new float[16 * 4];
        for (int i=0; i< 16; ++i) {
            mColorData[(i << 2)] = 0.5f;
            mColorData[(i << 2) + 1] = 0.5f;
            mColorData[(i << 2) + 2] = 0.5f;
            mColorData[(i << 2) + 3] = a;
        }
    }

    private void createBorderBuffer() {
        final float X = mRatio;
        final float Z = mRatio;
        final float delta = 0.008f;
        mBorderLinesBuffer = Sprd3DUtilities.getFloatBuffer(new float[] {
                delta - X, Y, Z,
                delta - X, -Y, Z,
                delta - X, -Y, Z,
                X - delta, -Y, Z,
                X - delta, -Y, Z,
                X - delta, Y, Z,
                X - delta, Y, Z,
                delta - X, Y, Z,// front
                -X, Y, delta - Z,
                -X, -Y, delta - Z,
                -X, -Y, delta - Z,
                -X, -Y, Z - delta,
                -X, -Y, Z - delta,
                -X, Y, Z - delta,
                -X, Y, Z - delta,
                -X, Y, delta - Z,// left
                X - delta, Y, -Z,
                X - delta, -Y, -Z,
                X - delta, -Y, -Z,
                delta - X, -Y, -Z,
                delta - X, -Y, -Z,
                delta - X, Y, -Z,
                delta - X, Y, -Z,
                -X, Y, delta - Z,// back
                X, Y, Z - delta,
                X, -Y, Z - delta,
                X, -Y, Z - delta,
                X, -Y, delta - Z,
                X, -Y, delta - Z,
                X, Y, delta - Z,
                X, Y, delta - Z,
                X, Y, Z - delta,// right
        });
    }

    private void createBorderColorBuffer(float alpha) {
        final float a = 0.5f * alpha;
        float[] colors = new float[32 * 4];
        for (int i=0; i< 32; ++i) {
            colors[(i << 2)] = 1.0f;
            colors[(i << 2) + 1] = 1.0f;
            colors[(i << 2) + 2] = 1.0f;
            colors[(i << 2) + 3] = a;
        }
        mBorderLinesColorBuffer = Sprd3DUtilities.getFloatBuffer(colors);
    }

    private void createShortLinesBuffer() {
        final float X = mRatio;
        final float Z = mRatio;
        final float DISTANCE = 0.03F;
        final float LENGTH = 0.15f;
        mShortLinesBuffer = Sprd3DUtilities.getFloatBuffer(new float[] {-LENGTH, Y - DISTANCE, Z, 
                LENGTH, Y - DISTANCE, Z, 
                -X, Y - DISTANCE, LENGTH, 
                -X, Y - DISTANCE, -LENGTH, 
                -LENGTH, Y - DISTANCE, -Z, 
                LENGTH, Y - DISTANCE, -Z, 
                X, Y - DISTANCE, LENGTH, 
                X, Y - DISTANCE, -LENGTH,
                -LENGTH, -Y + DISTANCE, Z, 
                LENGTH, -Y + DISTANCE, Z, 
                -X, -Y + DISTANCE, LENGTH, 
                -X, -Y + DISTANCE, -LENGTH, 
                -LENGTH, -Y + DISTANCE, -Z, 
                LENGTH, -Y + DISTANCE, -Z, 
                X, -Y + DISTANCE, LENGTH, 
                X, -Y + DISTANCE, -LENGTH});
    }

    private void createShortLinesColorBuffer(float alpha) {
        final float a = 0.8f * alpha;
        float[] colors = new float[32 * 4];
        for (int i=0; i< 32; ++i) {
            colors[(i << 2)] = 0.6f;
            colors[(i << 2) + 1] = 1.0f;
            colors[(i << 2) + 2] = 1.0f;
            colors[(i << 2) + 3] = a;
        }
        mShortLinesColorsBuffer = Sprd3DUtilities.getFloatBuffer(colors);
    }

    private void createTexBuffer() {
        final float X = mRatio;
        final float Y = 1.0f;
        final float Z = mRatio;
        final float delta = 0.0f;
        final float distance = 0.02f;
        final float yUpper = 0.0f;
        mTexLeftVertexBuffer = Sprd3DUtilities.getFloatBuffer(new float[] {
                -X - distance, Y + yUpper, delta - Z, 0, 0,
                -X - distance, -Y + yUpper, delta - Z, 0, 1,
                -X - distance, -Y + yUpper, Z - delta, 1, 1,
                -X - distance, Y + yUpper, Z - delta, 1, 0
        });
        mTexVertexBuffer = Sprd3DUtilities.getFloatBuffer(new float[] {
                delta - X, Y + yUpper, Z + distance, 0, 0,
                delta - X, -Y + yUpper, Z + distance, 0, 1,
                X - delta, -Y + yUpper, Z + distance, 1, 1,
                X - delta, Y + yUpper, Z + distance, 1, 0
        });
        mTexRightVertexBuffer = Sprd3DUtilities.getFloatBuffer(new float[] {
                X + distance, Y + yUpper, Z - delta, 0, 0,
                X + distance, -Y + yUpper, Z - delta, 0, 1,
                X + distance, -Y + yUpper, delta - Z, 1, 1,
                X + distance, Y + yUpper, delta - Z, 1, 0
        });

        mTexIndexBuffer = Sprd3DUtilities.getShortBuffer(new short[] {
                0, 1, 2, 2, 3, 0
        });
    }

    @Override
    protected String getVertexShader() {
        return mVertexShader;
    }

    @Override
    protected String getFragmentShader() {
        return mFragmentShader;
    }

    @Override
    protected void handleVariables() {
        super.handleVariables();

        maColorHandle = GLES20.glGetAttribLocation(mProgram, "aColor");
        checkGlError("glGetAttribLocation aColor");
        if (maColorHandle == -1) {
            throw new RuntimeException(
                    "Could not get attrib location for vColor");
        }
        handleTexVariables();
    }

    private void handleTexVariables() {
        mTexPositionHandle = GLES20.glGetAttribLocation(mTexProgram, "aPosition");
        checkGlError("glGetAttribLocation");
        if (maPositionHandle == -1) {
            throw new RuntimeException("Could not get attrib location for aPosition");
        }

        mTexMMatrixHandle = GLES20.glGetUniformLocation(mTexProgram, "uMMatrix");
        checkGlError("glGetUniformLocation");
        if (muMMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uMMatrix");
        }

        mTexVMatrixHandle = GLES20.glGetUniformLocation(mTexProgram, "uVMatrix");
        checkGlError("glGetUniformLocation");
        if (muVMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uVMatrix");
        }

        mTexPMatrixHandle = GLES20.glGetUniformLocation(mTexProgram, "uPMatrix");
        checkGlError("glGetUniformLocation");
        if (muPMatrixHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uPMatrix");
        }

        mTextureHandle = GLES20.glGetAttribLocation(mTexProgram, "aTextureCoord");
        checkGlError("glGetAttribLocation aTextureCoord");
        if (mTextureHandle == -1) {
            throw new RuntimeException("Could not get attrib location for aTextureCoord");
        }
    }

    @Override
    public void setAlpha(float alpha) {
        if (alpha > 1.0f) {
            alpha = 1.0f;
        } else if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        super.setAlpha(alpha);

        createColorData(alpha);
        mColorBuffer = Sprd3DUtilities.getFloatBuffer(mColorData);
        createBorderColorBuffer(alpha);
        createShortLinesColorBuffer(alpha);
    }

    @Override
    public void onBeginMoving(float x , float y) {
        mLastX = x;
        mRotateYValue = 0;
        mTranslate = 0;
        mAnimation = false;
        Log.w(TAG, "beginAnimation " + x);
    }

    @Override
    public void onMoving(float x, float y) {
        float dx = x - mLastX;
        mLastX = x;

        mRotateYValue += (float) (dx * 90.0) * 1.1f;
        if (mLimitLeft) {
            if (mRotateYValue > LIMIT_ANGLE) {
                mRotateYValue = LIMIT_ANGLE;
            }
        } else if (mLimitRight) {
            if (mRotateYValue < -LIMIT_ANGLE) {
                mRotateYValue = -LIMIT_ANGLE;
            }
        }

        calcTranslate(mRotateYValue / 90.0f);
        Log.w(TAG, "onActionMove - Rotate: " + mRotateYValue + ", Translate: " + mTranslate);
    }

    @Override
    public void onStopMoving() {
        if (mFling) {
            Log.w(TAG, "onActionUp: fling is true.");
            return;
        }
        mAnimation = true;
        if (mRotateYValue > 37.0f) {
            mAnimationAngle = (90.01f - mRotateYValue);
            mPageDelta = -1;
            Log.w(TAG, "[37, 90) " + mAnimationAngle + ", " + mPageDelta);
        } else if (mRotateYValue > -37.0f) {
            mAnimationAngle = -mRotateYValue;
            mPageDelta = 0;
            Log.w(TAG, "(-37, 37] " + mAnimationAngle + ", " + mPageDelta);
        } else {
            mAnimationAngle = -(mRotateYValue + 90.01f);
            mPageDelta = 1;
            Log.w(TAG, "(-90, -37] " + mAnimationAngle + ", " + mPageDelta);
        }

        if (mRotateYValue < 3.0f && mRotateYValue > -3.0f) {
            mAnimationindex = ANIMATION_COUNT;
        } else {
            mAnimationindex = 0;
        }
        Log.w(TAG, "onActionUp - " + mAnimationAngle + ", " + mAnimation);
        if (mCallback != null) {
            mCallback.onPreClose(mPageDelta);
        }
    }

    @Override
    public void onFling(float velocityX, float velocityY) {
        if (mFling == true) {
            return;
        }
        Log.w(TAG, "onFling: " + mRotateYValue + ", " + mTranslate + ", " + velocityX);
        if (velocityX > FLING_VELOCITY_LIMIT) {
            if (mLimitLeft) {
                Log.w(TAG, "LimitLeft - " + mRotateYValue);
                if (mRotateYValue > LIMIT_ANGLE) {
                    mRotateYValue = LIMIT_ANGLE;
                }
                onLightFling(velocityX, velocityY);
            } else {
                mAnimationindex = 0;
                mAnimationAngle = (90.0f - mRotateYValue);
                mPageDelta = -1;
                mFling = true;
                if (mCallback != null) {
                    mCallback.onPreClose(mPageDelta);
                }
            }
        } else if (velocityX < -FLING_VELOCITY_LIMIT) {
            if (mLimitRight) {
                Log.w(TAG, "LimitRight - " + mRotateYValue);
                if (mRotateYValue < -LIMIT_ANGLE) {
                    mRotateYValue = -LIMIT_ANGLE;
                }
                onLightFling(velocityX, velocityY);
            } else {
                mAnimationindex = 0;
                mAnimationAngle = -(90.0f + mRotateYValue);
                mPageDelta = 1;
                mFling = true;
                if (mCallback != null) {
                    mCallback.onPreClose(mPageDelta);
                }
            }
        } else {
            Log.w(TAG, "The velocityX of fling is too small(<" + FLING_VELOCITY_LIMIT + "): " + velocityX);
            onLightFling(velocityX, velocityY);
        }
    }

    @Override
    protected void onLightFling(float velocityX, float velocityY) {
        Log.w(TAG, "onLightFling: " + velocityX);
        mFling = false;
        mLightFling = true;
        mAnimationAngle = (velocityX > 0 ? LIMIT_ANGLE : - LIMIT_ANGLE);
        mAnimationindex = (int) (mRotateYValue * LIGHT_ANIMATION_COUNT / mAnimationAngle);
        mPageDelta = 0;
        Log.w(TAG, "onLightFling: " + mRotateYValue + ", " + mAnimationAngle + ", " + mAnimationindex);
        if (mCallback != null) {
            mCallback.onPreClose(mPageDelta);
        }
    }

    @Override
    protected void doAnimation() {
        if (mLightFling) {
            if (mAnimationindex < LIGHT_ANIMATION_COUNT) {
                mRotateYValue += mAnimationAngle / LIGHT_ANIMATION_COUNT;
                calcTranslate(mRotateYValue / 90.0f);
//                Log.w(TAG, "1. RotateValue: " + mRotateYValue + ", " + mAnimationindex);
                ++mAnimationindex;
            } else if (mAnimationindex < 2 * LIGHT_ANIMATION_COUNT) {
                mRotateYValue -= mAnimationAngle / LIGHT_ANIMATION_COUNT;
                calcTranslate(mRotateYValue / 90.0f);
//                Log.w(TAG, "2. RotateValue: " + mRotateYValue + ", " + mAnimationindex);
                ++mAnimationindex;
                setAlpha(1.0f - 0.02f * mAnimationindex);
            } else if (mAnimationindex == 2 * LIGHT_ANIMATION_COUNT) {
                adjustRotate();
//                Log.w(TAG, "3. RotateValue: " + mRotateYValue + ", " + mAnimationindex);
                setAlpha(0.4f);
                mTranslate = 0.0f;
                ++mAnimationindex;
                if (null != mCallback) {
                    mCallback.onClosing(mPageDelta);
                }
            } else if (mAnimationindex < 3 * LIGHT_ANIMATION_COUNT) {
                setAlpha(getAlpha() - 0.08f);
                mAnimationindex += 2;
            } else {
                Log.w(TAG, "endAnimation: " + mPageDelta);
                finishAnimation();
            }
        } else if (mAnimation || mFling) {
            if (mAnimationindex < ANIMATION_COUNT) {
                mRotateYValue += mAnimationAngle / ANIMATION_COUNT;
                calcTranslate(mRotateYValue / 90.0f);
                ++mAnimationindex;
                setAlpha(1.0f - 0.03f * mAnimationindex);
//                Log.w(TAG, "RotateValue: " + mRotateYValue + ", " + mAnimationindex);
            } else if (mAnimationindex == ANIMATION_COUNT) {
                adjustRotate();
                setAlpha(0.5f);
                mTranslate = 0.0f;
                ++mAnimationindex;
                if (null != mCallback) {
                    mCallback.onClosing(mPageDelta);
                }
            } else if (mAnimationindex < 1.4 * ANIMATION_COUNT) {
//                mTranslate = 0.0f;
                ++mAnimationindex;
                setAlpha(0.45f - 0.06f * (mAnimationindex - ANIMATION_COUNT));
            } else {
                Log.w(TAG, "endAnimation: " + mPageDelta);
                finishAnimation();
            }
        } else if (mAlpha < 1.0f) {
            if (mAlpha < 0.919999f) {
                setAlpha(mAlpha + 0.08f);
            } else {
                setAlpha(1.0f);
            }
        }
    }

    @Override
    protected void doTransfer(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        Matrix.setIdentityM(modelMatrix, 0);
        Matrix.translateM(modelMatrix, 0, 0, 0.0f, mTranslate - 0.05f);
        Matrix.rotateM(modelMatrix, 0, mRotateYValue, 0, 1, 0);
        Matrix.scaleM(modelMatrix, 0, mZoom, mZoom, mZoom);
    }

    @Override
    protected void setAttrib() {
        mVerticesBuffer.position(0);
        GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 3 * 4, mVerticesBuffer);
        GLES20.glEnableVertexAttribArray(maPositionHandle);

        mColorBuffer.position(0);
        GLES20.glVertexAttribPointer(maColorHandle, 4, GLES20.GL_FLOAT, false, 4 * 4, mColorBuffer);
        GLES20.glEnableVertexAttribArray(maColorHandle);
    }

    @Override
    public void draw(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        drawTexture(modelMatrix, viewMatrix, projectMatrix);

        super.draw(modelMatrix, viewMatrix, projectMatrix);

        drawBorderLines();
        drawShortLines();

    }

    private void drawBorderLines() {
        mBorderLinesBuffer.position(0);
        GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 3 * 4, mBorderLinesBuffer);
        GLES20.glEnableVertexAttribArray(maPositionHandle);
        mBorderLinesColorBuffer.position(0);
        GLES20.glVertexAttribPointer(maColorHandle, 4, GLES20.GL_FLOAT, false, 4 * 4, mBorderLinesColorBuffer);
        GLES20.glEnableVertexAttribArray(maColorHandle);
        GLES20.glLineWidth(1.0f);
        GLES20.glDrawArrays(GLES20.GL_LINES, 0, 32);
    }
    
    private void drawShortLines() {
        mShortLinesBuffer.position(0);
        GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 3 * 4, mShortLinesBuffer);
        GLES20.glEnableVertexAttribArray(maPositionHandle);
        mShortLinesColorsBuffer.position(0);
        GLES20.glVertexAttribPointer(maColorHandle, 4, GLES20.GL_FLOAT, false, 4 * 4, mShortLinesColorsBuffer);
        GLES20.glEnableVertexAttribArray(maColorHandle);
        GLES20.glLineWidth(3.0f);
        GLES20.glDrawArrays(GLES20.GL_LINES, 0, 16);
    }

    private void drawTexture(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        if (null == mTextureIDs) {
            return;
        }
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);

        if ((mBindTexture & 0x1) > 0 && mRotateYValue > 0.1f) {
            drawTexture(mTexLeftVertexBuffer, mTextureIDs[0], modelMatrix, viewMatrix, projectMatrix);
        }
        if ((mBindTexture & 0x2) > 0 && mRotateYValue < 89.9f && mRotateYValue > -89.9f) {
            drawTexture(mTexVertexBuffer, mTextureIDs[1], modelMatrix, viewMatrix, projectMatrix);
        }
        if ((mBindTexture & 0x4) > 0 && mRotateYValue < -0.1f) {
            drawTexture(mTexRightVertexBuffer, mTextureIDs[2], modelMatrix, viewMatrix, projectMatrix);
        }
    }

    private void drawTexture(FloatBuffer verticesBuffer, int textureID, float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        // setAttrib
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureID);

        verticesBuffer.position(0);
        GLES20.glVertexAttribPointer(mTexPositionHandle, 3, GLES20.GL_FLOAT, false, 5 * 4, verticesBuffer);
        checkGlError("glVertexAttribPointer mTexPositionHandle");
        GLES20.glEnableVertexAttribArray(mTexPositionHandle);
        checkGlError("glEnableVertexAttribArray mTexPositionHandle");

        verticesBuffer.position(3);
        GLES20.glVertexAttribPointer(mTextureHandle, 2, GLES20.GL_FLOAT, false, 5 * 4, verticesBuffer);
        checkGlError("glVertexAttribPointer maTextureHandle");
        GLES20.glEnableVertexAttribArray(mTextureHandle);
        checkGlError("glEnableVertexAttribArray maTextureHandle");

        useProgram(mTexProgram);

        // setUniform
        GLES20.glUniformMatrix4fv(mTexMMatrixHandle, 1, false, modelMatrix, 0);
        GLES20.glUniformMatrix4fv(mTexVMatrixHandle, 1, false, viewMatrix, 0);
        GLES20.glUniformMatrix4fv(mTexPMatrixHandle, 1, false, projectMatrix, 0);
//        GLES20.glUniform1i(mTextureHandle, 0);

        mTexIndexBuffer.position(0);
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, mTexIndexBuffer.capacity(),
                GLES20.GL_UNSIGNED_SHORT, mTexIndexBuffer);
    }

    @Override
    protected void onClose() {
        if (null != mTextureIDs) {
            GLES20.glDeleteTextures(mTextureIDs.length, mTextureIDs, 0);
//            Log.w(TAG, "-----onClose: mTextureIDs: " + mTextureIDs + "-----");
            checkGlError("glDeleteTextures" + mTextureIDs);
        }
    }

    private void adjustRotate() {
        if (mRotateYValue > 86.0f) {
            mRotateYValue = 90.0f;
        } else if (mRotateYValue > -10.0f) {
            mRotateYValue = 0.0f;
        } else {
            mRotateYValue = -90.0f;
        }
    }

    private void calcTranslate(float r) {
        if (r < 0.0f) {
            r = -r;
        }
        while (r >= 1.0f) {
            r -= 1.0f;
        }
        mTranslate = 5 * r * (r - 1);
    }

    private void finishAnimation() {
        mRotateYValue = 0.0f;
        mTranslate = 0.0f;
        mAnimation = false;
        mFling = false;
        mLightFling = false;
        if (mCallback != null) {
            mCallback.onClose(mPageDelta);
        }
        onClose();
    }
}
