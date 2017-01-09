package com.sprd.fileexploreraddon.activities;

import java.io.File;

import android.app.AddonManager;
import android.content.Context;
import android.media.MediaFile;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.activities.FilePickerActivity;
import com.sprd.fileexplorer.drmplugin.FilePickerActivityUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class FilePickerActivityUtilsAddon extends FilePickerActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public FilePickerActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public String isDRMType(File selectedFile){
        String filePath = selectedFile.getPath();
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        if(DRMFileUtil.isDrmEnabled() && MediaFile.getFileType(filePath) != null && (MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType))){
            return mimeType;
        }
        return null;
    }
    @Override
    public boolean isDRMFileSelect(String filePath,boolean isFromMms,FilePickerActivity activity){
        
        if (DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath) && !isFromMms){
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_select),Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
    @Override
    public boolean isDRMSetWallpaper(String filePath,FilePickerActivity activity){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_select),Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
    @Override
    public boolean isDRMToGalley(String filePath,FilePickerActivity activity){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_select),Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
    @Override
    public boolean isDRMFromLauncher(String filePath,FilePickerActivity activity){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_select),Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
    /* SPRD: Add for bug 506739 @{ */
    public boolean isSupportDrm(String filePath, FilePickerActivity activity) {
        if (DRMFileUtil.getDrmFileType(filePath).toLowerCase().equals(DRMFileUtil.FL_DRM_FILE)
                || DRMFileUtil.getDrmFileType(filePath).toLowerCase().equals(DRMFileUtil.CD_DRM_FILE)) {
            Toast.makeText(activity, mAddonContext.getString(R.string.choose_drm_alert_mms), Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
    /* @} */
    /* SPRD: Add for bug 602185 @{ */
    @Override
    public boolean isDrmFile(String filePath){
        return DRMFileUtil.isDrmFile(filePath);
    }
    /* @} */
}
