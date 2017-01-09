package com.sprd.fileexploreraddon.adapters;

import java.io.File;

import com.sprd.fileexplorer.adapters.FileAdapter.ViewHolder;
import com.sprd.fileexplorer.drmplugin.SearchListAdapterUtils;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.NotifyManager;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;
import com.sprd.fileexploreraddon.util.DRMIntentUtil;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.StrictMode;
import android.util.Log;
import android.widget.Toast;

public class SearchListAdapterUtilsAddon extends SearchListAdapterUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public SearchListAdapterUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileSendIntent(Context context,File file,Uri mUri){
        String filePath = file.getAbsolutePath();
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        final Intent intent = DRMIntentUtil.getIntentByFileType(mAddonContext,
                fileType, file);
        final Context mContext = context;
        //final Uri uri = mUri;
        
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        
        if(intent != null){
            if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
                String drmType = null;
                String drmMimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
                Log.d("is_drm", "drmMimeType = "+drmMimeType);
                try {
                    ContentValues values = DRMFileUtil.mDrmManagerClient.getMetadata(filePath);
                    if (values != null){
                        Log.w("is_drm", "SearchListAdapterUtilsAddon extended_data -- "+values);
                        drmType = values.getAsString("extended_data");
                    }
                }catch(Exception e){
                    Log.e("is_drm","SearchListAdapterUtilsAddon get extended_data error");
                }
                // SPRD: Modify for bug615723.
                final Uri uri = Uri.fromFile(file);
                Log.d("is_drm", "uri = "+uri);
                if (DRMFileUtil.isDrmValid(filePath)) {
                    if(drmType.toString().equals("fl")){
                        if(drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")){
                            try{
                                Intent drmIntent = new Intent();
                                drmIntent = drmIntent.setAction(Intent.ACTION_VIEW);
                                drmIntent = drmIntent.setDataAndType(uri, drmMimeType);
                                Log.d("is_drm", "drmIntent = "+drmIntent);
                                // SPRD: Add for bug615723.
                                StrictMode.disableDeathOnFileUriExposure();
                                mContext.startActivity(drmIntent);
                            }catch(Exception e){
                                Toast.makeText(mContext, mAddonContext.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                            // SPRD: Add for bug615723.
                            } finally {
                                StrictMode.enableDeathOnFileUriExposure();
                            }
                        }else{
                            Toast.makeText(mContext, com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_LONG).show();
                        }
                        return true;
                    }
                    if(drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                            || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")){
                        final String typeDrm = drmMimeType;
                        final Context mAddonContext1 = mAddonContext;
                        new AlertDialog.Builder(mContext).
                            setTitle(mAddonContext.getString(R.string.drm_consume_title)).
                            setMessage(mAddonContext.getString(R.string.drm_consume_hint)).
                            setPositiveButton(com.sprd.fileexplorer.R.string.common_text_ok, new AlertDialog.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    // TODO Auto-generated method stub
                                    try{
                                        Intent drm_intent = new Intent();
                                        drm_intent = drm_intent.setAction(Intent.ACTION_VIEW);
                                        drm_intent = drm_intent.setDataAndType(uri, typeDrm);
                                        // SPRD: Add for bug615723.
                                        StrictMode.disableDeathOnFileUriExposure();
                                        mContext.startActivity(drm_intent);
                                    }catch(Exception e){
                                        Toast.makeText(mContext, mAddonContext1.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                                    // SPRD: Add for bug615723.
                                    } finally {
                                        StrictMode.enableDeathOnFileUriExposure();
                                    }
                                }
                            }).
                            setNegativeButton(com.sprd.fileexplorer.R.string.common_text_cancel, null).show();
                    }else{
                        Toast.makeText(mContext, com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_LONG).show();
                    }
                }else{
                    /*Adapter adapter = (Adapter)mAddonContext;
                    ((BaseAdapter)adapter).notifyDataSetChanged();*/
                    if(drmMimeType == null){
                        Toast.makeText(mContext, com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_LONG).show();
                    }else if(drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                            || drmMimeType.startsWith("audio") || drmMimeType.startsWith("application/ogg")){
                        String mimeType = "application/vnd.oma.drm.content";
                        try {
                            Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                            drm_Intent.putExtra("filename", filePath);
                            drm_Intent.putExtra("mimetype", mimeType);
                            drm_Intent.putExtra("isrenew", true);
                            mContext.startActivity(drm_Intent);
                        } catch (Exception e) {
                            Toast.makeText(mContext, mAddonContext.getString(R.string.drm_no_application_open),
                                    Toast.LENGTH_LONG).show();
                            Log.e("is_drm", "DRMFileSendIntent(): 1. get activity to handle Intent error!");
                            e.printStackTrace();
                        }
                    }else{
                        Toast.makeText(mContext, com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_LONG).show();
                    }
                }
            }else{
                mContext.startActivity(intent);
            }

        }else{
            if(DRMFileUtil.isDrmEnabled() && (filePath.endsWith(".dr") || filePath.endsWith(".DR"))){
                String mimeType = "application/vnd.oma.drm.rights+xml";
                try {
                    Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                    drm_Intent.putExtra("filename", filePath);
                    drm_Intent.putExtra("mimetype", mimeType);
                    drm_Intent.putExtra("isrenew", false);
                    mContext.startActivity(drm_Intent);
                } catch (Exception e) {
                    Toast.makeText(mContext, mAddonContext.getString(R.string.drm_no_application_open),
                            Toast.LENGTH_LONG).show();
                    Log.e("is_drm", "DRMFileSendIntent(): 2. get activity to handle Intent error!");
                    e.printStackTrace();
                }
            }
           /* else if(FileUtil.isDrmEnabled() && filePath.endsWith(".dm")){
                String mimeType = "application/vnd.oma.drm.message";
                Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                drm_Intent.putExtra("filename", filePath);
                drm_Intent.putExtra("mimetype", mimeType);
                drm_Intent.putExtra("isrenew", false);
                mContext.startActivity(drm_Intent);
            }*/
         /*   else {
                NotifyManager.getInstance().showToast(mContext.getResources().getString(com.sprd.fileexplorer.R.string.msg_invalid_intent));
            }*/
        }
        if(DRMFileUtil.isDrmFile(filePath) || (filePath.endsWith(".dr") || filePath.endsWith(".DR")) ){
            return true;
        }else{
            return false;
        }
    }
    @Override
    public boolean DRMFileSetIcon(Context mContext,File file,ViewHolder vHolder){
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        String filePath = file.getPath();
        
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        if(fileType == DRMFileType.FILE_TYPE_DRM){
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
            /* SPRD : BugFix bug399953 there is no thumbnail in search list @{ */
            return true;
        }
        return false;
    }
}
