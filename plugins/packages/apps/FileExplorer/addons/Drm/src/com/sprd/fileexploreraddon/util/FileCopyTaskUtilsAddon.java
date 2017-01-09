package com.sprd.fileexploreraddon.util;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Map.Entry;

import com.sprd.fileexplorer.drmplugin.FileCopyTaskUtils;
import com.sprd.fileexplorer.util.FileCopyTask;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexploreraddon.R;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.Toast;

public class FileCopyTaskUtilsAddon extends FileCopyTaskUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;
    private long mIncludeDRMFileCount = 0;

    public FileCopyTaskUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean copyDRMFileHint(Context context,FileCopyTask fileCopyTask){
        final Context pluginContext1 = mAddonContext;
        final Context mContext = context;
        final long DRMFileCount = mIncludeDRMFileCount;
  
        if (mIncludeDRMFileCount > 0) {
            fileCopyTask.mMainHandler.post(new Runnable() {
            @Override
                public void run() {
                    Toast.makeText(mContext, pluginContext1.getString(R.string.drm_invalid_folder_copy,DRMFileCount),
                        Toast.LENGTH_LONG).show();
                }

            });
            mIncludeDRMFileCount = 0;
            return true;
        }
        return false;
    }
    @Override
    public void includeDRMFileCopy(FileCopyTask fileCopyTask){
        List<File> removeDRMFileList = new ArrayList<File>();
        List<File> leftFileValue = null;
        File keyFile = null;
        
        for(Entry<File, List<File>> e : fileCopyTask.opFilesMap.entrySet()) {
            keyFile = e.getKey();
            List<File> allFile = e.getValue();
            if(keyFile.isDirectory()){
                leftFileValue = FileUtil.getAllFiles(keyFile, true);
            }
            for(File f : allFile) {
                // SPRD: Add for bug 524335, judge whether it is dr or drc rights file.
                if (DRMFileUtil.isDrmEnabled() && (DRMFileUtil.isDrmFile(f.getPath())
                        || DRMFileUtil.isRightsFileType(f.getPath()))) {
                    mIncludeDRMFileCount += 1;
                    fileCopyTask.srcFilesSize -= f.length();
                    removeDRMFileList.add(f);
                    if(keyFile.isDirectory()){
                        leftFileValue.remove(f);
                    }
                }
            }
        }
        if(removeDRMFileList != null){
            if(keyFile.isDirectory() && leftFileValue != null){
                fileCopyTask.opFilesMap.remove(keyFile);
                fileCopyTask.opFilesMap.put(keyFile,leftFileValue);
            }else{
                for(File file:removeDRMFileList){
                    fileCopyTask.opFilesMap.remove(file);
                }
            }
       }
    }
}
