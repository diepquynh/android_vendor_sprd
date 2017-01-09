package com.sprd.fileexplorer.drmplugin;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.util.FileCopyTask;

import android.content.Context;
import android.util.Log;
import android.app.AddonManager;

public class FileCopyTaskUtils {
    static FileCopyTaskUtils sInstance;

    public FileCopyTaskUtils(){
        
    }
    public static FileCopyTaskUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileCopyTaskUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_filecopytask, FileCopyTaskUtils.class);
        return sInstance;
    }

    public boolean copyDRMFileHint(Context mContext,FileCopyTask fileCopyTask){
        return false;
    }
    public void includeDRMFileCopy(FileCopyTask fileCopyTask){
    }
}
