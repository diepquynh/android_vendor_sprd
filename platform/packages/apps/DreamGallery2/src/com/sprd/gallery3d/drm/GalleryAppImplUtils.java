
package com.sprd.gallery3d.drm;

import com.android.gallery3d.app.GalleryAppImpl;
import com.android.gallery3d.R;

import android.app.AddonManager;
import android.drm.DrmManagerClient;

public class GalleryAppImplUtils {

    static GalleryAppImplUtils sInstance;

    public static GalleryAppImplUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (GalleryAppImplUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_galleryappimpl, GalleryAppImplUtils.class);
        return sInstance;
    }

    public void createGalleryAppImpl(GalleryAppImpl impl) {
    }
}
