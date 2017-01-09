package com.sprd.fileexplorer.drmplugin;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileSearchActivity;

import android.content.Context;
import android.app.AddonManager;

public class FileSearchActivityUtils {
    static FileSearchActivityUtils sInstance;

    public FileSearchActivityUtils(){
        
    }
    public static FileSearchActivityUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileSearchActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_filesearchactivity, FileSearchActivityUtils.class);
        return sInstance;
    }

    public void addOrRemoveDRMFile(int type,int addOrRemove,FileSearchActivity activity){
    }
}
