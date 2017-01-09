
package com.dream.camera.vgesture;

/*
 * New Feature for VGesture
 */

import android.graphics.RectF;
import android.hardware.Camera.Face;
import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.camera.CameraActivity;
import com.android.camera.app.AppController;
import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.ui.FaceView;
import com.android.camera.ui.RotateImageView;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.sprd.hz.selfportrait.detect.DetectEngine;
import com.sprd.hz.selfportrait.util.ContextUtil;
import com.sprd.hz.selfportrait.util.SoundUtil;
import com.sprd.ucam.vgesutre.DetectView;
import com.sprd.ucam.vgesutre.TimerCountView;

public class VGestureImp implements VGestureInterface, OrientationManager.OnOrientationChangeListener{
    private static final Log.Tag TAG = new Log.Tag("VGestureImp");
    private static VGestureImp mInstance;

    private DetectView mDetectView;
    private ImageView mViewMode = null;
    private ImageView mGestureButton = null;
    private RotateImageView mGestureGuideView = null;
    private TextView mGestureHelpView = null;
    protected TimerCountView mTimerView = null;
    protected DetectEngine mDetectEngine = null;
    private boolean mVGestureStarted = false;
    private RelativeLayout mVgesture = null;
    private SoundUtil mSoundUtil = null;
    private AppController mAppController = null;
    private CameraActivity mActivity = null;
    private CameraAgent.CameraProxy mCameraDevice = null;
    private int mVgesturePreviewWidth = 0;
    private int mVgesturePreviewHeight = 0;
    private boolean mHasInitControl = false;

    public static synchronized VGestureImp getInstance(CameraActivity activity) {
        if (mInstance == null) {
            mInstance = new VGestureImp(activity);
        }
        return mInstance;
    }

    public VGestureImp(CameraActivity activity) {
        mAppController = activity;
        mActivity = activity;
    }

    @Override
    public void onStartVGestureDetection(int orientation, boolean mirror,
            CameraAgent.CameraProxy cameraDevice, Handler h, int PreviewWidth, int PreviewHeight,
            View rootView) {
        mCameraDevice = cameraDevice;
        mVgesturePreviewWidth = PreviewWidth;
        mVgesturePreviewHeight = PreviewHeight;
        initControls(h, PreviewWidth, PreviewWidth, rootView);
        mDetectEngine.startDetect(cameraDevice, PreviewWidth, PreviewHeight);
        rotateUI();
        updateDetectEngineDisplayOritation();
        mDetectEngine.setMirror(mirror);
    }

    @Override
    public void onStopVGestureDetection() {
        if (mDetectEngine != null) {
            mDetectEngine.destroy();
        }
        if (mVgesture != null) {
            mVgesture.setVisibility(View.INVISIBLE);
        }
        /* SPRD: Add for bug 569343 (487754 in 5.1) @{ */
        if (mDetectView != null) {
            mDetectView.clear();
        }
        /* @} */
    }

