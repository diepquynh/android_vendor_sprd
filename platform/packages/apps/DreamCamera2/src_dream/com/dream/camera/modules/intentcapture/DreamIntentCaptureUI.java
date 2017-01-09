
package com.dream.camera.modules.intentcapture;

import java.util.HashMap;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import android.widget.FrameLayout;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.vgesture.VGestureImp;
import com.android.camera2.R;
import android.graphics.RectF;
import android.hardware.Camera.Face;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.ui.FaceView;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import com.dream.camera.settings.DataModuleManager;
import com.ucamera.ucam.modules.utils.UCamUtill;

public class DreamIntentCaptureUI extends DreamPhotoUI {
    private static final Log.Tag TAG = new Log.Tag("DreamIntentCaptureUI");
    private View topPanel;

    /* SPRD: New feature vgesture detect @{ */
    private VGestureImp mVGestureImp;

    /* @} */

    public DreamIntentCaptureUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
        if (UCamUtill.isVgestureEnnable() && isInFrontCamera()) {
            mVGestureImp = VGestureImp.getInstance(activity);
        }
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.intentcapture_back_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);
            bindFlashButton();

        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.intentcapture_front_top_panel,
                        topPanelParent);
            }
            mActivity.getButtonManager().load(topPanel);
        }
        bindCountDownButton();
        bindCameraButton();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

    }

    @Override
    public void updateSlidePanel() {
        mActivity.getCameraAppUI().hideSlide();
    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().hideBottomPanelLeftRight();
    }

    public boolean isDetectView() {
        return false;
    }


    public boolean isInFrontCamera() {
        DreamUtil dreamUtil = new DreamUtil();
        return DreamUtil.FRONT_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
    }

}
