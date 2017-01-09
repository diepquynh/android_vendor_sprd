package com.dream.camera.modules.continuephoto;

import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.CameraActivity;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.camera.PhotoUI;
import android.view.View;
import com.android.camera2.R;
import android.widget.Toast;
import com.dream.camera.DreamModule;

import com.dream.camera.dreambasemodules.DreamPhotoModule;
import com.dream.camera.settings.DataModuleManager;

public class ContinuePhotoModule extends DreamPhotoModule {
    private static final Log.Tag TAG = new Log.Tag("ContinuePhotoModule");
    static int captureAlready = 0;

    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        showBurstHint();
    }

    //dream ui check 224
    public void showBurstHint() {
        boolean shouldBurstHint = mDataModule.getBoolean(Keys.KEY_CAMERA_BURST_HINT);
        if (shouldBurstHint == true) {
            Toast.makeText(mActivity, R.string.dream_continuephotomodule_module_warning,
                    Toast.LENGTH_LONG).show();
            mDataModule.set(Keys.KEY_CAMERA_BURST_HINT, false);
        }
    }

    public ContinuePhotoModule(AppController app) {
        super(app);
        captureAlready = 0;
    }

    @Override
    public void resume() {
//        mActivity.getSettingsManager().set(SettingsManager.SCOPE_GLOBAL,
//                Keys.KEY_CAMERA_CONTINUE_CAPTURE, "ninetynine");
        //mActivity.getCameraAppUI().setBottomBarRightUI(View.VISIBLE);
        super.resume();
        mBurstNotShowShutterButton = false;
        mBurstCaptureType = -1;
        mAppController.getCameraAppUI().setShutterButtonEnabled(true);
        ((ContinuePhotoUI) mUI).changeOtherUIVisible(false, View.VISIBLE);
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        return new ContinuePhotoUI(activity, this,
                activity.getModuleLayoutRoot());
    }

    @Override
    public void pause() {
//        mActivity.getSettingsManager().set(
//                SettingsManager.SCOPE_GLOBAL,
//                Keys.KEY_CAMERA_CONTINUE_CAPTURE,
//                mActivity.getResources().getString(
//                        R.string.pref_camera_burst_entry_defaultvalue));
        ((ContinuePhotoUI) mUI).changeExtendPanelUI(View.GONE);
        mAppController.getCameraAppUI().setBursting(false);
        super.pause();
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }

    @Override
    protected void doSomethingWhenonPictureTaken() {
        captureAlready++;
        ((ContinuePhotoUI) mUI).updateCaptureUI("" + captureAlready);
        if (captureAlready == mBurstCount) {
            handleActionUp();
        }
        if (mBurstNotShowShutterButton) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mUI.showBurstScreenHint(captureAlready);
                }
            });
        }
    }

    @Override
    protected void doSomethingWhenonShutterButtonClick() {
        // show extend panel xx/99
    }

    @Override
    protected void doSomethingWhenonBrustStart() {
        captureAlready = 0;
        ((ContinuePhotoUI) mUI).updateCaptureUI("" + captureAlready);
        ((ContinuePhotoUI) mUI).changeExtendPanelUI(View.VISIBLE);
        ((ContinuePhotoUI) mUI).changeOtherUIVisible(true, View.INVISIBLE);
    }

    @Override
    protected void handleActionDown(int action) {
        if (mBurstCaptureType != -1) {
            Log.e(TAG, "handleActionDown is running while handleActionUp not");
            return;
        }
        //Sprd Fix bug:665197
        if (mUI.isZooming()){
            Log.i(TAG, "camera can not burst capture when zooming");
            return;
        }
        ((ContinuePhotoUI) mUI).changeOtherUIVisible(true, View.INVISIBLE);
        super.handleActionDown(action);
    }

    @Override
    protected void handleActionUp() {
        // show something
        //if(mBurstMode) {
            getHandler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    ((ContinuePhotoUI) mUI).changeExtendPanelUI(View.GONE);
                }
            }, 1000);
            ((ContinuePhotoUI) mUI).changeOtherUIVisible(false, View.VISIBLE);
        //}
        super.handleActionUp();
    }

    @Override
    public void touchCapture(){

    }

    @Override
    public void updateBatteryLevel(int level) {
        // if you don't want open the flah in this module do not call super method
        Log.d(TAG, "updateBatteryLevel: donothing");
    }

    @Override
    public void onHideBurstScreenHint() {
        if(mBurstNotShowShutterButton || captureAlready == mBurstCount) {
            getHandler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "onHideBurstScreenHint()");
                    mUI.dismissBurstScreenHit();
                    mBurstNotShowShutterButton = false;
                    mAppController.getCameraAppUI().setShutterButtonEnabled(true);
                    if(captureAlready == mBurstCount) {
                        ((ContinuePhotoUI) mUI).changeExtendPanelUI(View.GONE);
                        ((ContinuePhotoUI) mUI).changeOtherUIVisible(false, View.VISIBLE);
                    }
                }
            }, 1500);
        }
        if (mAppController.getCameraAppUI().isInFreezeReview()) {// SPRD :BUG
            // 398284
            mAppController.getCameraAppUI().setSwipeEnabled(false);
            mAppController.getCameraAppUI().onShutterButtonClick();
        } else {
            mAppController.getCameraAppUI().setSwipeEnabled(true);
            mAppController.getCameraAppUI().showModeOptions();
        }
    }

    @Override
    public boolean isBurstCapturing() {
        return ((ContinuePhotoUI) mUI).getExtendPanelVisibility() || mUI.getBurstHintVisibility();
    }

    @Override
    public int getModuleTpye() {
        return DreamModule.CONTINUE_MODULE;
    }
    /*SPRD:Fix bug651120, burst camera and switch camera*/
    @Override
    public boolean isShutterClicked(){
        return !mAppController.isShutterEnabled() || isBurstCapturing();
    }
}