    public void initControls(Handler h, int PreviewWidth, int PreviewHeight, View rootView) {
        /*
         * LayoutInflater.from(mActivity).inflate(R.layout.ucam_vgesture_module,true);
         */
        mVgesture = (RelativeLayout) rootView
                .findViewById(R.id.vgesture_root_layout);
        ContextUtil.getInstance().appContext = mAppController
                .getAndroidContext();
        RelativeLayout.LayoutParams param = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT);
        RelativeLayout.LayoutParams timeparam = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
        timeparam.addRule(RelativeLayout.CENTER_IN_PARENT, RelativeLayout.TRUE);
        mTimerView = new TimerCountView(mActivity);
        mVgesture.addView(mTimerView, timeparam);
        mTimerView.setHandler(h);
        // mDetectView = new DetectView(mActivity);
        mDetectView = (DetectView) rootView.findViewById(R.id.camera_view_detect);
        // mVgesture.addView(mDetectView, param);
        mViewMode = (ImageView) mVgesture.findViewById(R.id.camera_mode_view);
        mGestureButton = (ImageView) mVgesture.findViewById(R.id.camera_gesture_button);
        mGestureGuideView = (RotateImageView) mVgesture.findViewById(R.id.camera_gesture_image);
        if(mActivity != null && mActivity.getOrientationManager() != null){
            mActivity.getOrientationManager().addOnOrientationChangeListener(this);
        }
        mGestureHelpView = (TextView) mVgesture.findViewById(R.id.camera_help_text);
        mDetectView.setHandler(h);
        mDetectEngine = new DetectEngine();
        mDetectEngine.setHandler(h);
        mDetectEngine.setDisplaySize(PreviewWidth, PreviewHeight);
        mDetectEngine.setDetectType(DetectEngine.DETECT_TYPE_GESTURE);
        mDetectView.setEngine(mDetectEngine);
        mViewMode.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                updateUIHelp();
            }
        });
        mGestureButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                updateUIGuide();
            }
        });
        mHasInitControl = true;
        showVGesturePanel();
    }

    public void rotateUI() {
        OrientationManager manager = mAppController.getOrientationManager();
        if(manager == null){
            return;
        }
        int orientation = manager.getDeviceOrientation()
                .getDegrees();
        if (orientation == 90) {
            orientation = 270;
        } else if (orientation == 270) {
            orientation = 90;
        }
        if (mTimerView != null) {
            mTimerView.setRotation(orientation);
        }

    }

    @Override
    public void setVGestureStart(boolean isStart) {
        if (isStart) {
            mVGestureStarted = true;
        } else {
            mVGestureStarted = false;
        }
    }

    @Override
    public void startTimer(int detectMode) {
        if(mDetectEngine != null){
            mDetectEngine.stopDetect();
        }
        // If the timer is already counting, start capture directly.
        if (!mTimerView.isTimerCounting()) {
            mTimerView.startTimer();
        } else {
            mTimerView.startTimerFromDetector(detectMode);
        }
    }

    @Override
    public void stopTimer(boolean restart){
        if(mDetectEngine != null && restart){
            mDetectEngine.startDetect(mCameraDevice, mVgesturePreviewWidth, mVgesturePreviewHeight);
        }
        if(mTimerView != null){
            mTimerView.stopTimer();
        }
    }

    @Override
    public void startDetect() {
        if (mDetectEngine != null) {
            mDetectEngine.startDetect(mCameraDevice, mVgesturePreviewWidth, mVgesturePreviewHeight);
        }
    }

    @Override
    public void stopDetect() {
        if (mDetectEngine != null) {
            mDetectEngine.stopDetect();
        }
    }

    public void updateDetectEngineDisplayOritation() {
        if (mAppController != null && mAppController.getOrientationManager() != null) {
            int orientation = mAppController.getOrientationManager().getDeviceOrientation()
                    .getDegrees();
            orientation += 90;
            mDetectEngine.setDisplayOritation(orientation % 360);
        }
    }

    public void onPreviewAreaChanged(RectF previewArea) {
        if (mDetectEngine != null) {
            mDetectEngine.onPreviewAreaChanged(previewArea);
        }
    }

    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        rotateUI();
        if (mDetectEngine != null) {
            updateDetectEngineDisplayOritation();
        }
    }

    public void setDisplayOrientation(int orientation) {
        if (mDetectEngine != null) {
            updateDetectEngineDisplayOritation();
        }
    }

    public void onPause() {
        if (mVGestureStarted && mDetectEngine != null) {
            mDetectEngine.destroy();
        }
        if (mVgesture != null) {
            mVgesture.setVisibility(View.INVISIBLE);
        }
        if (mTimerView != null) {
            mTimerView.stopTimer();
        }
        /* SPRD: Add for bug 569343 (487754 in 5.1) @{ */
        if (mDetectView != null) {
            mDetectView.clear();
        }
        /* @} */
    }

    public boolean isNeedClearFaceView() {
        return !mVGestureStarted;
    }

    public boolean isDetectView() {
        return mVGestureStarted;
    }

    public void doDectView(Face[] faces, FaceView faceView) {
        if (mDetectEngine != null && faces != null) {
            mDetectView.onDetectFace(faceView.getFaceRect(faces));
            mDetectEngine.setFaces(faceView.getFaceRect(faces), faceView.getRealArea());
        }

    }
    /* @} */
    /* nj dream camera test 24 */
    public void resetVGestureImp() {
        mInstance = null;
    }
    /* @} */

    public void hideVGesturePanel() {
        if (mHasInitControl && mVgesture != null) {
            mVgesture.setVisibility(View.INVISIBLE);
        }
    }

    public void showVGesturePanel() {
        if (mHasInitControl && mVgesture != null) {
            mVgesture.setVisibility(View.VISIBLE);
        }
        showGuide();
        showHelp();
    }

    private void updateUIHelp() {
        boolean value = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE_HELP);

        DataModuleManager.getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .changeSettings(Keys.KEY_CAMERA_VGESTURE_HELP, !value);
    }

    private void updateUIGuide() {

        boolean value = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE_GUIDE);

        DataModuleManager.getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .changeSettings(Keys.KEY_CAMERA_VGESTURE_GUIDE, !value);

    }

    @Override
    public void showGuide() {
        if(!mHasInitControl){
            return;
        }
        if(mGestureGuideView == null || mGestureButton == null){
            return;
        }
        boolean bShow = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE_GUIDE);
        if (bShow) {
            mGestureGuideView.setVisibility(View.VISIBLE);
            mGestureButton.setImageResource(R.drawable.guideline_on);
        } else {
            mGestureGuideView.setVisibility(View.INVISIBLE);
            mGestureButton.setImageResource(R.drawable.guideline_off);
        }
    }

    @Override
    public void showHelp() {
        if(!mHasInitControl){
            return;
        }
        if(mGestureHelpView == null || mViewMode == null){
            return;
        }
        boolean bShow = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE_HELP);
        if (bShow) {
            mGestureHelpView.setVisibility(View.VISIBLE);
            mViewMode.setImageResource(R.drawable.information_on);
        } else {
            mGestureHelpView.setVisibility(View.INVISIBLE);
            mViewMode.setImageResource(R.drawable.information_off);
        }
    }
}
