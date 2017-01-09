
package com.sprd.gallery3d.drm;

import java.io.InputStream;

import com.sprd.gallery3d.gif.GifDecoder;
import com.android.gallery3d.R;

import android.app.AddonManager;
import android.net.Uri;

public class GifDecoderUtils {

    static GifDecoderUtils sInstance;

    public static GifDecoderUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (GifDecoderUtils) AddonManager.getDefault().getAddon(
                R.string.feature_drm_gifDecoder, GifDecoderUtils.class);
        return sInstance;
    }

    public void initDrm(Uri uri) {
    }

    public boolean isReadDrmUri() {
        return false;
    }

    public InputStream readDrmUri() {
        return null;
    }
}
