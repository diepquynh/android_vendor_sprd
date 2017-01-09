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

package com.dream.camera.modules.filterdream;

import android.os.Handler;
import android.view.Gravity;
import android.view.View;
import android.content.Context;
import android.view.Display;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.ImageButton;
import android.view.ViewGroup;
import android.view.LayoutInflater;

import com.android.camera.settings.Keys;
import com.android.camera.app.CameraAppUI;
import com.android.camera.util.CameraUtil;
import com.android.camera2.R;

import com.android.camera.CameraActivity;
import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.PhotoController;
import com.dream.camera.DreamUI;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.dreambasemodules.DreamBasicUI;
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.util.DreamUtil;

import android.widget.FrameLayout;
import android.graphics.RectF;
import com.thundersoft.advancedfilter.TsAdvancedFilterNative;
import com.thundersoft.advancedfilter.SmallAdvancedFilter;
import com.thundersoft.advancedfilter.TsAdvancedFilterGLView;


public class DreamFilterModuleUI extends DreamBasicUI implements DreamInterface, 
        OrientationManager.OnOrientationChangeListener,
        CameraAppUI.PanelsVisibilityListener{
    private static final Log.Tag TAG = new Log.Tag("DreamFilterModuleUI");

    private TsAdvancedFilterGLView mFilterGLView;
    private SmallAdvancedFilter mSmallAdvancedFilter;
    private static Boolean mPaused = false;
    private boolean mDelayUiPause = false;
    private View topPanel;
    private ImageButton tSettingsButton;
    private int mLastDisplayOrientation = 0;


    public DreamFilterModuleUI(int cameraId, CameraActivity activity,
            PhotoController controller, View parent) {
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
                TsAdvancedFilterNative.ADVANCEDFILTER_1839
                /************* add new filters ********************/
                , TsAdvancedFilterNative.ADVANCEDFILTER_GREEN,
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
    }

    @Override
    public void setTransformMatrix() {
        float scaledTextureWidth, scaledTextureHeight;
        if (mAspectRatio == 0) {
            return;
        }
        if (mWidth > mHeight) {
            scaledTextureWidth = Math.min(mWidth,
                    (int) (mHeight * mAspectRatio + 0.5));//SPRD:fix bug601061
            scaledTextureHeight = Math.min(mHeight,
                    (int) (mWidth / mAspectRatio + 0.5));
        } else {
            scaledTextureWidth = Math.min(mWidth,
                    (int) (mHeight / mAspectRatio + 0.5));
            scaledTextureHeight = Math.min(mHeight,
                    (int) (mWidth * mAspectRatio + 0.5));
        }
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mFilterGLView
                .getLayoutParams();
        params.width = (int) scaledTextureWidth;
        ;
        params.height = (int) scaledTextureHeight;
        ;
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
            // Set of 'gone then visible' is same as requestLayout(), but
            // requestLayout does not work sometimes.
            // The 30ms delay is needed, or set of 'gone then visible' will be
            // useless.
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

        Log.i(TAG, "setTransformMatrix(): width = " + mWidth + " height = "
                + mHeight + " scaledTextureWidth = " + scaledTextureWidth
                + " scaledTextureHeight = " + scaledTextureHeight
                + " mAspectRatio = " + mAspectRatio + " displayOrientation = "
                + currentDisplayOrientation);
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

    @Override
    public void resetUI() {
        super.resetUI();
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    @Override
    public void onResume() {
        Log.i(TAG, "onResume");
        mPaused = false;
        mDelayUiPause = false;
        mFilterGLView.setVisibility(View.VISIBLE);
        mSmallAdvancedFilter.setVisibility(View.VISIBLE);
        super.onResume();
    }

    @Override
    public void onPause() {
        mPaused = true;
        super.onPause();
    }

    /* SPRD:fix bug605522 filter mode will normal preview when back camera @{*/
    public void destroy() {
        if (mPaused) {
            mFilterGLView.setVisibility(View.GONE);
            mSmallAdvancedFilter.setVisibility(View.GONE);
            Log.i(TAG, "GLView Filter setVisibility(View.GONE)");
            mSmallAdvancedFilter.removeOnReceiveBufferListener();
        }
    }
    /* @} */

    public void delayUiPause() {
        Log.i(TAG,"delayUiPausedelayUiPause");
        mDelayUiPause = true;
    }

    @Override
    public void onPanelsHidden() {
        if (mSmallAdvancedFilter != null) {
            mSmallAdvancedFilter.setVisibility(View.GONE);
        }
    }

    @Override
    public void onPanelsShown() {
        if (mSmallAdvancedFilter != null) {
            mSmallAdvancedFilter.setVisibility(View.VISIBLE);
        }
    }

    private void bindTopButtons() {
        mActivity.getButtonManager().load(topPanel);

        tSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(tSettingsButton);
        bindFlashButton();
        bindCountDownButton();
        bindCameraButton();
    }
    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        // TODO Auto-generated method stub
        Log.i(TAG, "fitTopPanel");
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.filterphoto_top_panel,
                        topPanelParent);
            }
            bindTopButtons();
        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.filterphoto_front_top_panel,
                        topPanelParent);
            }
            mActivity.getButtonManager().load(topPanel);

            tSettingsButton = (ImageButton) topPanel
                    .findViewById(R.id.settings_button_dream);

            bindSettingsButton(tSettingsButton);
            bindCountDownButton();
            bindCameraButton();
        }
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
        // TODO Auto-generated method stub

    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(SlidePanelManager.FILTER,View.VISIBLE);
        SlidePanelManager.getInstance(mActivity).focusItem(
                SlidePanelManager.FILTER, false);
    }

    @Override
    public void updateSidePanel() {
        sidePanelMask = DreamUtil.SP_EXTERNAL_STORAGE | DreamUtil.SP_INTERNAL_STORAGE
                | DreamUtil.SP_USB_STORAGE | DreamUtil.SP_FACE_DETECT
                | DreamUtil.SP_LOCATE | DreamUtil.SP_FILTER
                | DreamUtil.SP_TOUCH_CAPTURE | DreamUtil.SP_OPPOSITE
                | DreamUtil.SP_CA;
    }
    /* @} */
    /* SPRD: Fix bug 578679,  Add Filter type of symmetry right. @{ */
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        int orientation = 0;
        if (mActivity.getOrientationManager() != null) {
            orientation = mActivity.getOrientationManager().getDeviceOrientation().getDegrees();
            Log.d(TAG, "onOrientationChanged: orientation = " + orientation);
        }
        mSmallAdvancedFilter.setOrientation(orientation);
    }
    /* @} */

    @Override
    public int getUITpye() {
        return DreamUI.DREAM_FILTER_UI;
    }
}
