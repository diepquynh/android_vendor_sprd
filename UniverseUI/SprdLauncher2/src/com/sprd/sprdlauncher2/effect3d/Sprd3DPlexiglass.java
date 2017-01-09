package com.sprd.sprdlauncher2.effect3d;

import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

public class Sprd3DPlexiglass extends Sprd3DRenderable {
    private static final String TAG = "Sprd3DPlexiglass";

    private final String mVertexShader = 
            "uniform mat4 uMMatrix;\n"
          + "uniform mat4 uVMatrix;\n"
          + "uniform mat4 uPMatrix;\n"
          + "attribute vec4 aPosition;\n"
          + "attribute vec4 aNormal;\n"
          + "attribute vec4 aTexCoor;\n"
          + "varying vec3 wordNormal;\n"
          + "varying vec3 wordPostion;\n"
          + "varying vec2 wordTex0;\n"
          + "void main() {\n"
          + "  mat4 mvpMatrix = uVMatrix * uMMatrix;\n"
          + "  mvpMatrix = uPMatrix * mvpMatrix;\n"
          + "  wordPostion = (uMMatrix * aPosition).xyz;\n"
          + "  wordNormal = normalize((uMMatrix * aNormal).xyz);\n"
          + "  wordTex0 = aTexCoor.xy;\n"
          + "  gl_Position = mvpMatrix * aPosition;\n"
          + "}\n";

  private final String mFragmentShaderT = 
            "precision mediump float;\n"
          + "uniform sampler2D uTexture;"
          + "uniform vec3 uCamPosition;\n"
          + "uniform vec3 uLightPosition;\n"
          + "uniform vec4 uAmbientCol;\n"
          + "uniform vec4 uSpecCol;\n"
          + "varying vec3 wordNormal;\n"
          + "varying vec3 wordPostion;\n"
          + "varying vec2 wordTex0;\n"
          + "void main() {\n"
          + "  vec3 DirToCam = normalize(uCamPosition - wordPostion);\n"
          + "  vec3 DirToLight = normalize(uLightPosition - wordPostion);\n"
          + "  vec3 reflectDir = normalize(reflect(-DirToLight, wordNormal));\n"
          + "  vec3 halfDir = normalize(DirToCam + DirToLight);\n"
          + "  float AmbFactor = 0.6;\n"
          + "  float DiffuseFactor = max(dot(-DirToLight, wordNormal), 0.0);\n"
          + "  float SpecFactor = max(dot(wordNormal, halfDir), 0.0);\n"
          + "  vec4 color = texture2D(uTexture, wordTex0);\n"
          + "  if(color.a < 0.5) {\n"
          //+ "    color = vec4(0.75, 0.867, 0.898, 0.4);\n"
          //+ "    color = vec4(0.477, 0.699, 0.813, 0.4);\n"
          + "    color = vec4(1.0, 1.0, 1.0, 0.1);\n"
          + "    color = color * AmbFactor * uAmbientCol + color * DiffuseFactor * uSpecCol + uSpecCol * pow(SpecFactor, 50.0);\n"
          + "  }\n"
          //+ "  color = vec4(0.6, 0.6, 1.0, 0.2);\n"
          + "  if(gl_FrontFacing) {\n"
          + "      gl_FragColor = color;\n"
          + "  } else {\n"
          + "      gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
          + "  }\n"
          //+ "  gl_FragColor = color;\n"
          + "}\n";

  private final String mFragmentShader = 
          "precision mediump float;\n"
        + "uniform sampler2D uTexture;"
        + "uniform vec3 uCamPosition;\n"
        + "uniform vec3 uLightPosition;\n"
        + "uniform vec4 uAmbientCol;\n"
        + "uniform vec4 uSpecCol;\n"
        + "varying vec3 wordNormal;\n"
        + "varying vec3 wordPostion;\n"
        + "varying vec2 wordTex0;\n"
        + "void main() {\n"
        + "  vec3 DirToCam = normalize(uCamPosition - wordPostion);\n"
        + "  vec3 DirToLight = normalize(uLightPosition - wordPostion);\n"
        + "  vec3 reflectDir = reflect(-DirToLight, wordNormal);\n"
        + "  vec3 halfDir = normalize(DirToCam + DirToLight);\n"
        + "  float AmbFactor = 0.8;\n"
        + "  float DiffuseFactor = max(dot(-DirToLight, wordNormal), 0.0);\n"
        + "  float SpecFactor = max(dot(wordNormal, halfDir), 0.0);\n"
        + "  vec4 color = texture2D(uTexture, wordTex0);\n"
        //+ "  color = vec4(0.75, 0.867, 0.898, 0.4);\n"
//        + "  color = vec4(0.477, 0.699, 0.813, 0.4);\n"
        + "if (color.a < 0.4){\n"
        + "  color = vec4(0.477, 0.699, 0.813, 0.4);\n"
        + "} else {\n"
        + "  color = vec4(0.477, 0.699, 0.813, color.a);\n"
        + "}"
        //+ "  color = vec4(1.0, 1.0, 1.0, 0.1);\n"
        //+ "  \n"
        + "  gl_FragColor = color * AmbFactor * uAmbientCol + color * DiffuseFactor * uSpecCol + uSpecCol * pow(SpecFactor, 50.0);\n"
        //+ "  gl_FragColor = color;\n"
        + "}\n";

