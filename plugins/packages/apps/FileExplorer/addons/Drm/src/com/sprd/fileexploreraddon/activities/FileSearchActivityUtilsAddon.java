package com.sprd.fileexploreraddon.activities;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.fileexplorer.activities.FileSearchActivity;
import com.sprd.fileexplorer.drmplugin.FileSearchActivityUtils;
import com.sprd.fileexploreraddon.util.DRMFileType;

public class FileSearchActivityUtilsAddon extends FileSearchActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public FileSearchActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void addOrRemoveDRMFile(int type,int addOrRemove,FileSearchActivity activity){
        switch(type){
        case 0:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_IMAGE);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_IMAGE);
            }
            break;
        case 1:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_VIDEO);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_VIDEO);
            }
            break;
        case 2:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_AUDIO);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_AUDIO);
            }
            break;
        /* SPRD: Add for bug505698. @{ */
        case 3:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_DOC);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_DOC);
            }
            break;
        case 4:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_APK);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_APK);
            }
            break;
        /* @} */
        /* SPRD: Add for bug507423. @{ */
        case 5:
            if(addOrRemove == 0){
                activity.mSearchType.remove(DRMFileType.FILE_TYPE_DRM_OTHERS);
            }else{
                activity.mSearchType.add(DRMFileType.FILE_TYPE_DRM_OTHERS);
            }
            break;
        /* @} */
        }
    }
}
