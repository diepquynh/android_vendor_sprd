
package com.sprd.gallery3d.drm;

import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.glrenderer.GLCanvas;
import com.android.gallery3d.glrenderer.ResourceTexture;
import com.android.gallery3d.R;
import com.android.gallery3d.ui.AlbumSlotRenderer;

import android.app.AddonManager;
import android.content.Context;

public class SlotRendererUtils {

    static SlotRendererUtils sInstance;

    public static SlotRendererUtils getInstance() {
        if (sInstance != null)
            return sInstance;
        sInstance = (SlotRendererUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_slotrenderer, SlotRendererUtils.class);
        return sInstance;
    }

    public ResourceTexture createDrmStatusOverlay(boolean loacked) {
        return null;
    }

    public void drawDrmStatusOverlay(AlbumSlotRenderer renderer, MediaItem item, GLCanvas canvas, int width, int height) {

    }
}
