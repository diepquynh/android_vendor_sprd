package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FilePickerActivity;

import android.content.Context;
import android.app.AddonManager;

public class FilePickerActivityUtils {
    static FilePickerActivityUtils sInstance;

    public FilePickerActivityUtils(){
        
    }
    public static FilePickerActivityUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FilePickerActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_filepickeractivity, FilePickerActivityUtils.class);
        return sInstance;
    }

    public String isDRMType(File selectedFile){
        return null;
    }
    public boolean isDRMFileSelect(String filePath,boolean isFromMms,FilePickerActivity activity){
        return false;
    }
    public boolean isDRMSetWallpaper(String filePath,FilePickerActivity activity){
        return false;
    }
    public boolean isDRMToGalley(String filePath,FilePickerActivity activity){
        return false;
    }
    public boolean isDRMFromLauncher(String filePath,FilePickerActivity activity){
        return false;
    }
    /* SPRD: Add for bug 506739 @{ */
    public boolean isSupportDrm(String filePath, FilePickerActivity activity){
        return false;
    }
    /* @} */
    /* SPRD: Add for bug 602185 @{ */
    public boolean isDrmFile(String filePath){
        return false;
    }
    /* @} */
}
