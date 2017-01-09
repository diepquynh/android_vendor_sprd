package com.sprd.drmgalleryplugin.app;

import com.android.gallery3d.app.AbstractGalleryActivity;
import com.sprd.gallery3d.app.NewVideoActivity;
import com.android.gallery3d.data.LocalMediaItem;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.data.MediaSet;
import com.sprd.gallery3d.app.VideoItems;
import com.sprd.gallery3d.drm.SomePageUtils;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.util.DrmUtil;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.drm.DrmManagerClient;
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
    // SPRD: bug 624616 ,Slide to DRM image, should Consume authority
    public boolean checkPressedIsDrm(
            AbstractGalleryActivity activity, MediaItem item,
            AlertDialog.OnClickListener confirmListener,
            AlertDialog.OnClickListener cancelListener, boolean getContent) {
        if (item != null && !getContent && item.mIsDrmFile
                && !item.mDrmFileType.equals(DrmUtil.FL_DRM_FILE)) {
            if (DrmUtil.isDrmValid(item.getFilePath())) {
                new AlertDialog.Builder(activity.getAndroidContext()).
                        setTitle(mAddonContext.getString(R.string.drm_consume_title)).
                        setMessage(mAddonContext.getString(R.string.drm_consume_hint)).
                        setPositiveButton(android.R.string.ok, confirmListener).
                        setNegativeButton(android.R.string.cancel, cancelListener).
                        setCancelable(false).
                        show();
            } else {
                Intent intent = new Intent(DrmUtil.ACTION_DRM);
                LocalMediaItem mediaItem = (LocalMediaItem) item;
                intent.putExtra(DrmUtil.FILE_NAME, mediaItem.filePath);
                intent.putExtra(DrmUtil.KEY_DRM_MIMETYPE, DrmUtil.DCF_FILE_MIMETYPE);
                intent.putExtra(DrmUtil.IS_RENEW, true);
                Activity download = (Activity) activity;
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
    /* SPRD: Add for new feature DRM @{ */
    public boolean newCheckPressedIsDrm(Context context,
            String url, OnClickListener listener, boolean getContent) {
        // TODO Auto-generated method stub
        if (DrmUtil.isDrmFile(url, null)) {
            if (DrmUtil.isDrmValid(url)) {
        /* SPRD:Add for bug597820 There is consume notification when playing non-count-limit drm videos @{ */
                if (!DrmUtil.getDrmFileType(url).equals(DrmUtil.FL_DRM_FILE)) {
                    new AlertDialog.Builder(context).
                            setTitle(mAddonContext.getString(R.string.drm_consume_title)).
                            setMessage(mAddonContext.getString(R.string.drm_consume_hint)).
                            setPositiveButton(android.R.string.ok, listener).
                            setNegativeButton(android.R.string.cancel, null).
                            show();
                } else {
                    return false;
                }
                /* Bug597820 end @} */
            } else {
                Intent intent = new Intent(DrmUtil.ACTION_DRM);
                intent.putExtra(DrmUtil.FILE_NAME, url);
                intent.putExtra(DrmUtil.KEY_DRM_MIMETYPE, DrmUtil.DCF_FILE_MIMETYPE);
                intent.putExtra(DrmUtil.IS_RENEW, true);
                context.startActivity(intent);
            }
            return true;
        }
        return false;
    }

    public boolean checkIsDrmFile(String filePath) {
        // TODO Auto-generated method stub
        return DrmUtil.isDrmFile(filePath, null);
    }

    public boolean checkIsDrmFileValid(String filePath) {
        // TODO Auto-generated method stub
        return DrmUtil.isDrmValid(filePath);
    }

    public boolean isDrmSupportTransfer(String filePath) {
        // TODO Auto-generated method stub
        return DrmUtil.isDrmSupportTransfer(filePath);
    }

   public Object newTransferDate(Long time, NewVideoActivity activity) {
    // TODO Auto-generated method stub
        return DrmUtil.newTransferDate(time, activity);
    }

   public Object newCompareDrmExpirationTime(Object object,byte[] clickTime,NewVideoActivity activity) {
        // TODO Auto-generated method stub
       return DrmUtil.newCompareDrmExpirationTime(object, clickTime, activity);
   }

   public Object newCompareDrmRemainRight(String filePath,Object object,NewVideoActivity activity) {
        // TODO Auto-generated method stub
       return DrmUtil.newCompareDrmRemainRight(filePath, object, activity);
   }

   public DrmManagerClient getDrmManagerClient() {
    // TODO Auto-generated method stub
       return AddonGalleryAppImpl.getDrmManagerClient();
   }

    /* SPRD: Add for bug599941 non-sd drm videos are not supported to share @{ */
    @Override
    public boolean newIsSupportShare(String filePath) {
        return DrmUtil.newIsSupportShare(filePath);
    }
    /* Bug599941 end @} */
    /* DRM feature end @} */
}