    // 定义环境光颜色
    private FloatBuffer lightAmbient = FloatBuffer.wrap(new float[] { 0.3f, 0.3f, 0.3f, 1.0f });
    // 定义镜面光颜色
    private FloatBuffer lightSpec = FloatBuffer.wrap(new float[] { 0.5f, 0.5f, 0.5f, 0.5f });
    // 光源的位置
    private FloatBuffer lightPosition = FloatBuffer.wrap(new float[] { -5.0f, 1.0f, 0.0f});
    private FloatBuffer camPosition = FloatBuffer.wrap(new float[] { 0.0f, 0.0f, 5.0f});

    private static short[] mIndexDateT = {
        // front
        0,1,2,
        0,2,3,

        // back
        4,5,6,
        4,6,7,
    };

    protected int muTextureHandle;
    protected int muLightPositionHandle;
    protected int muCamPositionHandle;
    protected int muAmbientHandle;
    protected int muSpecHandle;
    
    private int mProgramT;
    private ShortBuffer mIndexBufferT;
    protected int maNormalHandle;
    private int maTexcoorHandle;
    // for animation
    private float mScale = 0.8f;
    private float mScaleDelta = (1.0f - mScale) / ANIMATION_COUNT;
    private int mAnimationindex = 0;
    private float mAnimationAngle;

    private static final int NUM_VERTICES = 4 * 6; // 4 vertices per side * 6 sides
    private static final  int NUM_ENTRIES_PER_VERTEX = 8;
    private static final  int NUM_VERTEX_ENTRIES = NUM_VERTICES * NUM_ENTRIES_PER_VERTEX;

    public Sprd3DPlexiglass() {
        super();
    }

    public Sprd3DPlexiglass(float ratio, float zoom, float alpha) {
        super(ratio, zoom, alpha);
        init();
    }

    @Override
    protected void createMesh() {
        mVerticesData = new float[] {
                // front side
                -1, -1, 1,  0, 0, 1,  0, 1,    // pos, normal, texcoord
                1, -1, 1,   0, 0, 1,  1, 1,
                1,  1, 1,   0, 0, 1,  1, 0,
                -1, 1, 1,   0, 0, 1,  0, 0,

                // back side
                1, -1, -1,  0, 0, -1, 0, 1,
                -1, -1, -1, 0, 0, -1, 1, 1,
                -1, 1, -1,  0, 0, -1, 1, 0,
                1, 1, -1,   0, 0, -1, 0, 0,

                // left side
                -1, -1, -1, -1, 0, 0, 0, 1,
                -1, -1, 1,  -1, 0, 0, 1, 1,
                -1, 1, 1,   -1, 0, 0, 1, 0,
                -1, 1, -1,  -1, 0, 0, 0, 0, 

                // right side
                1, -1, 1,   1, 0, 0,  0, 1,
                1, -1, -1,  1, 0, 0,  1, 1,
                1, 1, -1,   1, 0, 0,  1, 0,
                1, 1, 1,    1, 0, 0,  0, 0,

                // up side
                -1, 1, 1,   0, 1, 0,  0, 1,
                1, 1, 1,    0, 1, 0,  1, 1,
                1, 1, -1,   0, 1, 0,  1, 0,
                -1, 1, -1,  0, 1, 0,  0, 0,

                // down side
                -1, -1, -1, 0, -1, 0, 0, 1,
                1, -1, -1,  0, -1, 0, 1, 1,
                1, -1, 1,   0, -1, 0, 1, 0,
                -1, -1, 1,  0, -1, 0, 0, 0 
        };

        mIndexData = new short[] {
//                // front
//                0,1,2,
//                0,2,3,
//
//                // back
//                4,5,6,
//                4,6,7,

                // left
                8,9,10,
                8,10,11,

                // right
                12,13,14,
                12,14,15,

                // up
                16,17,18,
                16,18,19,

                // down
                20,21,22,
                20,22,23
        };
        super.createMesh();

        mIndexBufferT = Sprd3DUtilities.getShortBuffer(mIndexDateT);
    }
    
