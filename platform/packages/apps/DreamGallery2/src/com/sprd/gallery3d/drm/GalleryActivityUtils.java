
package com.sprd.gallery3d.drm;

import com.android.gallery3d.R;
import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

public class GalleryActivityUtils {

    static GalleryActivityUtils sInstance;

    public static GalleryActivityUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (GalleryActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_galleryactivity, GalleryActivityUtils.class);
        return sInstance;
    }

    public void startGetContentSetAs(Intent intent, Bundle data) {

    }
}
