/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.camera;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;

import com.android.camera.debug.Log;
import com.android.camera.ui.RotateImageButton;
import com.android.camera.util.Gusterpolator;
import com.android.camera2.R;

/*
 * A toggle button that supports two or more states with images rendererd on top
 * for each state.
 * The button is initialized in an XML layout file with an array reference of
 * image ids (e.g. imageIds="@array/camera_flashmode_icons").
 * Each image in the referenced array represents a single integer state.
 * Every time the user touches the button it gets set to next state in line,
 * with the corresponding image drawn onto the face of the button.
 * State wraps back to 0 on user touch when button is already at n-1 state.
 */
public class MultiToggleImageButton extends RotateImageButton {
    /*
     * Listener interface for button state changes.
     */
    public interface OnStateChangeListener {
        /*
         * @param view the MultiToggleImageButton that received the touch event
         * @param state the new state the button is in
         */
        public abstract void stateChanged(View view, int state);
    }

    // SPRD Bug:519334 Refactor Rotation UI of Camera.
    private final static Log.Tag TAG = new Log.Tag("MultiToggleButton");

    public static final int ANIM_DIRECTION_VERTICAL = 0;
    public static final int ANIM_DIRECTION_HORIZONTAL = 1;

    private static final int ANIM_DURATION_MS = 0;
    private static final int UNSET = -1;

    private OnStateChangeListener mOnStateChangeListener;
    private OnStateChangeListener mOnStatePreChangeListener;
    private int mState = UNSET;
    private int[] mImageIds;
    private int[] mDescIds;
    private int mLevel;
    private boolean mClickEnabled = true;
    private int mParentSize;
    private int mAnimDirection;
    private Matrix mMatrix = new Matrix();
    private ValueAnimator mAnimator;
    private Context mContext;

    public MultiToggleImageButton(Context context) {
        super(context);
        mContext = context;
        init();
    }

