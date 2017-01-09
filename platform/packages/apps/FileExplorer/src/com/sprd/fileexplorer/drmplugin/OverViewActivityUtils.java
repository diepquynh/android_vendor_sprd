package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.OverViewActivity;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;

import android.content.Context;
import android.view.ContextMenu;
import android.view.View;
import android.app.AddonManager;

public class OverViewActivityUtils {
    static OverViewActivityUtils sInstance;

    public OverViewActivityUtils(){
        
    }
    public static OverViewActivityUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (OverViewActivityUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_overviewactivity, OverViewActivityUtils.class);
        return sInstance;
    }

    public void protectMenu(ContextMenu menu,File selectedFile,Context mContext){
    }
    public boolean clickDRMFile(QuickScanCursorAdapter mAdapter,File file,View view,OverViewActivity activity){
        return false;
    }
    public boolean DRMFileCopyMenu(File selectedFile){
        return true;
    }
    public boolean DRMFileShareMenu(File selectedFile){
        return true;
    }
    public boolean DRMFileSetAsMenu(File selectedFile){
        return true;
    }
    public String DRMFileShareClick(File selectedFile){
        return null;
    }
}
