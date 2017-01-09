package com.sprd.fileexploreraddon.activities;

import java.io.File;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.drm.DrmStore;
import android.media.MediaFile;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Toast;
import android.net.Uri;

import com.sprd.fileexplorer.activities.OverViewActivity;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;
import com.sprd.fileexplorer.drmplugin.OverViewActivityUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;
import com.sprd.fileexploreraddon.util.DRMIntentUtil;

public class OverViewActivityUtilsAddon extends OverViewActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;
    private static final int MENU_PROTECT = 17;

    public OverViewActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void protectMenu(ContextMenu menu,File selectedFile,Context mContext){
        String filePath = selectedFile.getPath();
        
        OverViewMenuClickListener l = new OverViewMenuClickListener(mContext,selectedFile);
        
        if (!selectedFile.isDirectory() && DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)) {
            menu.add(0, MENU_PROTECT, 0, mAddonContext.getString(R.string.drm_protect_viewdetails))
            .setOnMenuItemClickListener(l);
        }
    }
    @Override
    public boolean clickDRMFile(QuickScanCursorAdapter mAdapter,File file,View view,OverViewActivity overViewActivity){
        String filePath = file.getPath();
        int fileType = DRMFileType.getFileType(mAddonContext).getFileType(file);
        Intent intent = DRMIntentUtil.getIntentByFileType(mAddonContext,
                fileType, file);
        final OverViewActivity activity = overViewActivity;

        String drmType = null;
        if (DRMFileUtil.mDrmManagerClient == null) {
            DRMFileUtil.init(mAddonContext);
        }
        final String drmMimeType = DRMFileUtil.mDrmManagerClient.getOriginalMimeType(filePath);
        
        try {
            ContentValues values = DRMFileUtil.mDrmManagerClient.getMetadata(filePath);
            if (values != null){
                Log.w("is_drm", "OverVIewActivity extended_data -- "+values);
                drmType = values.getAsString("extended_data");
            }
        }catch(Exception e){
            Log.e("is_drm","OverVIewActivity get extended_data error");
        }
        final Uri uri = Uri.fromFile(file);
        if(intent != null){
            intent = intent.setDataAndType(uri, drmMimeType);
        }
        final Intent drmIntent = intent;
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            if (DRMFileUtil.isDrmValid(filePath)) {
                if(drmType.toString().equals("fl")){
                    if (drmIntent != null) {
                        /* SPRD 506702 @{ */
                        try {
                            activity.startActivity(drmIntent);
                        } catch (ActivityNotFoundException anfe) {
                            Toast.makeText(activity, mAddonContext.getString(R.string.drm_no_application_open),
                                    Toast.LENGTH_LONG).show();
                            anfe.printStackTrace();
                        }
                        /* @} */
                    } else {
                        Toast.makeText(view.getContext(), com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_SHORT).show();
                    }
                    return true;
                }
                    new AlertDialog.Builder(view.getContext()).
                            setTitle(mAddonContext.getString(R.string.drm_consume_title)).
                            setMessage(mAddonContext.getString(R.string.drm_consume_hint)).
                            setPositiveButton(com.sprd.fileexplorer.R.string.common_text_ok, new AlertDialog.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    // TODO Auto-generated method stub
                                    if (drmIntent != null) {
                                        /* SPRD 506702 @{ */
                                        try {
                                            activity.startActivity(drmIntent);
                                        } catch (ActivityNotFoundException anfe) {
                                            Toast.makeText(activity,
                                                    mAddonContext.getString(R.string.drm_no_application_open),
                                                    Toast.LENGTH_LONG).show();
                                            anfe.printStackTrace();
                                        }
                                        /* @} */
                                    }else {
                                        Toast.makeText(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent, Toast.LENGTH_SHORT)
                                        .show();
                                    }
                                }
                            }).
                            setNegativeButton(com.sprd.fileexplorer.R.string.common_text_cancel, null).show();
                } else {
                    mAdapter.refresh();
                    String mimeType = "application/vnd.oma.drm.content";
                    try {
                        Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                        drm_Intent.putExtra("filename", filePath);
                        drm_Intent.putExtra("mimetype", mimeType);
                        drm_Intent.putExtra("isrenew", true);
                        activity.startActivity(drm_Intent);
                    } catch (Exception e) {
                        Toast.makeText(activity, mAddonContext.getString(R.string.drm_no_application_open),
                                Toast.LENGTH_LONG).show();
                        Log.e("is_drm", "clickDRMFile(): get activity to handle Intent error!");
                        e.printStackTrace();
                    }
                }
            }
        if(DRMFileUtil.isDrmFile(filePath)){
            return true;
        }else{
            return false;
        }
    }
    @Override
    public boolean DRMFileCopyMenu(File selectedFile){
        String filePath = selectedFile.getPath();
        if (selectedFile.canRead()) {
            if(DRMFileUtil.isDrmEnabled() && !DRMFileUtil.isDrmFile(filePath)){
                return true;
            }
            if(!DRMFileUtil.isDrmEnabled()){
                return true;
            }
        }
        return false;
    }
    @Override
    public boolean DRMFileShareMenu(File selectedFile){
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
    
    final class OverViewMenuClickListener implements
    MenuItem.OnMenuItemClickListener {
        private File mClickedFile;
        private Context mContext;

        public OverViewMenuClickListener(Context context,File selectedFile) {
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
