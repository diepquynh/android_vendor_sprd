package com.dream.camera.modules.autophoto;

import java.util.HashMap;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.PhotoModule;

import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera2.R;

import android.widget.FrameLayout;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.util.DreamUtil;
import android.hardware.Camera.Face;
import com.android.camera.ui.FaceView;
import android.graphics.RectF;
import com.dream.camera.vgesture.VGestureImp;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.dream.camera.MakeupController;
import com.dream.camera.MakeupController.*;

public class AutoPhotoUI extends DreamPhotoUI implements ResetListener {
    private static final Log.Tag TAG = new Log.Tag("AutoPhotoUI");
    private ImageButton mSettingsButton;
    private View topPanel;

    public AutoPhotoUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
//        if (UCamUtill.isVgestureEnnable() && isInFrontCamera()) {
//            mVGestureImp = VGestureImp.getInstance(activity);
//        }
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.autophoto_top_panel,
                        topPanelParent);
            }
            mActivity.getButtonManager().load(topPanel);
            bindFlashButton();

        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.autophoto_front_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);

            bindMakeupButton();

//            if (isInFrontCamera() && UCamUtill.isVgestureEnnable()) {
//                bindVGestureButton();
//            } else {
//                FrameLayout vgestureLayout = (FrameLayout)topPanel.findViewById(R.id.vgesture_layout);
//                vgestureLayout.setVisibility(View.GONE);
//            }
        }

        mSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);
        bindSettingsButton(mSettingsButton);
        bindCountDownButton();
        bindHdrButton();
        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA != dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            // mFreezeFrame = extendPanelParent;
            View extendPanel = lf.inflate(
                    R.layout.autophoto_front_extend_panel, extendPanelParent);
            initMakeupControl(extendPanelParent);
        }
//        DataModuleManager.getInstance(mActivity).addListener(this);
    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(
                SlidePanelManager.FILTER,View.VISIBLE);
        SlidePanelManager.getInstance(mActivity).focusItem(
                SlidePanelManager.CAPTURE, false);
    }

    @Override
    public void onSettingReset() {
        DreamUtil dreamUtil = new DreamUtil();
        int cameraid = dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
        if (DreamUtil.BACK_CAMERA != cameraid) {
            mBasicModule.updateMakeLevel();
            ((PhotoModule)mController).updateVGesture();
        }
    }

    @Override
    public void onResume(){
        DataModuleManager.getInstance(mActivity).addListener(this);
        updateSettingsButton(mSettingsButton);//SPRD:fix bug 607898
    }

    @Override
    public void onPause() {
        super.onPause();
        DataModuleManager.getInstance(mActivity).removeListener(this);
        /* SPRD: New feature vgesture detect @{ */
//        if (mVGestureImp != null) {
//            mVGestureImp.onPause();
//        }
    }

    public boolean isDetectView() {
//        if (isShowVGesture()) {
//            return mVGestureImp.isDetectView();
//        }
        return false;
    }


    public boolean isInFrontCamera() {
        DreamUtil dreamUtil = new DreamUtil();
        return DreamUtil.FRONT_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
    }

//    @Override
//    public void hidePanels() {
//        super.hidePanels();
//
//        if (isShowVGesture() && mVGestureImp != null) {
//            mVGestureImp.hideVGesturePanel();
//        }
//    }
//
//    @Override
//    public void showPanels() {
//        super.showPanels();
//
//        if (isShowVGesture() && mVGestureImp != null) {
//            mVGestureImp.showVGesturePanel();
//        }
//    }
//
//  public void doDectView(Face[] faces, FaceView faceView) {
//  if (isShowVGesture()) {
//      mVGestureImp.doDectView(faces, faceView);
//  }
//}
//    
//  public boolean isShowVGesture() {
//  DreamUtil dreamUtil = new DreamUtil();
//  return isInFrontCamera()
//          && UCamUtill.isVgestureEnnable()
//          && DataModuleManager.getInstance(mActivity)
//                  .getCurrentDataModule()
//                  .isEnableSettingConfig(Keys.KEY_CAMERA_VGESTURE)
//          && DataModuleManager.getInstance(mActivity)
//                  .getCurrentDataModule()
//                  .getBoolean(Keys.KEY_CAMERA_VGESTURE);
//}
//    
//  @Override
//  public boolean isNeedClearFaceView() {
//      boolean result = super.isNeedClearFaceView();
//      if (isShowVGesture()) {
//
//          return mVGestureImp.isNeedClearFaceView() && result;
//      }
//      return result;
//  }
//    
//  /* SPRD: New Feature for VGesture @{ */
//  @Override
//  public void onPreviewAreaChanged(RectF previewArea) {
//      super.onPreviewAreaChanged(previewArea);
//      /* SPRD: New feature vgesture detect @{ */
//      if (isShowVGesture() && mVGestureImp != null) {
//          mVGestureImp.onPreviewAreaChanged(previewArea);
//      }
//      /* @} */
//  }
//
//  @Override
//  public void setDisplayOrientation(int orientation) {
//      super.setDisplayOrientation(orientation);
//      /* SPRD: New feature vgesture detect @{ */
//      if (isShowVGesture()) {
//          mVGestureImp.setDisplayOrientation(orientation);
//      }
//      /* @} */
//  }
//    
//  /* SPRD: New feature vgesture detect @{ */
//  private VGestureImp mVGestureImp;

  /* @} */
}