    public MultiToggleImageButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        init();
        parseAttributes(context, attrs);
        /*
         * SPRD: SPRD: nanjing tester's response 4: the UI will show flash "off"
         * before show real value
         * @{
         */
        setVisibility(INVISIBLE);
       //setState(0);
        /* @} */
    }

    /*
     * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
     * Original Android code:

    public MultiToggleImageButton(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
        parseAttributes(context, attrs);
        setState(0);
    }

      */

    /*
     * Set the state change listener.
     *
     * @param onStateChangeListener The listener to set.
     */
    public void setOnStateChangeListener(OnStateChangeListener onStateChangeListener) {
        mOnStateChangeListener = onStateChangeListener;
    }

    /**
     * Set the listener that will be invoked right after the click event before
     * all the operations required to change the state of the button.  This
     * listener is useful if the client doesn't want to wait until the state
     * change is completed to perform certain tasks.
     *
     * @param onStatePreChangeListener The listener to set.
     */
    public void setOnPreChangeListener(OnStateChangeListener onStatePreChangeListener) {
        mOnStatePreChangeListener = onStatePreChangeListener;
    }

    /*
     * Get the current button state.
     *
     */
    public int getState() {
        return mState;
    }

    /*
     * Set the current button state, thus causing the state change listener to
     * get called.
     *
     * @param state the desired state
     */
    public void setState(int state) {
        setState(state, true);
    }

    /*
     * Set the current button state.
     *
     * @param state the desired state
     * @param callListener should the state change listener be called?
     */
    public void setState(final int state, final boolean callListener) {
        /* SPRD: fix for bug 505919 there may be problem with ArrayIndexOutOfBoundsException @{ */
        if (state >= mImageIds.length) {
            return;
        }
        /* @} */
        setStateAnimatedInternal(state, callListener);
    }

    /**
     * Set the current button state via an animated transition.
     *
     * @param state
     * @param callListener
     */
    private void setStateAnimatedInternal(final int state, final boolean callListener) {
        if(callListener && mOnStatePreChangeListener != null) {
            mOnStatePreChangeListener.stateChanged(MultiToggleImageButton.this, mState);
        }

        if (mState == state || mState == UNSET) {
            setStateInternal(state, callListener);
            return;
        }

        if (mImageIds == null) {
            return;
        }

        /* SPRD:fix bug 618437 cancel use AsyncTask because no animation @{ */
        setStateInternal(state, callListener);
        setClickEnabled(true);
        /* @} */

//        new AsyncTask<Integer, Void, Bitmap>() {
//
//            // SPRD: Fix bug 559046 whether error happens
//            boolean hasError = false;
//
//            @Override
//            protected Bitmap doInBackground(Integer... params) {
//                /*
//                 * SPRD: Fix bug 559046 overrideImageIds will reset mImageIds, which could result
//                 * ArrayIndexOutOfBoundsException or Resources$NotFoundException,
//                 * not add synchronization here, just try-catch it @{
//                 * original code
//                 *
//                return combine(params[0], params[1]);
//                 */
//                try {
//                    return combine(params[0], params[1]);
//                } catch (Exception e) {
//                    Log.d(TAG, "combine error");
//                    hasError = true;
//                    return null;
//                }
//                /* @} */
//            }
//
//            @Override
//            protected void onPostExecute(Bitmap bitmap) {
//                // SPRD: Fix bug 559046 if error happens, do nothing.
//                if (hasError) return;
//
//                /*SPRD:fix bug 545546 JavaCrash:java.lang.NullPointerException @{*/
//                if (mContext != null && (mContext instanceof CameraActivity)) {
//                    CameraActivity cameraActivity = (CameraActivity)mContext;
//                    if (cameraActivity.isDestroyed()) {
//                        return;
//                    }
//                }
//                /* @} */
//
//                if (bitmap == null) {
//                    setStateInternal(state, callListener);
//                } else {
//                    setImageBitmap(bitmap);
//
//                    int offset;
//
//                    if (mAnimDirection == ANIM_DIRECTION_VERTICAL) {
//                        offset = (mParentSize + getHeight()) / 2;
//                    } else if (mAnimDirection == ANIM_DIRECTION_HORIZONTAL) {
//                        offset = (mParentSize + getWidth()) / 2;
//                    } else {
//                        return;
//                    }
//
//                    mAnimator.setFloatValues(-offset, 0.0f);
//                    AnimatorSet s = new AnimatorSet();
//                    s.play(mAnimator);
//                    s.addListener(new AnimatorListenerAdapter() {
//                        @Override
//                        public void onAnimationStart(Animator animation) {
//                            setClickEnabled(false);
//                            // SPRD Bug:519334 Refactor Rotation UI of Camera.
//                            stateAnim = true;
//                        }
//
//                        @Override
//                        public void onAnimationEnd(Animator animation) {
//                            /* SPRD: fix bug 545546 and 582818, JavaCrash:java.lang.NullPointerException @{*/
//                            if (mContext != null && (mContext instanceof CameraActivity)) {
//                                CameraActivity cameraActivity = (CameraActivity)mContext;
//                                if (cameraActivity.isPaused()) {
//                                    return;
//                                }
//                            }
//                            /* @} */
//                            setStateInternal(state, callListener);
//                            setClickEnabled(true);
//                            // SPRD Bug:519334 Refactor Rotation UI of Camera.
//                            stateAnim = false;
//                        }
//                    });
//                    s.start();
//                }
//            }
//        /**
//         * SPRD: Fix bug 599084 @{
//         * original code
//        }.execute(mState, state);
//         */
//        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, mState, state);
        /* @} */
    }

    /**
     * Enable or disable click reactions for this button
     * without affecting visual state.
     * For most cases you'll want to use {@link #setEnabled(boolean)}.
     * @param enabled True if click enabled, false otherwise.
     */
    public void setClickEnabled(boolean enabled) {
        mClickEnabled = enabled;
    }

    private void setStateInternal(int state, boolean callListener) {
        mState = state;
        /*
         * SPRD: Fix bug 559046 overrideImageIds will reset mImageIds, which could result
         * ArrayIndexOutOfBoundsException or Resources$NotFoundException,
         * not add synchronization here, just try-catch it @{
         * original code
         *
        if (mImageIds != null) {
            setImageByState(mState);
        }

        if (mDescIds != null) {
            String oldContentDescription = String.valueOf(getContentDescription());
            String newContentDescription = getResources().getString(mDescIds[mState]);
            if (oldContentDescription != null && !oldContentDescription.isEmpty()
                    && !oldContentDescription.equals(newContentDescription)) {
                setContentDescription(newContentDescription);
                String announceChange = getResources().getString(
                    R.string.button_change_announcement, newContentDescription);
                announceForAccessibility(announceChange);
            }
        }
         */
        try {
            if (mImageIds != null) {
                setImageByState(mState);
            }

            if (mDescIds != null) {
                String oldContentDescription = String.valueOf(getContentDescription());
                String newContentDescription = getResources().getString(mDescIds[mState]);
                if (oldContentDescription != null && !oldContentDescription.isEmpty()
                        && !oldContentDescription.equals(newContentDescription)) {
                    setContentDescription(newContentDescription);
                    String announceChange = getResources().getString(
                            R.string.button_change_announcement, newContentDescription);
                    announceForAccessibility(announceChange);
                }
            }
        } catch (Exception e) {
            Log.d(TAG, "setStateInternal error");
            return;
        }
        /* @} */

        super.setImageLevel(mLevel);

        if (callListener && mOnStateChangeListener != null) {
            mOnStateChangeListener.stateChanged(MultiToggleImageButton.this, getState());
        }
    }

    private void nextState() {
        int state = mState + 1;
        if (state >= mImageIds.length) {
            state = 0;
        }
        setState(state);
    }

    protected void init() {
        this.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mContext == null){
                    return;
                }
                if (((CameraActivity) mContext).getCameraAppUI().getBursting()) {
                    Log.e(TAG,"can not switch camera when burst capture");
                    return;
                }
                if (((CameraActivity) mContext).getCurrentModule()
                        .isShutterClicked()) {
                    Log.e(TAG, "can not set when shutterbutton is clicked");
                    return;
                }
                if (!((CameraActivity) mContext).getCurrentModule()
                        .isCameraAvailable()) {
                    Log.e(TAG, "can not set when camera is not available");
                    return;
                }
                if (((CameraActivity) mContext).getCurrentModule()
                        .isPhotoFocusing()) {
                    Log.e(TAG, "can not set when camera is focusing");
                    return;
                }
                if (mClickEnabled) {
                    nextState();
                }
            }
        });
        setScaleType(ImageView.ScaleType.MATRIX);

        mAnimator = ValueAnimator.ofFloat(0.0f, 0.0f);
        mAnimator.setDuration(ANIM_DURATION_MS);
        mAnimator.setInterpolator(Gusterpolator.INSTANCE);
        mAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                mMatrix.reset();

                if (mAnimDirection == ANIM_DIRECTION_VERTICAL) {
                    mMatrix.setTranslate(0.0f, (Float) animation.getAnimatedValue());
                } else if (mAnimDirection == ANIM_DIRECTION_HORIZONTAL) {
                    mMatrix.setTranslate((Float) animation.getAnimatedValue(), 0.0f);
                }

                setImageMatrix(mMatrix);
                invalidate();
            }
        });
    }

    private void parseAttributes(Context context, AttributeSet attrs) {
        TypedArray a = context.getTheme().obtainStyledAttributes(
            attrs,
            R.styleable.MultiToggleImageButton,
            0, 0);
        int imageIds = a.getResourceId(R.styleable.MultiToggleImageButton_imageIds, 0);
        if (imageIds > 0) {
            overrideImageIds(imageIds);
        }
        int descIds = a.getResourceId(R.styleable.MultiToggleImageButton_contentDescriptionIds, 0);
        if (descIds > 0) {
            overrideContentDescriptions(descIds);
        }
        a.recycle();
    }

    /**
     * Override the image ids of this button.
     */
    public void overrideImageIds(int resId) {
        TypedArray ids = null;
        try {
            ids = getResources().obtainTypedArray(resId);
            mImageIds = new int[ids.length()];
            for (int i = 0; i < ids.length(); i++) {
                mImageIds[i] = ids.getResourceId(i, 0);
            }
        } finally {
            if (ids != null) {
                ids.recycle();
            }
        }

        if (mState >= 0 && mState < mImageIds.length) {
            setImageByState(mState);
        }
    }

    /**
     * Override the content descriptions of this button.
     */
    public void overrideContentDescriptions(int resId) {
        TypedArray ids = null;
        try {
            ids = getResources().obtainTypedArray(resId);
            mDescIds = new int[ids.length()];
            for (int i = 0; i < ids.length(); i++) {
                mDescIds[i] = ids.getResourceId(i, 0);
            }
        } finally {
            if (ids != null) {
                ids.recycle();
            }
        }
    }

    /**
     * Set size info (either width or height, as necessary) of the view containing
     * this button. Used for offset calculations during animation.
     * @param s The size.
     */
    public void setParentSize(int s) {
        mParentSize = s;
    }

    /**
     * Set the animation direction.
     * @param d Either ANIM_DIRECTION_VERTICAL or ANIM_DIRECTION_HORIZONTAL.
     */
    public void setAnimDirection(int d) {
        mAnimDirection = d;
    }

    @Override
    public void setImageLevel(int level) {
        super.setImageLevel(level);
        mLevel = level;
    }

    private void setImageByState(int state) {
        if (mImageIds != null) {
            setImageResource(mImageIds[state]);
        }
        super.setImageLevel(mLevel);
    }

    private Bitmap combine(int oldState, int newState) {
        // In some cases, a new set of image Ids are set via overrideImageIds()
        // and oldState or newState overrun the array.
        // check here for that.
        if (oldState >= mImageIds.length || newState >= mImageIds.length) {
            return null;
        }

        int width = getWidth();
        int height = getHeight();

        if (width <= 0 || height <= 0) {
            return null;
        }

        int[] enabledState = new int[] {android.R.attr.state_enabled};

        // new state
        Drawable newDrawable = getResources().getDrawable(mImageIds[newState]).mutate();
        newDrawable.setState(enabledState);

        // old state
        Drawable oldDrawable = getResources().getDrawable(mImageIds[oldState]).mutate();
        oldDrawable.setState(enabledState);

        // combine 'em
        Bitmap bitmap = null;

        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         */
        if (orientation == 0) {
            int bitmapHeight = (height * 2) + ((mParentSize - height) / 2);
            int oldBitmapOffset = height + ((mParentSize - height) / 2);
            bitmap = Bitmap.createBitmap(width, bitmapHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            newDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(),
                    newDrawable.getIntrinsicHeight());
            oldDrawable.setBounds(0, oldBitmapOffset,
                    oldDrawable.getIntrinsicWidth(),
                    oldDrawable.getIntrinsicHeight() + oldBitmapOffset);
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
        } else if (orientation == 90) {
            int bitmapHeight = (height * 2) + ((mParentSize - height) / 2);
            int oldBitmapOffset = height + ((mParentSize - height) / 2);
            bitmap = Bitmap.createBitmap(width, bitmapHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            oldDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(),
                    newDrawable.getIntrinsicHeight());
            newDrawable.setBounds(oldBitmapOffset, 0, oldBitmapOffset
                    + oldDrawable.getIntrinsicWidth(),
                    oldDrawable.getIntrinsicHeight());
            Log.i(TAG,
                    "combine bitmapHeight=" + bitmapHeight
                            + " ,oldBitmapOffset=" + oldBitmapOffset
                            + " ,height=" + height + " ,width=" + width
                            + " ,newDrawable.getIntrinsicWidth()"
                            + newDrawable.getIntrinsicWidth()
                            + " ,newDrawable.getIntrinsicHeight()="
                            + newDrawable.getIntrinsicHeight());
            canvas.rotate(orientation + 180, width / 2, width / 2);
            canvas.translate(-oldBitmapOffset, 0);
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
        } else if (orientation == 180) {
            int bitmapHeight = (height * 2) + ((mParentSize - height) / 2);
            int oldBitmapOffset = height + ((mParentSize - height) / 2);
            bitmap = Bitmap.createBitmap(width, bitmapHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            oldDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(),
                    newDrawable.getIntrinsicHeight());
            newDrawable.setBounds(0, oldBitmapOffset,
                    oldDrawable.getIntrinsicWidth(),
                    oldDrawable.getIntrinsicHeight() + oldBitmapOffset);
            canvas.rotate(180, width / 2, bitmapHeight / 2);
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
        } else if (orientation == 270) {
            int bitmapHeight = (height * 2) + ((mParentSize - height) / 2);
            int oldBitmapOffset = height + ((mParentSize - height) / 2);
            bitmap = Bitmap.createBitmap(width, bitmapHeight,
                    Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            newDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(),
                    newDrawable.getIntrinsicHeight());
            oldDrawable.setBounds(oldBitmapOffset, 0, oldBitmapOffset
                    + oldDrawable.getIntrinsicWidth(),
                    oldDrawable.getIntrinsicHeight());
            Log.i(TAG,
                    "combine bitmapHeight=" + bitmapHeight
                            + " ,oldBitmapOffset=" + oldBitmapOffset
                            + " ,height=" + height + " ,width=" + width
                            + " ,newDrawable.getIntrinsicWidth()"
                            + newDrawable.getIntrinsicWidth()
                            + " ,newDrawable.getIntrinsicHeight()="
                            + newDrawable.getIntrinsicHeight());
            canvas.rotate(orientation - 180, width / 2, width / 2);
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
            /* @} */
        } else if (mAnimDirection == ANIM_DIRECTION_VERTICAL) {
            int bitmapHeight = (height*2) + ((mParentSize - height)/2);
            int oldBitmapOffset = height + ((mParentSize - height)/2);
            bitmap = Bitmap.createBitmap(width, bitmapHeight, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            newDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(), newDrawable.getIntrinsicHeight());
            oldDrawable.setBounds(0, oldBitmapOffset, oldDrawable.getIntrinsicWidth(), oldDrawable.getIntrinsicHeight()+oldBitmapOffset);
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
        } else if (mAnimDirection == ANIM_DIRECTION_HORIZONTAL) {
            int bitmapWidth = (width*2) + ((mParentSize - width)/2);
            int oldBitmapOffset = width + ((mParentSize - width)/2);
            bitmap = Bitmap.createBitmap(bitmapWidth, height, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            newDrawable.setBounds(0, 0, newDrawable.getIntrinsicWidth(), newDrawable.getIntrinsicHeight());
            oldDrawable.setBounds(oldBitmapOffset, 0, oldDrawable.getIntrinsicWidth()+oldBitmapOffset, oldDrawable.getIntrinsicHeight());
            newDrawable.draw(canvas);
            oldDrawable.draw(canvas);
        }

        return bitmap;
    }

    /*
     * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
     */
    private int orientation = 0;

    public int getOrientation() {
        return orientation;
    }

    public void setOrientation(int orientation) {
        this.orientation = orientation;
    }
    /* @} */
}
