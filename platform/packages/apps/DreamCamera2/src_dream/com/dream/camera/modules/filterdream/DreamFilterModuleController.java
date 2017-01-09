
package com.dream.camera.modules.filterdream;

import android.os.Handler;
import com.android.camera.CameraActivity;
import com.android.camera.CameraModule;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.debug.Log;
import com.android.camera.ui.MainActivityLayout;
import com.android.camera.ui.ModeTransitionView;
import com.android.camera.util.CameraUtil;
import com.android.camera2.R;
import com.ucamera.ucam.modules.ufilter.UcamFilterPhotoModule;
import com.dream.camera.modules.filterdream.DreamFilterModule;

import android.graphics.Bitmap;
import android.graphics.Canvas;

public class DreamFilterModuleController {
    private static final Log.Tag TAG = new Log.Tag("DreamFilterModuleController");
    private AppController mAppController;
    private final static int SWIPE_LEFT = 3;
    private final static int SWIPE_RIGHT = 4;
    private int mPhotoIndex;
    private int mFilterIndex;
    private Bitmap mScreenShot;
    private final ModeTransitionView mModeTransitionView;
    private final MainActivityLayout mAppRootView;
    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    private final String TARGET_CAMERA_UNFREEZE_FRAME_COUNT = "persist.sys.cam.unfreeze_cnt";
    private final int MAX_FRAME_COUNT = android.os.SystemProperties.getInt(
            TARGET_CAMERA_UNFREEZE_FRAME_COUNT, 0);
    private int mFrameCount = 0;
    /* @} */
    private UcamFilterPhotoModule mFilterModule;
    private CameraAppUI mCameraAppUI;

    public DreamFilterModuleController(AppController controller, MainActivityLayout appRootView) {
        mAppController = controller;
        mAppRootView = appRootView;
        mPhotoIndex = mAppController.getAndroidContext().getResources()
                .getInteger(R.integer.camera_mode_auto_photo);
        mFilterIndex = mAppController.getAndroidContext().getResources()
                .getInteger(R.integer.camera_mode_filter);
        mModeTransitionView = (ModeTransitionView) mAppRootView
                .findViewById(R.id.mode_transition_view);
    }

    public boolean switchMode(int swipeState) {
        if (swipeState == SWIPE_LEFT && mAppController != null
                && mAppController.getCurrentModuleIndex() == mPhotoIndex) {
            mAppController.freezeScreenUntilPreviewReady();
            mAppController.onModeSelected(mFilterIndex);
            return true;
        } else if (swipeState == SWIPE_RIGHT && mAppController != null
                && mAppController.getCurrentModuleIndex() == mFilterIndex) {
                mFilterModule = (DreamFilterModule)((CameraActivity)mAppController).getCurrentModule();
                /* SPRD: Fix bug 597195 the freeze screen for switch mode @{ */
                //mFilterModule.delayUiPause();
                mFilterModule.freezeScreen(CameraUtil.isFreezeBlurEnable(), false);
                new Handler().post(new Runnable() {
                    @Override
                    public void run() {
                        if (!mAppController.isPaused()){
                            mAppController.onModeSelected(mPhotoIndex);
                        }
                    }
                });
                /* @} */
            return true;
        }
        return false;
    }

    private void setScreenShot() {
        if (mFilterModule != null) {
            // disable preview data callback to pause preview
           // mFilterModule.setPreviewDataCallback(false);
            Bitmap preview = mFilterModule.getPreviewBitmap();
            mScreenShot = Bitmap.createBitmap(getCameraAppUI().getModuleRootView().getWidth(),getCameraAppUI().getModuleRootView().getHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(mScreenShot);
            canvas.drawARGB(255, 0, 0, 0);
            if (preview != null) {
                canvas.drawBitmap(preview, null,
                        getCameraAppUI().getPreviewArea(), null);
                preview.recycle();
            }
            Bitmap overlay = getCameraAppUI().getPreviewOverlayAndControls();
            if (overlay != null) {
                canvas.drawBitmap(overlay, 0f, 0f, null);
                overlay.recycle();
            }
        }
    }

    private void freezeScreen(Bitmap screenShot) {
        // SPRD: nj dream camera monkey test
        if (screenShot != null) {
            Log.i(TAG,"freezeScreen");
            mModeTransitionView.setupModeCover(screenShot);
        }
        resetFrameCount();
    }

    public void checkFrameCount() {
        if (mFrameCount < MAX_FRAME_COUNT) {
            mFrameCount++;
        } else if (mFrameCount == MAX_FRAME_COUNT) {
            mFrameCount++;
            Log.i(TAG,"hideImageCover");
            // TODO if you want setup mode Cover in filter Module
            // you should notice this palace
            // mModeTransitionView.hideImageCover();
            getCameraAppUI().hideModeCover();
            /*SPRD:fix bug612383 filter not need textureview @ {*/
            if (mAppController != null && mAppController.getCurrentModuleIndex() == mFilterIndex) {
                getCameraAppUI().pauseTextureViewRendering();
            }
            /* @} */
        }
    }

    public void resetFrameCount() {
        mFrameCount = 0;
    }

    public CameraAppUI getCameraAppUI() {
        if (mCameraAppUI == null) {
            mCameraAppUI = mAppController.getCameraAppUI();
        }
        return mCameraAppUI;
    }

    /* SPRD: nj dream camera test debug 120 @{ */
    public void updateFilterPanelUI(int visible) {
        if (mAppController != null
                && mAppController.getCurrentModuleIndex() == mFilterIndex) {
            ((DreamFilterModule) ((CameraActivity) mAppController)
                    .getCurrentModule()).updateFilterPanelUI(visible);
        }
    }
    /* @} */
}
