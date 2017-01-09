
package com.sprd.drmgalleryplugin.ui;

import android.R.integer;
import android.app.AddonManager;
import android.content.Context;
import android.graphics.Picture;
import android.graphics.Rect;
import com.android.gallery3d.ui.PhotoView.Model;
import android.net.Uri;
import android.util.Log;
import com.android.gallery3d.ui.PhotoView;
import com.android.gallery3d.glrenderer.GLCanvas;
import com.android.gallery3d.glrenderer.ResourceTexture;
import com.android.gallery3d.app.SinglePhotoDataAdapter;
import com.android.gallery3d.app.PhotoDataAdapter;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.data.LocalImage;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.data.AddonLocalMediaItem;
import com.sprd.drmgalleryplugin.util.DrmUtil;
import com.sprd.gallery3d.drm.PhotoViewUtils;

public class AddonPhotoView extends PhotoViewUtils implements
        AddonManager.InitialCallback {
    private Context mAddonContext;
    private boolean mIsDrmLocked;
    private boolean mIsDrmUnLocked;
    private ResourceTexture mDRMLockedIcon;
    private ResourceTexture mDRMunLockedIcon;
    private static final String TAG = "AddonGalleryDrm";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void initPictureDrmIcon() {
        mDRMLockedIcon = new ResourceTexture(mAddonContext, R.drawable.ic_drm_lock);
        mDRMunLockedIcon = new ResourceTexture(mAddonContext, R.drawable.ic_drm_unlock);
    }

    @Override
    public boolean isDrmUnLocked(Model mModel, int offset) {
        if (mModel instanceof PhotoDataAdapter) {
            MediaItem item = mModel.getMediaItem(offset);
            if (item != null) {
                String filePath = item.getFilePath();
                return (DrmUtil.isDrmFile(filePath, null) && DrmUtil.isDrmValid(filePath)
                && !DrmUtil.getDrmFileType(filePath).equals(DrmUtil.FL_DRM_FILE));
            }
        }
        return false;
    }

    @Override
    public boolean isDrmLocked(Model mModel, int offset) {
        if (mModel instanceof PhotoDataAdapter) {
            MediaItem item = mModel.getMediaItem(offset);
            if (item != null) {
                String filePath = item.getFilePath();
                return (DrmUtil.isDrmFile(filePath, null) && !DrmUtil.isDrmValid(filePath)
                && !DrmUtil.getDrmFileType(filePath).equals(DrmUtil.FL_DRM_FILE));
            }
        }
        return false;
    }

    @Override
    public void setDrmIcon(GLCanvas canvas, Rect r, PhotoView mPhotoView, Boolean mIsVideo, Boolean mIsDrmLocked, Boolean mIsDrmUnLocked) {
        int s = Math.min(mPhotoView.getWidth(), mPhotoView.getHeight()) / 10;
        if (mIsDrmLocked) {
            mDRMLockedIcon.draw(canvas, 3 * s, 3 * s);
        } else if (mIsVideo && mIsDrmUnLocked) {
            mDRMunLockedIcon.draw(canvas, 3 * s, 3 * s);
        }
    }
}
