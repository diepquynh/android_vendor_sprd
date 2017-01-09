package com.sprd.drmgalleryplugin.ui;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.glrenderer.GLCanvas;
import com.android.gallery3d.glrenderer.ResourceTexture;
import com.android.gallery3d.ui.AlbumSlotRenderer;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.util.DrmUtil;
import com.sprd.gallery3d.drm.SlotRendererUtils;

public class AddonSlotRenderer extends SlotRendererUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public ResourceTexture createDrmStatusOverlay(boolean loacked) {
        if (loacked) {
            return new ResourceTexture(mAddonContext, R.drawable.ic_drm_lock);
        } else {
            return new ResourceTexture(mAddonContext, R.drawable.ic_drm_unlock);
        }
    }

    @Override
    public void drawDrmStatusOverlay(AlbumSlotRenderer renderer, MediaItem item, GLCanvas canvas, int width, int height) {
        if(item == null || !item.mIsDrmFile) return;
        int s = Math.min(width, height) / 4;
        if (DrmUtil.isDrmValid(item.getFilePath())) {
            renderer.mDRMUnlockedIcon.draw(canvas, (width - s), (height - s), s, s);
        } else {
            renderer.mDRMLockedIcon.draw(canvas, (width - s), (height - s), s, s);
        }
    }
}
