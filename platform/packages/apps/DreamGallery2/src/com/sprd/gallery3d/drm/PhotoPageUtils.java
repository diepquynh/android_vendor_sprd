
package com.sprd.gallery3d.drm;

import com.android.gallery3d.R;
import com.android.gallery3d.data.MediaItem;

import android.app.AddonManager;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;


public class PhotoPageUtils {

    static PhotoPageUtils sInstance;

    public static PhotoPageUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (PhotoPageUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_photopage, PhotoPageUtils.class);
        return sInstance;
    }

    public void getFirstPickIsDrmPhoto(Bundle data) {

    }

    public boolean cosumeDrmRights(Message message) {
        return true;
    }

    public void updateDrmCurrentPhoto(MediaItem photo, Handler handler) {

    }

    public void onDrmDestroy() {

    }
}
