package com.sprd.fileexploreraddon;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.fileexplorer.drmplugin.FileExplorerAppUtils;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class FileExplorerAppUtilsAddon extends FileExplorerAppUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public FileExplorerAppUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void getContext() {
        DRMFileUtil.init(mAddonContext);
        DRMFileType.init(mAddonContext);
    }
}