    @Override
    protected void createProgram() {
        super.createProgram();
        mProgramT = Sprd3DUtilities.createProgram(mVertexShader, mFragmentShaderT);
        if (mProgramT == -1) {
            throw new RuntimeException("Could not create mProgramT");
        }
    }

    @Override
    protected void handleVariables() {
        super.handleVariables();

        muTextureHandle = GLES20.glGetUniformLocation(mProgram, "uTexture");
        checkGlError("glGetUniformLocation");
        if (muTextureHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uTexture");
        }

        muLightPositionHandle = GLES20.glGetUniformLocation(mProgram, "uLightPosition");
        checkGlError("glGetUniformLocation");
        if (muLightPositionHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uLightPosition");
        }

        muCamPositionHandle = GLES20.glGetUniformLocation(mProgram, "uCamPosition");
        checkGlError("glGetUniformLocation");
        if (muCamPositionHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uCamPosition");
        }

        muAmbientHandle = GLES20.glGetUniformLocation(mProgram, "uAmbientCol");
        checkGlError("glGetUniformLocation");
        if (muAmbientHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uAmbientCol");
        }

        muSpecHandle = GLES20.glGetUniformLocation(mProgram, "uSpecCol");
        checkGlError("glGetUniformLocation");
        if (muSpecHandle == -1) {
            throw new RuntimeException("Could not get Uniform location for uSpecCol");
        }

        maNormalHandle = GLES20.glGetAttribLocation(mProgram, "aNormal");
        if (maNormalHandle == -1) {
            throw new RuntimeException(
                    "Could not get attrib location for vColor");
        }
        maTexcoorHandle = GLES20.glGetAttribLocation(mProgram, "aTexCoor");
        checkGlError("glGetAttribLocation aTextureCoord");
        if (maTexcoorHandle == -1) {
            throw new RuntimeException("Could not get attrib location for aTextureCoord");
        }
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
    protected void setUniform(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        super.setUniform(modelMatrix, viewMatrix, projectMatrix);

        GLES20.glUniform1i(muTextureHandle, 0);
        GLES20.glUniform3fv(muLightPositionHandle, 1, lightPosition);
        GLES20.glUniform3fv(muCamPositionHandle, 1, camPosition);
        GLES20.glUniform4fv(muAmbientHandle, 1, lightAmbient);
        GLES20.glUniform4fv(muSpecHandle, 1, lightSpec);
    }

    @Override
    protected void setAttrib() {
        super.setAttrib();

        mVerticesBuffer.position(0);
        GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, NUM_ENTRIES_PER_VERTEX * 4, mVerticesBuffer);
        GLES20.glEnableVertexAttribArray(maPositionHandle);

        mVerticesBuffer.position(3);
        GLES20.glVertexAttribPointer(maNormalHandle, 3, GLES20.GL_FLOAT, false, NUM_ENTRIES_PER_VERTEX * 4, mVerticesBuffer);
        GLES20.glEnableVertexAttribArray(maNormalHandle);

        mVerticesBuffer.position(6);
        GLES20.glVertexAttribPointer(maTexcoorHandle, 2, GLES20.GL_FLOAT, false, NUM_ENTRIES_PER_VERTEX * 4, mVerticesBuffer);
        GLES20.glEnableVertexAttribArray(maTexcoorHandle);
    }

    @Override
    protected void doTransfer(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        Matrix.setIdentityM(modelMatrix, 0);
        Matrix.rotateM(modelMatrix, 0, mRotateYValue, 0, 1, 0);
        Matrix.scaleM(modelMatrix, 0, mScale * mZoom * mRatio, mScale * mZoom, 0.1f);
    }

    public void setRotateValue(float rotate) {
        mRotateYValue = rotate;
    }

    @Override
    public void onBeginMoving(float x, float y) {
        mLastX = x;
        mRotateYValue = 0;
    }

    @Override
    public void onMoving(float x, float y) {
//        Log.e(TAG, "onTouchEvent:x=" + x + " y=" + y);
        float dx = x - mLastX;
        mLastX = x;

        mRotateYValue += dx * 180.0f;
        if (mLimitLeft) {
            if (mRotateYValue > LIMIT_ANGLE) {
                mRotateYValue = LIMIT_ANGLE;
            }
        } else if (mLimitRight) {
            if (mRotateYValue < -LIMIT_ANGLE) {
                mRotateYValue = -LIMIT_ANGLE;
            }
        }
    }

