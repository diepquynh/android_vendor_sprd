
package com.sprd.drmgalleryplugin.app;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmManagerClient;
import android.util.Log;
import java.io.File;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore;
import android.database.Cursor;
import android.content.ContentUris;
import com.sprd.gallery3d.drm.GalleryActivityUtils;
import com.sprd.drmgalleryplugin.util.DrmUtil;

public class AddonGalleryActivity extends GalleryActivityUtils implements
        AddonManager.InitialCallback {
    private Context mAddonContext;
    public static final String KEY_SET_WALLPAPER = "applyForWallpaper";
    private static final String TAG = "AddonGalleryDrm";

    public AddonGalleryActivity() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void startGetContentSetAs(Intent intent, Bundle data) {
        if (intent.hasExtra(KEY_SET_WALLPAPER)
                || intent.hasExtra(MediaStore.EXTRA_OUTPUT)) {
            data.putBoolean("key-set-as", true);
        }
    }
}
