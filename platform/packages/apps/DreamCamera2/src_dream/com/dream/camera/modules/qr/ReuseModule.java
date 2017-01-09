package com.dream.camera.modules.qr;

import java.util.Vector;

import com.android.camera.ButtonManager;
import com.android.camera.CameraActivity;
import com.android.camera.CameraModule;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.app.CameraProvider;
import com.android.camera.app.CameraAppUI.BottomBarUISpec;
import com.android.camera.debug.Log;
import com.android.camera.hardware.HardwareSpec;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.TouchCoordinate;
import android.hardware.Camera;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.View;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataStructSetting;
import com.dream.camera.util.DreamUtil;
import com.ucamera.ucam.sound.ShutterSound;
import com.dream.camera.modules.qr.ReuseController;
import java.util.HashMap;

public class ReuseModule extends CameraModule implements ReuseController ,ResetListener, DreamSettingChangeListener{

    private ViewGroup mRootView;
    protected CameraActivity mActivity;
    private AppController mAppController;
    private CameraProxy mCameraDevice;
    private boolean mPaused;
    protected int mCameraId;
    protected DataModuleBasic mDataModule;
    protected boolean mQuickCapture;
    protected ShutterSound mShutterSound;
    protected DataModuleBasic mDataModuleCurrent;
    protected int mPendingSwitchCameraId = -1;

    public ReuseModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        //Log.i(TAG, "init Camera");
        mActivity = activity;
        // TODO: Need to look at the controller interface to see if we can get
        // rid of passing in the activity directly.
        mAppController = mActivity;

        // change the data storage module
        int cameraId = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getDataModuleCamera().getInt(Keys.KEY_CAMERA_ID);

        DataStructSetting dataSetting = new DataStructSetting(
                DreamUtil.intToString(getMode()), DreamUtil.isFrontCamera(
                        mAppController.getAndroidContext(), cameraId),
                mActivity.getCurrentModuleIndex(), cameraId);

        DataModuleManager.getInstance(mAppController.getAndroidContext())
                .changeModuleStatus(dataSetting);

        mDataModule = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getDataModuleCamera();

        mDataModuleCurrent = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();
        mActivity.getCameraAppUI().initSidePanel();
        /* @} */

        // SPRD initialize mJpegQualityController.
        //mJpegQualityController = new JpegQualityController();
        makeModuleUI(this, mActivity.getModuleLayoutRoot());

        mShutterSound = new ShutterSound(mActivity);
        mShutterSound.loadSounds(mActivity);
        // mActivity.setPreviewStatusListener(mUI);

        SettingsManager settingsManager = mActivity.getSettingsManager();
        initCameraID();
        mCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);
    }

    public final ButtonManager.ButtonCallback mFlashCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            if (state == 0) {
                enableTorchMode(true);
            } else {
                enableTorchMode(false);
            }
        }
    };

    public void enableTorchMode(boolean enable) {
        Camera camera = CameraManager.get().getCamera();
        if (camera == null) {
            return;
        }
        android.hardware.Camera.Parameters parameters = camera.getParameters();
        if (enable) {
            parameters.setFlashMode("torch");
        } else {
            parameters.setFlashMode("off");
        }
        camera.setParameters(parameters);
    }

    public final ButtonManager.ButtonCallback mCameraCallback =
            new ButtonManager.ButtonCallback() {
                @Override
                public void onStateChanged(int state) {
                    // At the time this callback is fired, the camera id
                    // has be set to the desired camera.

                    if (mPaused || mAppController.getCameraProvider().waitingForCamera()) {
                        ButtonManager buttonManager = mActivity.getButtonManager();
                        if (buttonManager != null) {
                            buttonManager.resetCameraButton();
                        }
                        return;
                    }

                    ButtonManager buttonManager = mActivity.getButtonManager();
                    buttonManager.disableCameraButtonAndBlock();

                    mPendingSwitchCameraId = state;

                    // We need to keep a preview frame for the animation before
                    // releasing the camera. This will trigger
                    // onPreviewTextureCopied.
                    // TODO: Need to animate the camera switch
                    switchCamera();
                }
            };

    protected void switchCamera() {
        // super.switchCamera();
        mActivity.switchFrontAndBackMode();

        mActivity.getCameraAppUI().updateModeList();

    }

    public void initCameraID() {
    }
    public void makeModuleUI(ReuseController controller, View parent){
    }
    @Override
    public void resume() {
        mDataModuleCurrent.addListener(this);
        DataModuleManager.getInstance(mActivity).addListener(this);
        // TODO Auto-generated method stub
    }

    @Override
    public void pause() {
        mDataModuleCurrent.removeListener(this);
        DataModuleManager.getInstance(mActivity).removeListener(this);
        // TODO Auto-generated method stub
    }

    @Override
    public void destroy() {
        // TODO Auto-generated method stub
    }

    @Override
    public void onDreamSettingChangeListener(
            HashMap<String, String> keys) {
    }

    @Override
    public void onSettingReset() {
        mActivity.getCameraAppUI().initSidePanel();
    }

    @Override
    public void onLayoutOrientationChanged(boolean isLandscape) {
        // TODO Auto-generated method stub
    }

    @Override
    public void hardResetSettings(SettingsManager settingsManager) {
        // TODO Auto-generated method stub
    }

    @Override
    public HardwareSpec getHardwareSpec() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public BottomBarUISpec getBottomBarSpec() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean isUsingBottomBar() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onShutterButtonFocus(boolean pressed) {
        // TODO Auto-generated method stub
    }

    @Override
    public void onShutterCoordinate(TouchCoordinate coord) {
        // TODO Auto-generated method stub
    }

    @Override
    public void onShutterButtonClick() {
        // TODO Auto-generated method stub
    }

    @Override
    public String getPeekAccessibilityString() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onCameraAvailable(CameraProxy cameraProxy) {

    }
}
