package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;

import android.content.Context;
import android.app.AddonManager;

public class MultiSelectFileAdapterUtils {
    static MultiSelectFileAdapterUtils sInstance;

    public MultiSelectFileAdapterUtils(){
        
    }
    public static MultiSelectFileAdapterUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (MultiSelectFileAdapterUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_multiselectfileadapter, MultiSelectFileAdapterUtils.class);
        return sInstance;
    }

    public boolean DRMFileSetIcon(Context context,File file,ViewHolder vHolder){
        return false;
    }
}
