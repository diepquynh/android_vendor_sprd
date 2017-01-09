package com.sprd.drmgalleryplugin.app;

import com.android.gallery3d.app.AbstractGalleryActivity;
import com.android.gallery3d.data.LocalMediaItem;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.data.MediaSet;
import com.sprd.gallery3d.drm.SomePageUtils;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.util.DrmUtil;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;

public class AddonSomePage extends SomePageUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean checkPressedIsDrm(AbstractGalleryActivity activity,
            MediaItem item, AlertDialog.OnClickListener listener, boolean getContent) {
        if(item != null && !getContent && item.mIsDrmFile
                && !item.mDrmFileType.equals(DrmUtil.FL_DRM_FILE)){
            if (DrmUtil.isDrmValid(item.getFilePath())) {
                new AlertDialog.Builder(activity.getAndroidContext()).
                        setTitle(mAddonContext.getString(R.string.drm_consume_title)).
                        setMessage(mAddonContext.getString(R.string.drm_consume_hint)).
                        setPositiveButton(android.R.string.ok, listener).
                        setNegativeButton(android.R.string.cancel, null).
                        show();
            }else {
                Intent intent = new Intent(DrmUtil.ACTION_DRM);
                LocalMediaItem mediaItem = (LocalMediaItem)item;
                intent.putExtra(DrmUtil.FILE_NAME, mediaItem.filePath);
                intent.putExtra(DrmUtil.KEY_DRM_MIMETYPE, DrmUtil.DCF_FILE_MIMETYPE);
                intent.putExtra(DrmUtil.IS_RENEW, true);
                Activity download = (Activity)activity;
                download.startActivity(intent);
            }
            return true;
        }
        return false;
    }

    @Override
    public boolean checkIsDrmFile(MediaSet targetSet) {
        return targetSet.getCoverMediaItem().mIsDrmFile;
    }

    @Override
    public boolean canGetFromDrm(Context context, boolean getContentForSetAs, MediaItem item){
        if(item.mIsDrmFile && (getContentForSetAs || !item.mDrmFileType.equals(DrmUtil.SD_DRM_FILE))){
            Toast.makeText(mAddonContext,
                    R.string.choose_drm_alert, Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
}
