package com.sprd.fileexploreraddon.util;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.sprd.fileexploreraddon.R;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.graphics.Color;
import android.util.Log;
import android.widget.ScrollView;
import android.widget.TextView;
import android.media.MediaFile;
import android.os.SystemProperties;


public class DRMFileUtil {

    private static final String TAG = "PluginFileUtil";
    // SPRD: Add for bug 506739
    public static final String DRM_FILE_TYPE = "extended_data";
    public static final String FL_DRM_FILE = "fl";
    public static final String CD_DRM_FILE = "cd";
    public static final String SD_DRM_FILE = "sd";

    public static final String RIGHTS_NO_lIMIT = "-1";
    public static DrmManagerClient mDrmManagerClient = null;
    protected static byte[] mLock = new byte[0];

    public static void init(Context context) {
        if (mDrmManagerClient == null) {  
            synchronized (mLock) {  
                if (mDrmManagerClient == null) {
                    mDrmManagerClient = new DrmManagerClient(context);
                }
            }  
        }
    }
    
    public static void viewDrmDetails(File file, Context context,Context pluginContext) {
        buildDrmDetailDialog(file, context,pluginContext);
    }
    
    private static void buildDrmDetailDialog(final File file, Context context,Context pluginContext) {
        Resources res = pluginContext.getResources();
        AlertDialog.Builder deteilDialog = new AlertDialog.Builder(context);
        TextView detailView = new TextView(context);
        detailView.setTextColor(Color.BLACK);
        detailView.setPadding(20, 0, 10, 0);
        detailView.setTextSize(18);
        detailView.setHorizontallyScrolling(false);
        ScrollView scrollView = new ScrollView(context);
        scrollView.addView(detailView);
        deteilDialog.setView(scrollView);
        String filePath = file.getPath();
        ContentValues value = null;
        boolean isTransfer = (DrmStore.RightsStatus.RIGHTS_VALID == mDrmManagerClient.checkRightsStatus(filePath,DrmStore.Action.TRANSFER));
        boolean isDrmValid = (DrmStore.RightsStatus.RIGHTS_VALID == mDrmManagerClient.checkRightsStatus(filePath));
        
        String mimeType = mDrmManagerClient.getOriginalMimeType(filePath);
        int fileType = DRMFileType.getFileType(pluginContext).getFileType(file);
        if (fileType == DRMFileType.FILE_TYPE_DRM) {
            if(mimeType.startsWith("image")){
                value = mDrmManagerClient.getConstraints(filePath, DrmStore.Action.DISPLAY);
            }else{
                value = mDrmManagerClient.getConstraints(filePath, DrmStore.Action.PLAY);
            }
        }
        if(value == null){
            return;
        }
        Long startTime = value.getAsLong(DrmStore.ConstraintsColumns.LICENSE_START_TIME);
        Long endTime = value.getAsLong(DrmStore.ConstraintsColumns.LICENSE_EXPIRY_TIME);
        byte[] clickTime = value.getAsByteArray(DrmStore.ConstraintsColumns.EXTENDED_METADATA);

        detailView
                .setText(res.getString(R.string.drm_file_name,file.getName())
                        + "\n"
                        + (isDrmValid ? res
                                .getString(R.string.drm_rights_validity)
                                : res.getString(R.string.drm_rights_invalidity))
                        + "\n"
                        + (isTransfer ? res
                                .getString(R.string.drm_rights_status_transfer)
                                : res.getString(R.string.drm_rights_status_untransfer))
                        + "\n"
                        + (res.getString(R.string.drm_start_time,transferDate(startTime,pluginContext)))
                        + "\n"
                        + (res.getString(R.string.drm_end_time,transferDate(endTime,pluginContext)))
                        + "\n"
                        + (res.getString(R.string.drm_expiration_time,
                                compareDrmRight(value.get(DrmStore.ConstraintsColumns.LICENSE_AVAILABLE_TIME),clickTime,pluginContext)))
                        + "\n"
                        + (res.getString(R.string.drm_remain_times,
                                compareDrmRight(value.get(DrmStore.ConstraintsColumns.REMAINING_REPEAT_COUNT),pluginContext))));
        deteilDialog
                .setTitle(res.getString(R.string.drm_info))
                .setPositiveButton(com.sprd.fileexplorer.R.string.dialog_back,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {

                            }
                        }).show();
    }
    
    public static boolean isDrmEnabled() {
        String prop = SystemProperties.get("drm.service.enabled");
        return prop != null && prop.equals("true");
    } 

    public static Object compareDrmRight(Object object,Context context){
        if(object == null){ 
            return context.getString(R.string.drm_unknow_rights);
        }
        return object.toString().equals(RIGHTS_NO_lIMIT) ?
            context.getString(R.string.drm_unlimited_rights) : object.toString();
    }
    
    public static Object compareDrmRight(Object object,byte[] clickTime,Context context){
        if(object == null){ 
            return context.getString(R.string.drm_unknow_rights);
        }else if(clickTime == null){
            return context.getString(R.string.drm_inactive_rights);
        }else if(object.toString().equals(RIGHTS_NO_lIMIT)){
            return context.getString(R.string.drm_unlimited_rights);
        }else {
            String cTime = new String(clickTime);
            Long time = Long.valueOf(object.toString()) + Long.valueOf(cTime);
            return transferDate(time,context);
        }
    }

    public static String transferDate(Long time,Context context){
        if(time == null){
            return context.getString(R.string.drm_unknow_rights);
        }
        if(time == -1){
            return context.getString(R.string.drm_unlimited_rights);
        }
        Date date = new Date(time * 1000);  
        SimpleDateFormat sdformat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss"); 
        return sdformat.format(date);  
    }
    
    public static boolean isDrmFile(String filePath){
        if(MediaFile.getFileType(filePath) != null 
                && (MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType))){
            return true;
        }
        return false;
    }
    public static boolean isDrmValid(String filePath) {
        if (mDrmManagerClient == null) {
            Log.d(TAG, "mDrmManagerClient is null");
        }
        return DrmStore.RightsStatus.RIGHTS_VALID == mDrmManagerClient.checkRightsStatus(filePath);
    }
    /* SPRD: Add for bug 506739 @{ */
    public static String getDrmFileType(String filePath) {
        String drmFileType = "";
        try {
            ContentValues values = mDrmManagerClient.getMetadata(filePath);
            if (values != null) {
                drmFileType = values.getAsString(DRM_FILE_TYPE);
            }
        } catch (Exception e) {
            Log.e(TAG, "Get DRM_FILE_TYPE error");
        }
        return drmFileType;
    }
    /* @} */

    /* SPRD: Add for bug 524335, judge whether it is dr or drc rights file. @{ */
    public static boolean isRightsFileType(String filePath) {
        if (filePath == null || filePath.isEmpty()) {
            return false;
        }
        String filepath = filePath.toLowerCase();
        if (filepath.endsWith(".dr") || filepath.endsWith(".drc")) {
            return true;
        }
        return false;
    }
    /* @} */
}
