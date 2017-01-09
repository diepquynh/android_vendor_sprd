package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileSearchResultActivity.SearchTask;

import android.content.Context;
import android.view.ContextMenu;
import android.app.AddonManager;

public class FileSearchResultActivityUtils {
    static FileSearchResultActivityUtils sInstance;

    public FileSearchResultActivityUtils(){
        
    }
    public static FileSearchResultActivityUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileSearchResultActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_filesearchresultactivity, FileSearchResultActivityUtils.class);
        return sInstance;
    }

    public boolean DRMFileCopyMenu(File selectedFile){
        return true;
    }
    public boolean DRMFileProtectMenu(ContextMenu menu,File selectedFile,Context mContext){
        return false;
    }
    public boolean DRMFileShareMenu(File selectedFile){
        return true;
    }
    public String DRMFileShareClick(String filePath){
        return null;
    }
    
    public boolean DRMFileCopyMenuClick(Context mContext,int itemId,int iscopy){
        return false;
    }
    
    public void fileSearchDRMFile(String filePath,Context mContext,SearchTask mSearchTask){
    }
    public boolean isDRMFile(String filePath){
        return false;
    }
}
