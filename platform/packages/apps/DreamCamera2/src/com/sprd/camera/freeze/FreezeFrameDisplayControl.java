package com.sprd.camera.freeze;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.PhotoUI;
import android.view.View;

import android.widget.FrameLayout;
import android.view.LayoutInflater;
import android.net.Uri;
import com.sprd.camera.freeze.FreezeFrameDisplayView;
import com.android.camera.app.OrientationManager;
import com.android.camera.settings.Keys;
import com.android.camera.ui.Rotatable;
import com.android.camera2.R;
import android.util.Log;
import android.app.AlertDialog;
import android.app.Dialog;

import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

public class FreezeFrameDisplayControl implements FreezeFrameDisplayView.ProxyFreezeFrameClick,
        Rotatable, OrientationManager.OnOrientationChangeListener {
    private static final String TAG = "CAM_CameraFreezeFrameDisplayControl";
    private boolean sBurstSwitch;
    private boolean sFreezSwitch;
    public  boolean mFreezeVisible;
    // SPRD: Save uri for Frezzeframe display
    private Uri mUri;
    // freeze-frame display view
    private FreezeFrameDisplayView sView;
    private CameraActivity mActivity;
    private SettingsManager mSettingsmanager;
    private FaceDetectionController mUI;
    private boolean mIsImageCaptureIntent;
    private Listener mListener;
    private int mDisplayOrientation;
    public interface Listener {
        public void proxyStartPreview();
        public void proxySetCameraState(int state);
        public void proxystartFaceDetection();
        public void proxyCaptureCancelled();
        public void proxyCaptureDone();
        public boolean proxyIsContinueTakePicture();
        public boolean proxyIsPauseStatus();
        public void proxySetImageData();
    }
    public void setListener(Listener listener) {
        mListener = listener;
    }

    public FreezeFrameDisplayControl(CameraActivity activity, FaceDetectionController ui,
            boolean isImageCaptureIntent) {
        mActivity = activity;
        mSettingsmanager = activity.getSettingsManager();
        mUI = ui;
        sFreezSwitch = getBooleanByPreferenceKey(Keys.KEY_FREEZE_FRAME_DISPLAY);
    }
    // default construct, initialize properties
 /*   private FreezeFrameDisplayControl() {
        sBurstSwitch = isContinueTakePicture();
        sFreezSwitch = getBooleanByPreferenceKey(CameraSettings.KEY_FREEZE_FRAME_DISPLAY);
    }*/
    private void initialize() {
        FrameLayout layoutRoot =
            (FrameLayout)mActivity.findViewById(R.id.activity_root_view);
        if (sView != null) {
            layoutRoot.removeView(sView);
        }
        LayoutInflater inflater = mActivity.getLayoutInflater();

        sView = (FreezeFrameDisplayView)
            inflater.inflate(R.layout.preview_camera_freeze_frame_display, null);
        layoutRoot.addView(sView);
        layoutRoot.bringChildToFront(sView);
        sView.setListener(this);
        sView.initial(mActivity);
        OrientationManager mOrientationManager = mActivity.getOrientationManager();
        if (mOrientationManager != null) {
            mOrientationManager.addOnOrientationChangeListener(this);
            if (mOrientationManager.getDeviceOrientation() != null)
                mActivity.getCameraAppUI().setOrientation(sView,
                        mOrientationManager.getDeviceOrientation().getDegrees(), true);
        }
    }

    // Runs in main thread
    public boolean proxyDoNotTake() {
    /*
    * validate condition following:
    * 1、default by sView visibility state, if current state is visible, so anyway(camera key or soft key) can't take picture
    * 2、rejection soft key double click event and camera key quick click event
    */
        return (proxyDisplay()/* || (sFreezSwitch && (mCameraState != IDLE))*/);
    }

    public boolean isFreezeFrame() {
        // SPRD: FixBug 269986,To obtain the values "FreezeDisplay".
        sFreezSwitch = Keys.isFreezeDisplayOn(mSettingsmanager);
        Log.d(TAG,"sFreezSwitch = " + sFreezSwitch);
        if (mListener != null)
        sBurstSwitch = mListener.proxyIsContinueTakePicture();
        return (sFreezSwitch && !sBurstSwitch);
    }

    // Runs in main thread
    public void proxyAnimation(boolean show,int displayOrientation) {
        mDisplayOrientation = displayOrientation;
        proxyAnimation(show);
    }
    public void proxyAnimation(boolean show) {
        initialize();
        if (show) {
            sView.proxyFadeIn(mIsImageCaptureIntent, mFreezeVisible);
            // we need reset FreezeFrameDisplayView orientation
            /* @{ SPRD: fix bug 255774 start */
            //sView.setOrientation(mOrientation, true);
            //sView.setOrientation(mOrientationCompensation, true);
            updateRotation(mDisplayOrientation);
            mFreezeVisible = true;

            mActivity.getCameraAppUI().transitionToFreezeReivewLayout();
            mUI.pauseFaceDetection();
        } else {
            mActivity.getCameraAppUI().transitionToCapture();
            mUI.resumeFaceDetection();
            sView.proxyFadeOut();
            // mCameraAppView.setVisibility(View.VISIBLE);
            mFreezeVisible = false;
            updateCameraAppView();
            // SPRD: bug 262968 show module switch view when FreezeFrame unDisplay
        }
    }

    public boolean proxyDisplay() {
        if (sView != null) {
            return sView.displayed();
        }
        return false;
    }

   public void proxyRunLoadProxy(Uri uri) {
        mUri = uri;
        mActivity.setSwipingEnabled(false);
        if (sView == null) {
            initialize();
        }
        sView.runLoadResource(uri);
    }

    /* @{ SPRD: bug 251198 start */
   public void proxyRunLoadProxy(byte [] jpagByte) {
        initialize();
        mActivity.setSwipingEnabled(false);
        sView.runLoadResource(jpagByte,/*mActivity.isAutoCapture()*/false);
    }
    /* bug 251198 end @} */

    public boolean getBooleanByPreferenceKey(String key) {
         return Keys.isFreezeDisplayOn(mSettingsmanager);
    }

    public void proxyRestartCamera() {
        if (!mIsImageCaptureIntent && mListener != null/*&& (mCameraStartUpThread == null)*/) {
            mListener.proxyStartPreview();
            mListener.proxySetCameraState(PhotoController.IDLE);
/*            String face = mPreferences.getString(CameraSettings.KEY_CAMERA_AI_DETECT,
                    CameraSettings.VAL_OFF);
            if(null != face && !face.equals(CameraSettings.VAL_OFF)){//SPRD: BUG 332304
                mListener.proxystartFaceDetection();
            }*/
        }
    }

    @Override
    public void proxyRestartViews() {
        /* @{ SPRD: bug 251198 start */
        if (mIsImageCaptureIntent) {
            mListener.proxyCaptureCancelled();
            return;
        }
        /* bug 251198 end @} */
        mActivity.setSwipingEnabled(true);
        proxyRestartCamera();
        proxyAnimation(false);
    }

    @Override
    public void proxyDoneClicked() {
        /* @{ SPRD: bug 251198 start */
        if(mIsImageCaptureIntent) {
            mListener.proxyCaptureDone();
            return;
        }
        // SPRD: NotifyNewMedia when select save the freeze display picture
        Log.d(TAG,"proxyDoneClicked, mUri = "+mUri);
        if(mUri != null)
            mActivity.notifyNewMedia(mUri);
        /* bug 251198 end @} */
        mActivity.getCameraAppUI().setSwipeEnabled(true);//SPRD:Fix bug 401765
        mActivity.getCameraAppUI().showModeOptions();//SPRD:Fix bug 401344
        proxyRestartViews();
    }

    @Override
    public void proxyFinishDeleted(Uri uri) {
        Log.i(TAG, "main thread delete picture uri = " + uri);
        // update thumbnail
        if (!mIsImageCaptureIntent) {
            mActivity.removeDataByUri(uri);
        }
        mActivity.getCameraAppUI().setSwipeEnabled(true);//SPRD:Fix bug 401765
        mActivity.getCameraAppUI().showModeOptions();//SPRD:Fix bug 401344
    }

    /* @{ SPRD: bug 251198 start */
    @Override
    public void proxyRetakeClicked() {
        // TODO Auto-generated method stub
        if(mIsImageCaptureIntent && mListener != null) {
            mListener.proxySetImageData();
            proxyRestartCamera();
            proxyAnimation(false);
            if (mListener.proxyIsPauseStatus()) return;
          //  mUI.hidePostCaptureAlert();
            mListener.proxyStartPreview();
            mListener.proxySetCameraState(PhotoController.IDLE);
      /*      String face = mPreferences.getString(CameraSettings.KEY_CAMERA_AI_DETECT,
                    CameraSettings.VAL_OFF);
            if(null != face && !face.equals(CameraSettings.VAL_OFF)){//SPRD: BUG 332304
                mListener.proxystartFaceDetection();
            }*/
            return;
        }
        else return;
    }
    /* bug 251198 end @} */

    public void setOrientation(int orientation, boolean animation) {
        if (sView != null && proxyDisplay()) {
        }
    }

    // @{ SPRD: Update the view for freezeframe display begin
    public void updateFreezeUi() {
        if(mFreezeVisible) {
//            initialize();
            proxyAnimation(true);
            if(mUri != null) {
                proxyRunLoadProxy(mUri);
            }
        }
    } // SPRD: Update the view for freezeframe display end @}

    // @{ SPRD: Change the rotation of views for freezeframe begin
    public void updateRotation(int rotation) {
        if(sView == null) return;
        switch (rotation) {
        case 180:
            sView.setRotationY(180);
            break;
        case 270:
            sView.setRotationX(180);
            break;
        default:
            ;
        }
        sView.updateFreezeChildUi(rotation);
    } // SPRD: Change the rotation of views for freezeframe end @}

    public void updateCameraAppView() {
        //mActivity.updateCameraAppView();//SPRD:Add for freeze_display
    }

    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        int orientation = deviceOrientation.getDegrees();

        if (sView != null && proxyDisplay() && mActivity != null
                && mActivity.getCameraAppUI() != null) {
            mActivity.getCameraAppUI().setOrientation(sView, orientation, true);
        }
    }

    /**
     * add for dream camera
     * 
     * @{
     */
    public boolean isFreezeFrame(boolean freezSwitch) {
        // SPRD: FixBug 269986,To obtain the values "FreezeDisplay".
        sFreezSwitch = freezSwitch;
        Log.d(TAG, "isFreezeFrame(boolean) sFreezSwitch = " + sFreezSwitch);
        if (mListener != null)
            sBurstSwitch = mListener.proxyIsContinueTakePicture();
        return (sFreezSwitch && !sBurstSwitch);
    }

    public FreezeFrameDisplayControl(CameraActivity activity, FaceDetectionController ui,
            boolean isImageCaptureIntent, boolean freezSwitch) {
        mActivity = activity;
        mSettingsmanager = activity.getSettingsManager();
        mUI = ui;
        sFreezSwitch = freezSwitch;
        Log.d(TAG, "DD sFreezSwitch" + sFreezSwitch);
    }
    /* @} */
}
