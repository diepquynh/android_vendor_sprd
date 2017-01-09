package com.sprd.fileexploreraddon.activities;

import java.io.File;

import android.app.AddonManager;
import android.content.Context;
import android.media.MediaFile;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.activities.MultiSelectActivity;
import com.sprd.fileexplorer.drmplugin.MultiSelectActivityUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class MultiSelectActivityUtilsAddon extends MultiSelectActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public MultiSelectActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileShareClick(String filePath,MultiSelectActivity activity){
        /* SPRD: Add for bug511177. @{ */
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if (DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)
                && (DRMFileUtil.getDrmFileType(filePath).toLowerCase().equals(DRMFileUtil.FL_DRM_FILE)
                || DRMFileUtil.getDrmFileType(filePath).toLowerCase().equals(DRMFileUtil.CD_DRM_FILE))) {
        /* @} */
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_share), Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }

    /* SPRD: Add for bug511177. @{ */
    @Override
    public String getDrmFileDetailType(File selectedFile){
        String filePath = selectedFile.getPath();
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        if (DRMFileUtil.isDrmEnabled() && MediaFile.getFileType(filePath) != null
                && (MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType))) {
            return mimeType;
        }
        return null;
    }
    /* @} */
}
