
package com.dream.camera;

import android.graphics.Bitmap;

import com.android.camera.CameraActivity;
import com.android.camera.settings.Keys;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;

public class DreamUI {

    public final static int UNDEFINED = -1;
    public final static int DREAM_GIF_UI = 0;
    public final static int DREAM_WIDEANGLEPANORAMA_UI = 1;
    public final static int DREAM_FILTER_UI = 2;

    private int mTopPanelValue = -1;

    public int getUITpye() {
        return UNDEFINED;
    }

    public void showPanels() {
    }

    public void hidePanels() {
    }

    public void onThumbnail(final Bitmap thumbmail) {
    }

    public void adjustUI(int orientation) {
    }

    public void onCloseModeListOrSettingLayout() {
    }

    public boolean isFreezeFrameShow() {
        return false;
    }

    public boolean isReviewImageShow() {
        return false;
    }

    public void updateTopPanelValue(CameraActivity mActivity) {
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            mTopPanelValue = DreamUtil.BACK_CAMERA;
        } else {
            mTopPanelValue = DreamUtil.FRONT_CAMERA;
        }
    }

    public int getTopPanelValue() {
        return mTopPanelValue;
    }
}
