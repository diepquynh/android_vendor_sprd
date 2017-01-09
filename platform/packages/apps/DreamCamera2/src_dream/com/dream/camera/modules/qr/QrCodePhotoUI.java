package com.dream.camera.modules.qr;

import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;
import com.android.camera2.R;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.ImageButton;

public class QrCodePhotoUI extends ReusePhotoUI{
    private static final Log.Tag TAG = new Log.Tag("QrCodePhotoUI");
    private View topPanel;
    private ImageButton mQrCodeGallery;

    public QrCodePhotoUI(CameraActivity activity, ReuseController controller,
            View parent) {
        super(activity, controller, parent);
        ViewGroup moduleRoot = (ViewGroup) mRootView.findViewById(R.id.module_layout);
        mActivity.getLayoutInflater().inflate(R.layout.qrcode_capture,
                 moduleRoot, true);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.qrcode_back_top_panel,
                        topPanelParent);
            }
            mActivity.getButtonManager().load(topPanel);

            mQrCodeGallery = (ImageButton) topPanel
                    .findViewById(R.id.qrcode_gallery_toggle_button);
            bindFlashButton();
            bindCameraButton();
            bindQrCodeButton(mQrCodeGallery);
        }
    }

    @Override
    public void updateSidePanel() {
        sidePanelMask = DreamUtil.SP_EMPTY;
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    @Override
    public void setDisplayOrientation(int orientation) {
        super.setDisplayOrientation(orientation);
    }

    @Override
    public void onResume(){
        //DataModuleManager.getInstance(mActivity).addListener(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        //DataModuleManager.getInstance(mActivity).removeListener(this);
}

    public boolean isInFrontCamera() {
        DreamUtil dreamUtil = new DreamUtil();
        return DreamUtil.FRONT_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
    }

}
