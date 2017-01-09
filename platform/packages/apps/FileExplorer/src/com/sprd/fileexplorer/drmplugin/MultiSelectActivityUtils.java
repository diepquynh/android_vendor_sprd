package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.MultiSelectActivity;

import android.content.Context;
import android.app.AddonManager;

public class MultiSelectActivityUtils {
    static MultiSelectActivityUtils sInstance;

    public MultiSelectActivityUtils(){
        
    }
    public static MultiSelectActivityUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (MultiSelectActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_multiselectactivity, MultiSelectActivityUtils.class);
        return sInstance;
    }

    public boolean DRMFileShareClick(String filePath,MultiSelectActivity activity){
        return false;
    }

    /* SPRD: Add for bug511177. @{ */
    public String getDrmFileDetailType(File selectedFile){
        return null;
    }
    /* @} */
}
