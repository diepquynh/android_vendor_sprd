
package com.sprd.gallery3d.drm;

import java.util.ArrayList;

import com.android.gallery3d.app.AlbumPage;
import com.android.gallery3d.app.GalleryAppImpl;
import com.android.gallery3d.app.PhotoPage;
import com.android.gallery3d.data.MediaDetails;
import com.android.gallery3d.R;
import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmManagerClient;
import android.view.Menu;

public class MenuExecutorUtils {

    static MenuExecutorUtils sInstance;

    public static MenuExecutorUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (MenuExecutorUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_menuexecutor, MenuExecutorUtils.class);
        return sInstance;
    }

    public void createDrmMenuItem(Menu menu) {
    }

    public void updateDrmMenuOperation(Menu menu, int supported) {
    }

    public boolean showHideDrmDetails(AlbumPage page, int itemId) {
        return false;
    }

    public boolean showHideDrmDetails(PhotoPage page, int itemId, int index) {
        return false;
    }

    public boolean setDrmDetails(Context context, MediaDetails details
            , ArrayList<String> items, boolean isDrmDetails) {
        return false;
    }

    public String getDetailsNameForDrm(Context context, int key) {
        return "Unknown key" + key;
    }

    public boolean keyMatchDrm(int key) {
        return false;
    }

}
