package com.sprd.fileexplorer.drmplugin;

import com.sprd.fileexplorer.R;

import android.content.Context;
import android.app.AddonManager;

public class FileExplorerAppUtils {
    static FileExplorerAppUtils sInstance;

    public FileExplorerAppUtils(){
        
    }
    public static FileExplorerAppUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileExplorerAppUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_fileexplorerapp, FileExplorerAppUtils.class);
        return sInstance;
    }

    public void getContext(){
    }
}
