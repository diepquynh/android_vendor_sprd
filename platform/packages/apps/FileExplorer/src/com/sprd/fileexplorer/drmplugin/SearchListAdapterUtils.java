package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;

import android.content.Context;
import android.net.Uri;
import android.app.AddonManager;

public class SearchListAdapterUtils {
    static SearchListAdapterUtils sInstance;

    public SearchListAdapterUtils(){
        
    }
    public static SearchListAdapterUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (SearchListAdapterUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_searchlistadapter, SearchListAdapterUtils.class);
        return sInstance;
    }

    public boolean DRMFileSendIntent(Context mContext,File file,Uri uri){
        return false;
    }
    public boolean DRMFileSetIcon(Context context,File file,ViewHolder vHodler){
        return false;
    }
}
