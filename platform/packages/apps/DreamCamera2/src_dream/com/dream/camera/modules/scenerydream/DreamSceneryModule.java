
package com.dream.camera.modules.scenerydream;

import java.util.HashMap;

import com.android.camera.CameraActivity;
import com.android.camera.app.AppController;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.dreambasemodules.DreamBasicUI;
import com.ucamera.ucam.modules.ui.BasicUI;
import com.ucamera.ucam.modules.uscenery.SprdSceneryModule;
import com.android.camera.PhotoController;
import android.view.View;
import android.widget.Toast;
import com.android.camera2.R;
import com.android.camera.debug.Log;
import com.android.camera.debug.Log.Tag;
import com.android.camera.settings.Keys;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.GcamHelper;

public class DreamSceneryModule extends SprdSceneryModule{
    public static final String DREAMSCENERY_MODULE_STRING_ID = "CAM_DreamSceneryModule";
    private static final Tag TAG = new Tag(DREAMSCENERY_MODULE_STRING_ID);

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        //showToastFirstTime();//SPRD:fix bug605019
    }

    @Override
    public void showToastFirstTime() {
        Toast.makeText(mActivity, R.string.dream_scenery_module_warning,Toast.LENGTH_LONG).show();
    }

    public DreamSceneryModule(AppController app) {
        super(app);
    }

    @Override
    protected void switchCamera() {
        // super.switchCamera();
        mActivity.switchFrontAndBackMode();

        mActivity.getCameraAppUI().updateModeList();

    }

    @Override
    public void makeModuleUI(PhotoController controller, View parent) {
        Log.i(TAG, "makeModuleUI S.");
        // SPRD: nj dream camera test debug 55
        initializeModuleControls();
        getUcamUI();
        updateSceneryMenuLayout();
        Log.i(TAG, "makeModuleUI E.");
    }

    public void updateSceneryMenuLayout() {
        if (mSceneryMenuLayout != null) {
            int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.scenery_menu_bottom_height);
            mSceneryMenuLayout.setPadding(mSceneryMenuLayout.getPaddingLeft(), mSceneryMenuLayout.getPaddingTop(), mSceneryMenuLayout.getPaddingRight(), bottom);
        }
    }

    @Override
    public int getMode() {
        return DreamUtil.PHOTO_MODE;
    }

    @Override
    public void pause() {
        super.pause();
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        Log.e(TAG, "dreamPhotoonDreamSettingChangeListener  ");
        if (mCameraDevice == null) {
            return;
        }
        DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();

        for (String key : keys.keySet()) {
            Log.e(TAG,
                    "onSettingChanged key = " + key + " value = "
                            + keys.get(key));
            switch (key) {
            case Keys.KEY_PICTURE_SIZE_BACK:
            case Keys.KEY_PICTURE_SIZE_FRONT:
                updateParametersPictureSize();
                break;
            case Keys.KEY_JPEG_QUALITY:
                updateParametersPictureQuality();
                break;
            case Keys.KEY_CAMERA_COMPOSITION_LINE:
                break;
            case Keys.KEY_CAMERA_AI_DATECT:
                updateFace();
                // SPRD:Modify for ai detect
                mUI.intializeAIDetection(mDataModuleCurrent);
                break;
            case Keys.KEY_FREEZE_FRAME_DISPLAY:
                break;
            case Keys.KEY_FLASH_MODE:
                if (mPaused
                        || mAppController.getCameraProvider()
                                .waitingForCamera()) {
                    return;
                }
                updateParametersFlashMode();
                break;
            case Keys.KEY_COUNTDOWN_DURATION:
                break;
            case Keys.KEY_FRONT_CAMERA_MIRROR:
                updateParametersMirror();
                break;
            case Keys.KEY_CAMERA_TIME_STAMP:
                updateTimeStamp();
                break;

            }
        }
        mActivity.getCameraAppUI().initSidePanel();

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }
    }

    @Override
    protected void setSceneSelectFrameView(int currentSceneMode) {
        super.setSceneSelectFrameView(currentSceneMode);
        mDataModuleCurrent.set(Keys.KEY_CAMERA_SCENERY_TYPE, currentSceneMode);
    }

    @Override
    public boolean isCameraMirror() {
        return mDataModuleCurrent.getBoolean(Keys.KEY_FRONT_CAMERA_MIRROR);
    }

    @Override
    public BasicUI getUcamUI(){
        if (mUI == null)
            mUI = new DreamSceneryUI(mCameraId, this, mActivity, mActivity.getModuleLayoutRoot());

        return mUI;
    }
    //SPDR: fix bug:606454 update thumbnail
    public boolean isFreezeFrameDisplayShow() {
        if(sFreezeFrameControl != null && sFreezeFrameControl.mFreezeVisible)
            return true;

        return false;
    }
}
