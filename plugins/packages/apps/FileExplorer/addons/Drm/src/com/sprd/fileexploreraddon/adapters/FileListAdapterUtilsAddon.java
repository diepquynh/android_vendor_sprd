package com.sprd.fileexploreraddon.adapters;

import java.io.File;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;
import com.sprd.fileexplorer.drmplugin.FileListAdapterUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;
import com.sprd.fileexploreraddon.util.DRMIntentUtil;

public class FileListAdapterUtilsAddon extends FileListAdapterUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;
    /* SPRD: Modify for bug524315. @{ */
    private final static int RIGHTS_NOT = -1;
    private final static int RIGHTS_DR = 1;
    private final static int RIGHTS_DRC = 2;
    /* @} */

    public FileListAdapterUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void DRMFileSetIcon(Context context, File file, ViewHolder vHolder) {

        Context mContext = context;
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        String filePath = file.getPath();

        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if (!DRMFileUtil.isDrmEnabled() && filePath.endsWith(".dcf")) {
            vHolder.fileIcon
                    .setImageDrawable(mContext
                            .getResources()
                            .getDrawable(
                                    com.sprd.fileexplorer.R.drawable.file_item_default_ic));
        }
        if (DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)) {
            String mimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
            if (mimeType == null) {
                vHolder.fileIcon
                        .setImageDrawable(mContext
                                .getResources()
                                .getDrawable(
                                        com.sprd.fileexplorer.R.drawable.file_item_default_ic));
            } else {
                if (DRMFileUtil.isDrmValid(filePath)) {
                    if (mimeType.startsWith("image")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_image_unlock));
                    } else if (mimeType.startsWith("audio")
                            || mimeType.equals("application/ogg")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_audio_unlock));
                    } else if (mimeType.startsWith("video")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_video_unlock));
                    } else {
                        vHolder.fileIcon
                                .setImageDrawable(mContext
                                        .getResources()
                                        .getDrawable(
                                                com.sprd.fileexplorer.R.drawable.file_item_default_ic));
                    }
                } else {
                    if (mimeType.startsWith("image")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_image_lock));
                    } else if (mimeType.startsWith("audio")
                            || mimeType.equals("application/ogg")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_audio_lock));
                    } else if (mimeType.startsWith("video")) {
                        vHolder.fileIcon.setImageDrawable(mAddonContext
                                .getResources().getDrawable(
                                        R.drawable.drm_video_lock));
                    } else {
                        vHolder.fileIcon
                                .setImageDrawable(mContext
                                        .getResources()
                                        .getDrawable(
                                                com.sprd.fileexplorer.R.drawable.file_item_default_ic));
                    }
                }
            }
        }
    }
    @Override
    public boolean DRMFileSendIntent(Context context,File file,Uri mUri,Handler mMainThreadHandler){
        final String filePath = file.getAbsolutePath();
        final Context pluginContext1 = mAddonContext;
        final Context mContext = context;
        final Uri uri = mUri;
        
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        final Intent intent = DRMIntentUtil.getIntentByFileType(mAddonContext,
              fileType, file);
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if (intent != null) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
                        String drmMimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
                        String drmType = null;
                        try {
                            ContentValues values = DRMFileUtil.mDrmManagerClient.getMetadata(filePath);
                            if (values != null){
                                Log.w("is_drm", "OverVIewActivity extended_data -- "+values);
                                drmType = values.getAsString("extended_data");
                            }
                        }catch(Exception e){
                            Log.e("is_drm","OverVIewActivity get extended_data error");
                        }
                        if (DRMFileUtil.isDrmValid(filePath)) {
                            if(drmType != null && drmType.toString().equals("fl")){
                                if(drmMimeType != null && drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                        || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")){
                                    try{
                                        Intent drmIntent = new Intent();
                                        drmIntent = drmIntent.setAction(Intent.ACTION_VIEW);
                                        drmIntent = drmIntent.setDataAndType(uri, drmMimeType);
                                        mContext.startActivity(drmIntent);
                                    }catch(Exception e){
                                        Toast.makeText(mContext, pluginContext1.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                                    }
                                }else{
                                    Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }
                                return;
                            }
                            if(drmMimeType != null && drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                    || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")){
                                final String typeDrm = drmMimeType;
                                
                                new AlertDialog.Builder(mContext).
                                    setTitle( pluginContext1.getString(R.string.drm_consume_title)).
                                    setMessage( pluginContext1.getString(R.string.drm_consume_hint)).
                                    setPositiveButton(com.sprd.fileexplorer.R.string.common_text_ok, new AlertDialog.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            // TODO Auto-generated method stub
                                            try{
                                                Intent drm_intent = new Intent();
                                                drm_intent = drm_intent.setAction(Intent.ACTION_VIEW);
                                                drm_intent = drm_intent.setDataAndType(uri, typeDrm);
                                                mContext.startActivity(drm_intent);
                                            }catch(Exception e){
                                                Toast.makeText(mContext, pluginContext1.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                                            }
                                        }
                                    }).setNegativeButton(com.sprd.fileexplorer.R.string.common_text_cancel, null).show();
                                }else{
                                    Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }
                            }else{
//                                    adapter.notifyDataSetChanged();
                                if(drmMimeType == null){
                                    Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }else if(drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                        || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")){
                                    String mimeType = "application/vnd.oma.drm.content";
                                    try {
                                        Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                                        drm_Intent.putExtra("filename", filePath);
                                        drm_Intent.putExtra("mimetype", mimeType);
                                        drm_Intent.putExtra("isrenew", true);
                                        mContext.startActivity(drm_Intent);
                                    } catch (Exception e) {
                                        Toast.makeText(mContext,
                                                pluginContext1.getString(R.string.drm_no_application_open),
                                                Toast.LENGTH_LONG).show();
                                        Log.e("is_drm", "DRMFileSendIntent(): 1. get activity to handle Intent error!");
                                        e.printStackTrace();
                                    }
                                }else{
                                    Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }
                            }
                    }else{
                        if (!DRMFileUtil.isDrmEnabled() && (filePath.endsWith(".dcf"))){
                            Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                        }else{
                            mContext.startActivity(intent);
                        }
                    }
                 }
            });

        } else {
            mMainThreadHandler.post(new Runnable() {

                @Override
                public void run() {
                    /* SPRD: Modify for bug524315, can install drm rights file in FileExplorer. @{ */
                    int rightsFile = rightsFileType(filePath);
                    if (DRMFileUtil.isDrmEnabled() && (rightsFile != RIGHTS_NOT)) {
                        // String mimeType = "application/vnd.oma.drm.rights+xml";
                        String mimeType;
                        if (rightsFile == RIGHTS_DR) {
                            mimeType = "application/vnd.oma.drm.rights+xml";
                        } else {
                            mimeType = "application/vnd.oma.drm.rights+wbxml";
                        }
                        /* @} */
                        try {
                            Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                            drm_Intent.putExtra("filename", filePath);
                            drm_Intent.putExtra("mimetype", mimeType);
                            drm_Intent.putExtra("isrenew", false);
                            mContext.startActivity(drm_Intent);
                        } catch (Exception e) {
                            Toast.makeText(mContext, pluginContext1.getString(R.string.drm_no_application_open),
                                    Toast.LENGTH_LONG).show();
                            Log.e("is_drm", "DRMFileSendIntent(): 2. get activity to handle Intent error!");
                            e.printStackTrace();
                        }
                    }
                    /*else{
                        Toast.makeText(mContext, mContext.getString(com.sprd.fileexplorer.R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                    }*/
                }
            });

        }
        // SPRD: Modify for bug524315, can install drm rights file in FileExplorer.
        if(DRMFileUtil.isDrmFile(filePath) || (rightsFileType(filePath) != RIGHTS_NOT)){
            return true;
        }else{
            return false;
        }
    }

    /* SPRD: Modify for bug524315, can install drm rights file in FileExplorer. @{ */
    public int rightsFileType(String filePath) {
        if (filePath.endsWith(".dr") || filePath.endsWith(".DR")) {
            return RIGHTS_DR;
        } else if (filePath.endsWith(".drc") || filePath.endsWith(".DRC")) {
            return RIGHTS_DRC;
        }
        return RIGHTS_NOT;
    }
    /* @} */
}
