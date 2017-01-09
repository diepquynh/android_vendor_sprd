package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;

import android.content.Context;
import android.app.AddonManager;

public class FileUtilUtils {
    static FileUtilUtils sInstance;

    public FileUtilUtils(){
        
    }
    public static FileUtilUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileUtilUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_fileutil, FileUtilUtils.class);
        return sInstance;
    }

    public boolean renameDRMFile(Context context,File newFile,File file){
        return false;
    }
}
