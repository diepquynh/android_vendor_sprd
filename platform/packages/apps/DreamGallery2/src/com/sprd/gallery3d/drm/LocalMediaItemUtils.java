
package com.sprd.gallery3d.drm;

import android.app.AddonManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.android.gallery3d.R;
import com.android.gallery3d.data.DecodeUtils;
import com.android.gallery3d.data.LocalImage;
import com.android.gallery3d.data.LocalMediaItem;
import com.android.gallery3d.data.LocalVideo;
import com.android.gallery3d.data.MediaDetails;
import com.android.gallery3d.util.ThreadPool.JobContext;

public class LocalMediaItemUtils {

    static LocalMediaItemUtils sInstance;

    public static LocalMediaItemUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (LocalMediaItemUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_localMedia, LocalMediaItemUtils.class);
        return sInstance;
    }

    public void loadDrmInfor(LocalMediaItem item) {

    }

    public int getImageSupportedOperations(LocalImage item, int operation) {
        return operation;
    }

    public int getVideoSupportedOperations(LocalVideo item, int operation) {
        return operation;
    }

    public Bitmap decodeThumbnailWithDrm(JobContext jc, final int type, String path,
            BitmapFactory.Options options, int targetSize) {
        return DecodeUtils.decodeThumbnail(jc, path, options, targetSize, type);
    }

    public MediaDetails getDetailsByAction(LocalMediaItem item, MediaDetails details, int action) {
        return details;
    }

    public boolean isCanFound(boolean found, String filePath, int mediaType) {
        return found;
    }
}
