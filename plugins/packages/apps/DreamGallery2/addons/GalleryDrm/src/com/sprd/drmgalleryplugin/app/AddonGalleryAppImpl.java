package com.sprd.drmgalleryplugin.app;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmManagerClientEx;
import android.util.Log;

import com.android.gallery3d.app.GalleryAppImpl;
import com.sprd.gallery3d.drm.GalleryAppImplUtils;


public class AddonGalleryAppImpl extends GalleryAppImplUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static GalleryAppImpl sGalleryAppImpl = null;
    private static DrmManagerClientEx mDrmManagerClient;

    
    public AddonGalleryAppImpl() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void createGalleryAppImpl (GalleryAppImpl impl) {
        sGalleryAppImpl = impl;
    }

    synchronized public static DrmManagerClientEx getDrmManagerClient() {
        if (mDrmManagerClient == null) {
            mDrmManagerClient = new DrmManagerClientEx((Context)sGalleryAppImpl);
        }
        return mDrmManagerClient;
    }

}
