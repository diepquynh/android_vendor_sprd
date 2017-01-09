package com.sprd.fileexploreraddon.util;

import java.io.File;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

import com.sprd.fileexploreraddon.R;

import android.content.Context;
import android.content.res.Resources;



public class DRMFileType {

    /* SPRD: Add for bug508342. @{ */
    /*public static final int FILE_TYPE_DRM = R.drawable.drm_image_unlock;
    public static final int FILE_TYPE_DRM_IMAGE = R.drawable.drm_image_lock;
    public static final int FILE_TYPE_DRM_AUDIO = R.drawable.drm_audio_lock;
    public static final int FILE_TYPE_DRM_VIDEO = R.drawable.drm_video_lock;
    public static final int FILE_TYPE_DRM_DOC = R.drawable.drm_doc_lock;
    public static final int FILE_TYPE_DRM_APK = R.drawable.drm_apk_lock;
    */
    public static final int FILE_TYPE_DRM = com.sprd.fileexplorer.R.drawable.drm_image_unlock;
    public static final int FILE_TYPE_DRM_IMAGE = com.sprd.fileexplorer.R.drawable.drm_image_lock;
    public static final int FILE_TYPE_DRM_AUDIO = com.sprd.fileexplorer.R.drawable.drm_audio_lock;
    public static final int FILE_TYPE_DRM_VIDEO = com.sprd.fileexplorer.R.drawable.drm_video_lock;
    public static final int FILE_TYPE_DRM_DOC = com.sprd.fileexplorer.R.drawable.drm_doc_lock;
    public static final int FILE_TYPE_DRM_APK = com.sprd.fileexplorer.R.drawable.drm_apk_lock;
    /* @} */
    // SPRD: Add for bug507423.
    public static final int FILE_TYPE_DRM_OTHERS = com.sprd.fileexplorer.R.drawable.file_item_default_ic;

    private Set<String> mDrmFileType = new HashSet<String>();
    private static boolean inited = false;
    private Resources resource;
    private static DRMFileType fileType = null;
    
    
    private DRMFileType(Context pluginContext) {
        resource = pluginContext.getResources();
        
        for (String s : resource.getStringArray(R.array.DrmFileType)) {
            mDrmFileType.add(s);
        }
    }
    
    public static void init(Context pluginContext) {
        fileType = new DRMFileType(pluginContext);
        inited = true;
    }
    
    public static DRMFileType getFileType(Context pluginContext) {
        if (!inited) {
            init(pluginContext);
        }
        return fileType;

    }
    
    private int getTypeBySuffix (String suffix) {
        if(mDrmFileType.contains(suffix)){
            return FILE_TYPE_DRM;
       }else{
           return -1;
       }
    }
    
    public int getFileTypeByName(String fileName) {
        return getTypeBySuffix(getSuffixByName(fileName));
    }
    
    public String getTypeByFile(File file) {
        String filePath = file.getAbsolutePath();
        if (DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
            if(mimeType.startsWith("image")){
                return "image/*";
            }else if(mimeType.startsWith("audio") || mimeType.equals("application/ogg") ){
                return "audio/*";
            }else if(mimeType.startsWith("video")){
                return "video/*";
            }
        }
        return null;
    }
    
    public int getFileType(File file) {
        String suffix = getSuffix(file);
        if (mDrmFileType.contains(suffix)) {
            return FILE_TYPE_DRM;
        }
        return -1;
    }
    
    public String getSuffix(File file) {
        if (file == null || !file.exists() || file.isDirectory()) {
            return null;
        }
        String fileName = file.getName();
        if (fileName.equals("") || fileName.endsWith(".")) {
            return null;
        }
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            return fileName.substring(index).toLowerCase(Locale.US);
        } else {
            return null;
        }
    }
    
    private String getSuffixByName(String fileName) {
        if (fileName.equals("") || fileName.endsWith(".")) {
            return null;
        }
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            return fileName.substring(index).toLowerCase(Locale.US);
        } else {
            return null;
        }
    }
}