    @Override
    public void onStopMoving() {
        if (mRotateYValue > 90.0f) {
            mPageDelta = 1;
            mAnimationAngle = (180.0f - mRotateYValue);
        } else if (mRotateYValue < -90.0f) {
            mPageDelta = -1;
            mAnimationAngle = (-180.0f - mRotateYValue);
        } else {
            mPageDelta = 0;
            mAnimationAngle = -mRotateYValue;
        }
        mAnimationindex = 0;
        mAnimation = true;
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
                mFling = false;
                onStopMoving();
            } else {
                mAnimationindex = 0;
                mAnimationAngle = (180.0f - mRotateYValue);
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
                mFling = false;
                onStopMoving();
            } else {
                mAnimationindex = 0;
                mAnimationAngle = -(180.0f + mRotateYValue);
                mPageDelta = 1;
                mFling = true;
                if (mCallback != null) {
                    mCallback.onPreClose(mPageDelta);
                }
            }
        } else {
            Log.w(TAG, "The velocityX of fling is too small(<" + FLING_VELOCITY_LIMIT + "): " + velocityX);
            mFling = false;
            onStopMoving();
        }
    }

    @Override
    protected void doAnimation() {
        if (mAnimation || mFling) {
            if (mAnimationindex < ANIMATION_COUNT) {
                mRotateYValue += mAnimationAngle / ANIMATION_COUNT;
                ++mAnimationindex;
                mScale += mScaleDelta;
//                setAlpha(1.0f - 0.01f * mAnimationindex);
//                Log.w(TAG, "RotateValue: " + mRotateYValue + ", " + mAnimationindex);
            } else if (mAnimationindex == ANIMATION_COUNT) {
                adjustAnimation();
//                setAlpha(0.65f);
                mTranslate = 0.0f;
                ++mAnimationindex;
                if (null != mCallback) {
                    mCallback.onClosing(mPageDelta);
                }
            } else {
                if (mAnimationindex < 1.4 * ANIMATION_COUNT) {
                    mTranslate = 0.0f;
                    ++mAnimationindex;
//                    setAlpha(0.65f - 0.06f * (mAnimationindex - ANIMATION_COUNT));
                } else {
                    Log.w(TAG, "endAnimation: " + mPageDelta);
                    finishAnimation();
                }
            }
        } else if (mAlpha < 0.9999f) {
            if (mAlpha < 0.949999f) {
                setAlpha(mAlpha + 0.05f);
            } else {
                setAlpha(1.0f);
            }
        }
    }

    private void adjustAnimation() {
        if (mRotateYValue > 175.0f) {
            mRotateYValue = 180.0f;
        } else if (mRotateYValue > -5.0f) {
            mRotateYValue = 0.0f;
        } else {
            mRotateYValue = -180.0f;
        }
        mScaleDelta = 1.0f;
    }

    private void finishAnimation() {
        mRotateYValue = 0.0f;
        mTranslate = 0.0f;
        mAnimation = false;
        mFling = false;
        if (mCallback != null) {
            mCallback.onClose(mPageDelta);
        }
        onClose();
    }

    private void drawCurrent() {
        if ((mBindTexture & 0x02) == 0) {
            return;
        }
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[1]);
        mIndexBufferT.position(0);
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 6, GLES20.GL_UNSIGNED_SHORT, mIndexBufferT);
    }

    private void drawNext() {
        int index = mRotateYValue < 0 ? 2 : 0;
        if ((mBindTexture & (1 << index)) == 0) {
            return;
        }
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[index]);
        mIndexBufferT.position(6);
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 6, GLES20.GL_UNSIGNED_SHORT, mIndexBufferT);
    }

    @Override
    public void draw(float[] modelMatrix, float[] viewMatrix, float[] projectMatrix) {
        doTransfer(modelMatrix, viewMatrix, projectMatrix);
        GLES20.glUseProgram(mProgramT);
        setAttrib();
        setUniform(modelMatrix, viewMatrix, projectMatrix);

        if(mRotateYValue > -90.0f && mRotateYValue < 90.0f) {
            drawCurrent();
            drawNext();
        } else {
            drawNext();
            drawCurrent();
        }

        doTransfer(modelMatrix, viewMatrix, projectMatrix);
        useProgram(mProgram);
        setAttrib();
        setUniform(modelMatrix, viewMatrix, projectMatrix);
        super.draw(modelMatrix, viewMatrix, projectMatrix);
    }
}
