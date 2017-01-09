package com.sprd.fileexploreraddon.util;

import java.io.File;

import com.sprd.fileexplorer.drmplugin.FileUtilUtils;
import com.sprd.fileexplorer.util.FileUtil;

import android.app.AddonManager;
import android.content.Context;
import android.provider.MediaStore;

public class FileUtilUtilsAddon extends FileUtilUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public FileUtilUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean renameDRMFile(Context context,File newFile,File file){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(file.getAbsolutePath())){
            FileUtil.scanFile(context, newFile);
            context.getContentResolver().delete(
            MediaStore.Files.getContentUri("external"),
            MediaStore.Files.FileColumns.DATA + "=?",
            new String[] {file.getAbsolutePath()});
            return true;
        }

        return false;
    }
}
