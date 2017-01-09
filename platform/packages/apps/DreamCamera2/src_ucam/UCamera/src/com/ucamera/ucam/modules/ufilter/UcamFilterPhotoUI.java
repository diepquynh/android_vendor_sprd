/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.ucamera.ucam.modules.ui;

import android.content.Context;
import android.graphics.RectF;
import android.os.Handler;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.util.CameraUtil;
import com.android.camera.widget.ModeOptionsOverlay;
import com.android.camera2.R;
import com.thundersoft.advancedfilter.SmallAdvancedFilter;
import com.thundersoft.advancedfilter.TsAdvancedFilterGLView;
import com.thundersoft.advancedfilter.TsAdvancedFilterNative;


public class UcamFilterPhotoUI extends BasicUI implements
        CameraAppUI.PanelsVisibilityListener,
        OrientationManager.OnOrientationChangeListener {
    private static final Log.Tag TAG = new Log.Tag("UcamFilterPhotoUI");
    private ImageView mFilterButton;

    private ModeOptionsOverlay mModeOptionsOverlay;
    protected TsAdvancedFilterGLView mFilterGLView;
    protected SmallAdvancedFilter mSmallAdvancedFilter;
    // SPRD: Fix bug 574390 Quick click two times filter, no filter interface,Filter effect diagram disappear
    private static Boolean mPaused = false;
    private Boolean mDelayUiPause = false;
    private int mLastDisplayOrientation = 0;

    public static final int FILTER_FREEZE_SCREEN_DELAY_TIME = 1500;

    public UcamFilterPhotoUI(int cameraId, CameraActivity activity, PhotoController controller, View parent) {
        super(cameraId, activity, controller, parent);
        // TODO Auto-generated constructor stub
        int[] types = {
                TsAdvancedFilterNative.ADVANCEDFILTER_FILM,
                TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,
                TsAdvancedFilterNative.ADVANCEDFILTER_BLACKWHITE,
                TsAdvancedFilterNative.ADVANCEDFILTER_FRESH,
                TsAdvancedFilterNative.ADVANCEDFILTER_HOPE,
                TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION,
                TsAdvancedFilterNative.ADVANCEDFILTER_INVERT,
                TsAdvancedFilterNative.ADVANCEDFILTER_1839,
                /************* add new filters ********************/
                TsAdvancedFilterNative.ADVANCEDFILTER_GREEN,
                TsAdvancedFilterNative.ADVANCEDFILTER_SKETCH,
                TsAdvancedFilterNative.ADVANCEDFILTER_NEGATIVE,
                TsAdvancedFilterNative.ADVANCEDFILTER_POSTERIZE,
                TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,
                TsAdvancedFilterNative.ADVANCEDFILTER_RED,
                TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_LEFT,
                TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT,
                TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_UP,
                TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_DOWN,
                TsAdvancedFilterNative.ADVANCEDFILTER_REVERSAL,
                TsAdvancedFilterNative.ADVANCEDFILTER_JAPAN,
                TsAdvancedFilterNative.ADVANCEDFILTER_BRIGHT,
                TsAdvancedFilterNative.ADVANCEDFILTER_CUTE,
                TsAdvancedFilterNative.ADVANCEDFILTER_BLUE,
                TsAdvancedFilterNative.ADVANCEDFILTER_FLOWERINESS,
                TsAdvancedFilterNative.ADVANCEDFILTER_FLY,
                TsAdvancedFilterNative.ADVANCEDFILTER_LOTUS,
                TsAdvancedFilterNative.ADVANCEDFILTER_BLESS,
                TsAdvancedFilterNative.ADVANCEDFILTER_SPARKLING,
                TsAdvancedFilterNative.ADVANCEDFILTER_HAPPY,
                TsAdvancedFilterNative.ADVANCEDFILTER_BLOOM,
                TsAdvancedFilterNative.ADVANCEDFILTER_MOSAIC,
                TsAdvancedFilterNative.ADVANCEDFILTER_EDGE,
                TsAdvancedFilterNative.ADVANCEDFILTER_COLORPENCIL,
                TsAdvancedFilterNative.ADVANCEDFILTER_GRAYSKETCHPENCIL,
                TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW,
                TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN,
                TsAdvancedFilterNative.ADVANCEDFILTER_DUSK,
                TsAdvancedFilterNative.ADVANCEDFILTER_NEON,
                TsAdvancedFilterNative.ADVANCEDFILTER_PENCIL1,
                TsAdvancedFilterNative.ADVANCEDFILTER_PENCIL2,
                TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS,
                TsAdvancedFilterNative.ADVANCEDFILTER_THERMAL,
                TsAdvancedFilterNative.ADVANCEDFILTER_CRAYON,
                TsAdvancedFilterNative.ADVANCEDFILTER_ENGRAVING,
                TsAdvancedFilterNative.ADVANCEDFILTER_KOREA,
                TsAdvancedFilterNative.ADVANCEDFILTER_AMERICA,
                TsAdvancedFilterNative.ADVANCEDFILTER_FRANCE,
                TsAdvancedFilterNative.ADVANCEDFILTER_DESERT,
                TsAdvancedFilterNative.ADVANCEDFILTER_JIANGNAN,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP1,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP2,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP3,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP4,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP5,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP6,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP7,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP8,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP9,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP10,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP11,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP12,
                TsAdvancedFilterNative.ADVANCEDFILTER_PEEP13
        };
        TsAdvancedFilterNative.init(types);
        // SPRD: Fix bug 578679,  Add Filter type of symmetry right.
        mActivity.getOrientationManager().addOnOrientationChangeListener(this);

        /*mActivity.getLayoutInflater().inflate(R.layout.ucam_magiclens_module,
                (ViewGroup) mRootView, true);*/

        mFilterGLView = (TsAdvancedFilterGLView) mRootView
                .findViewById(R.id.advanced_filter_glview);
        mFilterGLView.setVisibility(View.VISIBLE);

        //mRootView.findViewById(R.id.cpu_small_effects_layout_id).setVisibility(View.VISIBLE);

        mSmallAdvancedFilter = new SmallAdvancedFilter(mActivity, (ViewGroup) mRootView);
        mSmallAdvancedFilter.setVisibility(View.VISIBLE);

        // SPRD: SPRD: Fix bug 578679,  Add Filter type of symmetry right.
        mSmallAdvancedFilter.setOrientation(mActivity.getCameraAppUI().getNowOrientation());
        initFilterButton();
    }

    @Override
    public void setTransformMatrix() {
        float scaledTextureWidth, scaledTextureHeight;
        if (mAspectRatio == 0) {
            return;
        }
        if (mWidth > mHeight) {
            scaledTextureWidth = Math.min(mWidth, (int) (mHeight * mAspectRatio + 0.5));//SPRD:fix bug601061
            scaledTextureHeight = Math.min(mHeight, (int) (mWidth / mAspectRatio + 0.5));
        } else {
            scaledTextureWidth = Math.min(mWidth, (int) (mHeight / mAspectRatio + 0.5));
            scaledTextureHeight = Math.min(mHeight, (int) (mWidth * mAspectRatio + 0.5));
        }
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mFilterGLView.getLayoutParams();
        params.width = (int)scaledTextureWidth;;
        params.height = (int)scaledTextureHeight;;
        RectF rect = mActivity.getCameraAppUI().getPreviewArea();

        // horizontal direction
        if (mWidth > mHeight) {
            params.setMargins((int) rect.left, 0, 0, 0);
        } else {
            params.setMargins(0, (int) rect.top, 0, 0);
        }
        mFilterGLView.setLayoutParams(params);

        int currentDisplayOrientation = CameraUtil.getDisplayRotation();
        if ((mLastDisplayOrientation != 0 && currentDisplayOrientation == 0)) {
            // Set of 'gone then visible' is same as requestLayout(), but requestLayout does not work sometimes.
            // The 30ms delay is needed, or set of 'gone then visible' will be useless.
            if (mFilterGLView.getVisibility() == View.VISIBLE) {
                mFilterGLView.setVisibility(View.GONE);
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mFilterGLView.setVisibility(View.VISIBLE);
                    }
                }, 30);
            }
        }
        mLastDisplayOrientation = currentDisplayOrientation;

        Log.i(TAG, "setTransformMatrix(): width = " + mWidth
                + " height = " + mHeight
                + " scaledTextureWidth = " + scaledTextureWidth
                + " scaledTextureHeight = " + scaledTextureHeight
                + " mAspectRatio = " + mAspectRatio
                + " displayOrientation = " + currentDisplayOrientation);
    }

    @Override
    public void updateUI(float aspectRation) {
        super.updateUI(aspectRation);
        mSmallAdvancedFilter.updateUI(aspectRation);
    }

    public TsAdvancedFilterGLView getAdvancedFilterGLView() {//qiao
        return mFilterGLView;
    }

    public SmallAdvancedFilter getSmallAdvancedFilter() {
        return mSmallAdvancedFilter;
    }

    private void initFilterButton() {
        Log.i(TAG,"initFilterButton");
        mFilterButton = (ImageView) mRootView.findViewById(R.id.btn_filter_button);
        mFilterButton.setVisibility(View.VISIBLE);
        setButtonOrientation(mActivity.getOrientationManager().getDisplayRotation());
    }

    @Override
    public void setButtonOrientation(OrientationManager.DeviceOrientation DeviceOrientation) {
        Log.i(TAG, "setButtonOrientation orientation = " + DeviceOrientation);

        FrameLayout.LayoutParams layoutFilter = new FrameLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,Gravity.CENTER);

        float margin =mActivity.getResources().getDimension(R.dimen.filter_make_up_button_magin);
        float buttomsize = mActivity.getResources().getDimension(R.dimen.filter_make_up_button_half_size);

        WindowManager wm = (WindowManager)mActivity.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        int width = display.getWidth();
        int height = display.getHeight();
        float mPort = width/2 - margin - buttomsize;
