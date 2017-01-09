package com.sprd.fileexploreraddon.fragments;

import java.io.File;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmStore;
import android.media.MediaFile;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;

import com.sprd.fileexplorer.drmplugin.DetailedListFragmentUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class DetailedListFragmentUtilsAddon extends DetailedListFragmentUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;
    private static final int MENU_PROTECT = 9;

    public DetailedListFragmentUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void DRMFileProtectMenu(ContextMenu menu,File selectedFile,Context mContext){
        String filePath = selectedFile.getPath();
        
        DetailFragmentMenuClickListener l = new DetailFragmentMenuClickListener(mContext,selectedFile);
       
        if (!selectedFile.isDirectory() && DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)) {
            menu.add(0, MENU_PROTECT, 0, mAddonContext.getString(R.string.drm_protect_viewdetails))
            .setOnMenuItemClickListener(l);
        }
    }
    @Override
    public boolean DRMFileCopyMenu(File selectedFile){
        String filePath = selectedFile.getPath();
        if (selectedFile.canRead()) {
            if (selectedFile.isDirectory()) {
                return true;
            }else {
                // SPRD: Add for bug 524335, judge whether it is dr or drc rights file.
                if (DRMFileUtil.isDrmEnabled() && !DRMFileUtil.isDrmFile(filePath)
                        && !DRMFileUtil.isRightsFileType(filePath)) {
                    return true;
                }
                if(!DRMFileUtil.isDrmEnabled()){
                    return true;
                }
            }
        }
        return false;
    }
    @Override
    public boolean DRMFileShareMenu(File selectedFile,Context mContext){
        String filePath = selectedFile.getPath();
        
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if (selectedFile.canRead()) {
            if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
                if(DrmStore.RightsStatus.RIGHTS_VALID == DRMFileUtil.mDrmManagerClient.checkRightsStatus(filePath,DrmStore.Action.TRANSFER)){
                    return true;
                }
            }else if (!filePath.endsWith(".dm")){
                return true;
            }
        }
        return false;
    }
    @Override
    public boolean DRMFileSetAsMenu(File selectedFile){
        String filePath = selectedFile.getPath();
        if(DRMFileUtil.isDrmEnabled() && !DRMFileUtil.isDrmFile(filePath)){
            return true;
        }else if (!selectedFile.toString().endsWith(".dcf")){
            return true;
        }
        return false;
    }
    @Override
    public String DRMFileShareClick(File selectedFile){
        String filePath = selectedFile.getPath();
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        if(DRMFileUtil.isDrmEnabled() && MediaFile.getFileType(filePath) != null && (MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType))){
            return mimeType;
        }
        return null;
    }
    
    final class DetailFragmentMenuClickListener implements
    MenuItem.OnMenuItemClickListener {
        private File mClickedFile;
        private Context mContext;

        public DetailFragmentMenuClickListener(Context context,File selectedFile) {
            mClickedFile = selectedFile;
            mContext = context;
        }
        @Override
        public boolean onMenuItemClick(MenuItem item) {
            if (mClickedFile == null) {
                return false;
            }

            int itemId = item.getItemId();
            switch (itemId) {
            case MENU_PROTECT:
                DRMFileUtil.viewDrmDetails(mClickedFile,mContext,mAddonContext);
            }
            
            return true;
        }
    }
}
