package com.sprd.fileexplorer.drmplugin;

import java.io.File;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;

import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;
import android.app.AddonManager;

public class FileListAdapterUtils {
    static FileListAdapterUtils sInstance;

    public FileListAdapterUtils(){
        
    }
    public static FileListAdapterUtils getInstance() {
        if (sInstance != null) return sInstance;
        sInstance = (FileListAdapterUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_drm_filelistadapter, FileListAdapterUtils.class);
        return sInstance;
    }

    public void DRMFileSetIcon(Context context,File file,ViewHolder vHolder){
    }
    public boolean DRMFileSendIntent(Context mContext,File file,Uri uri,Handler mMainThreadHandler){
        return false;
    }
}