//        float mLand = height/2 - margin - buttomsize;
//
//        if (DeviceOrientation == DeviceOrientation.CLOCKWISE_0){
//            layoutFilter.setMargins(0,0,(int)mPort,0);
//        } else if(DeviceOrientation == DeviceOrientation.CLOCKWISE_90){
//            layoutFilter.setMargins(0,(int)mLand,0,0);
//        } else if(DeviceOrientation == DeviceOrientation.CLOCKWISE_180){
//            layoutFilter.setMargins((int)mPort,0,0,0);
//        } else if(DeviceOrientation == DeviceOrientation.CLOCKWISE_180){
//            layoutFilter.setMargins(0,0,0,(int)mLand);
//        }
        layoutFilter.setMargins(0,0,(int)mPort,0);
        mFilterButton.setLayoutParams(layoutFilter);
    }

    @Override
    public void resetUI() {
        if (mModeOptionsOverlay != null) {
            mModeOptionsOverlay.setPadding(0,0,0,0);
        }
        super.resetUI();
    }

    public void initModeOptions() {
        int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.mode_option_overlay_filter_height);
        mModeOptionsOverlay = (ModeOptionsOverlay) mRootView.findViewById(R.id.mode_options_overlay);
        mModeOptionsOverlay.setPadding(0, 0, 0, bottom);
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    @Override
    public void onResume() {
        mPaused = false;
        mDelayUiPause = false;
        mFilterGLView.setVisibility(View.VISIBLE);
        mSmallAdvancedFilter.setVisibility(View.VISIBLE);
        mFilterButton.setVisibility(View.VISIBLE);
        mFilterButton.setEnabled(true);
        initModeOptions();
        super.onResume();
    }

    @Override
    public void onPause() {
        mPaused = true;
        if (mDelayUiPause) {
            new Handler().postDelayed(new Runnable() {
                public void run() {
                    if (mPaused) {
                        mFilterGLView.setVisibility(View.GONE);
                        mSmallAdvancedFilter.setVisibility(View.GONE);
                    }
                }
            }, FILTER_FREEZE_SCREEN_DELAY_TIME);
        } else {
            mFilterGLView.setVisibility(View.GONE);
            mSmallAdvancedFilter.setVisibility(View.GONE);
        }

        mFilterButton.setVisibility(View.GONE);
        super.onPause();
    }

    public void delayUiPause() {
        mDelayUiPause = true;
    }

    public void setFilterButtonEnabled(boolean enabled) {
        if (mFilterButton != null) {
            mFilterButton.setEnabled(enabled);
        }
    }
    /* @} */

    @Override
    public void onPanelsHidden() {
        if (mSmallAdvancedFilter != null) {
            mSmallAdvancedFilter.setVisibility(View.GONE);
        }
    }

    @Override
    public void showAudioNoteProgress() {
        super.showAudioNoteProgress();
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.GONE);
        }
    }

    @Override
    public void onPanelsShown() {
        if (mSmallAdvancedFilter != null) {
            mSmallAdvancedFilter.setVisibility(View.VISIBLE);
        }
    }

    public void hideAudioNoteProgress() {
        super.hideAudioNoteProgress();
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.VISIBLE);
        }
    }

    /* SPRD: Fix bug 578679,  Add Filter type of symmetry right. @{ */
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        int orientation = 0;
        if (mActivity.getOrientationManager() != null) {
            orientation = mActivity.getOrientationManager().getDeviceOrientation().getDegrees();
        }
        mSmallAdvancedFilter.setOrientation(orientation);
    }
    /* @} */
}
