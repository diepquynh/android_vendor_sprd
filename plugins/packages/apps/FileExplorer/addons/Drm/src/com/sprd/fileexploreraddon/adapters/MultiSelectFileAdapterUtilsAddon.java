package com.sprd.fileexploreraddon.adapters;

import java.io.File;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;
import com.sprd.fileexplorer.drmplugin.MultiSelectFileAdapterUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class MultiSelectFileAdapterUtilsAddon extends MultiSelectFileAdapterUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public MultiSelectFileAdapterUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileSetIcon(Context context,File file,ViewHolder vHolder){
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        String filePath = file.getPath();
        Context mContext = context;
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(fileType == DRMFileType.FILE_TYPE_DRM){
            if(!DRMFileUtil.isDrmEnabled() && filePath.endsWith(".dcf")){
                vHolder.fileIcon.setImageDrawable(mContext.getResources().getDrawable(com.sprd.fileexplorer.R.drawable.file_item_default_ic));
            }
            if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
                String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
                if (mimeType == null){
                    vHolder.fileIcon.setImageDrawable(mContext.getResources().getDrawable(com.sprd.fileexplorer.R.drawable.file_item_default_ic));
                }else{
                    if(DRMFileUtil.isDrmValid(filePath)){
                        if(mimeType.startsWith("image")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_image_unlock));
                        }else if(mimeType.startsWith("audio") || mimeType.equals("application/ogg")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_audio_unlock));
                        }else if(mimeType.startsWith("video")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_video_unlock));
                        }else{
                            vHolder.fileIcon.setImageDrawable(mContext.getResources().getDrawable(com.sprd.fileexplorer.R.drawable.file_item_default_ic));
                        }
                    }else{
                        if(mimeType.startsWith("image")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_image_lock));
                        }else if(mimeType.startsWith("audio") || mimeType.equals("application/ogg")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_audio_lock));
                        }else if(mimeType.startsWith("video")){
                            vHolder.fileIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_video_lock));
                        }else{
                            vHolder.fileIcon.setImageDrawable(mContext.getResources().getDrawable(com.sprd.fileexplorer.R.drawable.file_item_default_ic));
                        }
                    }
                }
            }
            /* SPRD :  Add for resolve the bugs of  under multi-selection state does not show thumbnails  @{ */
            return true;
        }
        //return true;
        return false;
        /* @} */
    }
}
