package com.sprd.fileexploreraddon.adapters;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;
import com.sprd.fileexplorer.drmplugin.QuickScanCursorAdapterUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class QuickScanCursorAdapterUtilsAddon extends QuickScanCursorAdapterUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public QuickScanCursorAdapterUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileVideoType(Context mContext,String filePath,ViewHolder vHolder){
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            if(DRMFileUtil.isDrmValid(filePath)){
                vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_video_unlock));
            }else{
                vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_video_lock));
            }
            return true;
        }
        return false;
    }
    @Override
    public boolean DRMFileAudioType(Context mContext,String filePath,ViewHolder vHolder){
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            if(DRMFileUtil.isDrmValid(filePath)){
                vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_audio_unlock));
            }else{
            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_audio_lock));
            }
            return true;
        }
        return false;
    }
    @Override
    public boolean DRMFileImageType(Context mContext,String filePath,ViewHolder vHolder){
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            if(DRMFileUtil.isDrmValid(filePath)){
                vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_image_unlock));
            }else{
                vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_image_lock));
            }
            return true;
        }
        return false;
    }

    /* SPRD: Modify for bug640525. @{ */
    @Override
    public boolean DRMFileDefaultType(Context mContext, String filePath, ViewHolder vHolder) {
        boolean isDrmEnabled = DRMFileUtil.isDrmEnabled();
        boolean isDrmFile = DRMFileUtil.isDrmFile(filePath);
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if (isDrmEnabled && isDrmFile) {
            vHolder.fileIcon.setImageDrawable(mContext.getResources().getDrawable(
                    com.sprd.fileexplorer.R.drawable.file_item_default_ic));
            return true;
        }
        return false;
    }
    /* @} */
}
