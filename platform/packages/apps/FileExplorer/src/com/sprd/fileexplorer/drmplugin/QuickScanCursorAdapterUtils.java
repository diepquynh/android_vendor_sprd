package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;

import android.content.Context;
import android.app.AddonManager;

public class QuickScanCursorAdapterUtils {
    static QuickScanCursorAdapterUtils sInstance;

    public QuickScanCursorAdapterUtils(){

    }
    public static QuickScanCursorAdapterUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (QuickScanCursorAdapterUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_quickscancursoradapter, QuickScanCursorAdapterUtils.class);
        return sInstance;
    }

    public boolean DRMFileVideoType(Context mContext,String filePath,ViewHolder vHolder){
        return false;
    }
    public boolean DRMFileAudioType(Context mContext,String filePath,ViewHolder vHolder){
        return false;
    }
    public boolean DRMFileImageType(Context mContext,String filePath,ViewHolder vHolder){
        return false;
    }

    /* SPRD: Add for bug640525. @{ */
    public boolean DRMFileDefaultType(Context mContext,String filePath,ViewHolder vHolder){
        return false;
    }
    /* @} */
}
