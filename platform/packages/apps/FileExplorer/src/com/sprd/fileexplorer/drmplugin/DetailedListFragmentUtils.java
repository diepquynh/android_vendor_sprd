package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;

import android.content.Context;
import android.view.ContextMenu;
import android.app.AddonManager;

public class DetailedListFragmentUtils {
    static DetailedListFragmentUtils sInstance;

    public DetailedListFragmentUtils(){
        
    }
    public static DetailedListFragmentUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (DetailedListFragmentUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_detailedlistfragment, DetailedListFragmentUtils.class);
        return sInstance;
    }

    public void DRMFileProtectMenu(ContextMenu menu,File selectedFile,Context mContext){
    }
    public boolean DRMFileCopyMenu(File selectedFile){
        return true;
    }
    public boolean DRMFileShareMenu(File selectedFile,Context mContext){
        return true;
    }
    public boolean DRMFileSetAsMenu(File selectedFile){
        return true;
    }
    public String DRMFileShareClick(File selectedFile){
        return null;
    }
}
