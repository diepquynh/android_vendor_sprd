package com.sprd.gallery3d.drm;

import android.app.AddonManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.android.gallery3d.R;
import com.android.gallery3d.data.DecodeUtils;
import com.android.gallery3d.data.MediaDetails;
import com.android.gallery3d.data.UriImage;
import com.android.gallery3d.util.ThreadPool;

import java.io.FileDescriptor;

public class UriImageDrmUtils {

    static UriImageDrmUtils sInstance;

    public static UriImageDrmUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (UriImageDrmUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_UriImage, UriImageDrmUtils.class);
        return sInstance;
    }

    public void loadUriDrmInfo(UriImage item) {

    }

    public Bitmap decodeUriDrmImage(ThreadPool.JobContext jc, final int type, String path,
                                    BitmapFactory.Options options, int targetSize, FileDescriptor fd){
        return DecodeUtils.decodeThumbnail(jc, fd, options, targetSize, type);
    }

    public boolean isDrmFile(String filePath, String mimeType){
        return false;
    }

    public MediaDetails getUriDetailsByAction(UriImage item, MediaDetails details, int action) {
        return details;
    }



}
