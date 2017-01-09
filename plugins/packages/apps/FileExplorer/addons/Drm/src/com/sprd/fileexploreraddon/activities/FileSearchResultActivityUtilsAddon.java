package com.sprd.fileexploreraddon.activities;

import java.io.File;
import java.util.List;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmStore;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.widget.Toast;

import com.sprd.fileexplorer.activities.FileSearchResultActivity;
import com.sprd.fileexplorer.activities.FileSearchResultActivity.SearchTask;
import com.sprd.fileexplorer.drmplugin.FileSearchResultActivityUtils;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;


public class FileSearchResultActivityUtilsAddon extends FileSearchResultActivityUtils implements AddonManager.InitialCallback{

    private final String LOGTAG = "FileSearchResultActivityUtilsAddon";
    private Context mAddonContext;
    private boolean mIsIncludeDrmFile = false;
    public static final int MENU_PROTECT = 9;

    public FileSearchResultActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileCopyMenu(File selectedFile){
        String filePath = selectedFile.getPath();
        
        if (selectedFile.isDirectory()) {
            List<File> allFile = FileUtil.getAllFiles(selectedFile, true);
            for(File f : allFile){
                if (DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(f.getAbsolutePath())){
                    mIsIncludeDrmFile = true;
                    break;
                }
            }
            return true;
        }else{
            if (DRMFileUtil.isDrmEnabled() && !DRMFileUtil.isDrmFile(filePath)){
                return true;
            }
            if(!DRMFileUtil.isDrmEnabled()){
                return true;
            }
        }
        return false;
    }
    @Override
    public boolean DRMFileProtectMenu(ContextMenu menu,File selectedFile,Context mContext){
        String filePath = selectedFile.getPath();
        
        FileSearchResultMenuClickListener l = new FileSearchResultMenuClickListener(mContext,selectedFile);
        
         if (!selectedFile.isDirectory() && DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)) {
             menu.add(0, MENU_PROTECT, 0, mAddonContext.getString(R.string.drm_protect_viewdetails))
             .setOnMenuItemClickListener(l);
             return true;
        }
        return false;
    }
    @Override
    public boolean DRMFileShareMenu(File selectedFile){
        String filePath = selectedFile.getPath();
        
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            if(DrmStore.RightsStatus.RIGHTS_VALID == DRMFileUtil.mDrmManagerClient.checkRightsStatus(filePath,DrmStore.Action.TRANSFER)){
                return true;
            }
        }else if (!filePath.endsWith(".dm")){
            return true;
        }
        return false;
    }
    @Override
    public String DRMFileShareClick(String filePath){
        
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            return mimeType;
        }
        return null;
    }
    @Override
    public boolean DRMFileCopyMenuClick(Context mContext,int itemId,int iscopy){
        if (mIsIncludeDrmFile == true){
            mIsIncludeDrmFile = false;
            if(itemId == iscopy){
                Toast.makeText(mContext, mAddonContext.getString(R.string.drm_invalid_folder_copy), Toast.LENGTH_LONG).show();
                return true;
            }
        }
        return false;
    }
    @Override
    public void fileSearchDRMFile(String filePath,Context mContext,SearchTask mSearchTask){
        File file = new File(filePath);
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        String drmMimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        if (((FileSearchResultActivity)mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_IMAGE) && drmMimeType != null && drmMimeType.startsWith("image")){
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity)mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        if(((FileSearchResultActivity)mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_AUDIO) && drmMimeType != null && (drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg"))){
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity)mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        if(((FileSearchResultActivity)mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_VIDEO) && drmMimeType != null && drmMimeType.startsWith("video")){
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity)mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        /* SPRD: Add for bug505698. @{ */
        if(((FileSearchResultActivity)mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_DOC)
                && drmMimeType != null && drmMimeType.equals("text/plain")){
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity)mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        if(((FileSearchResultActivity)mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_APK)
                && drmMimeType != null && drmMimeType.equals("application/vnd.android.package-archive")){
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity)mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        /* @} */
        /* SPRD: Add for bug507423. @{ */
        if (((FileSearchResultActivity) mContext).mSearchType.contains(DRMFileType.FILE_TYPE_DRM_OTHERS)
                && !(drmMimeType != null && (drmMimeType.startsWith("image") || drmMimeType.startsWith("audio")
                        || drmMimeType.equals("application/ogg")
                        || drmMimeType.equals("application/vnd.android.package-archive")
                        || drmMimeType.startsWith("video")
                        || drmMimeType.equals("text/plain")
                        || drmMimeType.equals("application/vnd.android.package-archive")))) {
            if ((file.getName().toLowerCase().contains(((FileSearchResultActivity) mContext).mSearchKey))) {
                mSearchTask.filesList.add(new File(filePath));
            }
        }
        /* @} */
    }
    @Override
    public boolean isDRMFile(String filePath){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            return true;
        }
        return false;
    }
    final class FileSearchResultMenuClickListener implements
    MenuItem.OnMenuItemClickListener {
        private File mClickedFile;
        private Context mContext;

        public FileSearchResultMenuClickListener(Context context,File selectedFile) {
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
