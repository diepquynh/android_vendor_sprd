
package com.sprd.gallery3d.drm;

import com.android.gallery3d.R;
import com.android.gallery3d.ui.PhotoView;
import com.android.gallery3d.glrenderer.GLCanvas;
import com.android.gallery3d.ui.PhotoView.Model;
import android.graphics.Rect;

import android.app.AddonManager;
import android.content.Context;


public class PhotoViewUtils {

    static PhotoViewUtils sInstance;

    public static PhotoViewUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (PhotoViewUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_photoview, PhotoViewUtils.class);
        return sInstance;
    }

    public void initPictureDrmIcon() {

    }

    public boolean isDrmUnLocked(Model mModel, int offset){
        return false;
    }

    public boolean isDrmLocked(Model mModel,int offset){
        return false;
    }

    public void setDrmIcon(GLCanvas canvas, Rect r, PhotoView mPhotoView, Boolean mIsVideo, Boolean mIsDrmLocked, Boolean mIsDrmUnLocked) {

    }
}
