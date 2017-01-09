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
import android.os.StrictMode;

import com.sprd.fileexplorer.activities.OverViewActivity;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;
import com.sprd.fileexplorer.drmplugin.OverViewActivityUtils;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileType;
import com.sprd.fileexploreraddon.util.DRMFileUtil;
import com.sprd.fileexploreraddon.util.DRMIntentUtil;

public class OverViewActivityUtilsAddon extends OverViewActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;
    /* SPRD: Modify for bug524315. @{ */
    private static final String TAG = "OverViewActivityUtilsAddon";
    private static final String TYPE_IMAGE = "image";
    private static final String TYPE_AUDIO = "audio";
    private static final String TYPE_VIDEO = "video";
    private static final String TYPE_APPLICATION_OGG = "application/ogg";

    private static final String DRMTYPE_FL = "fl";
    private static final String SUFFIX_DCF = ".dcf";
    private static final String SUFFIX_DR = ".dr";
    private static final String SUFFIX_DRC = ".drc";

    private static final String FILE_NAME = "filename";
    private static final String MIME_TYPE = "mimetype";
    private static final String IS_RENEW = "isrenew";
    private static final String ACTION_VIEW_DOWNLOADS_DRM = "sprd.android.intent.action.VIEW_DOWNLOADS_DRM";

    private static final int RIGHTS_NOT = -1;
    private static final int RIGHTS_DR = 1;
    private static final int RIGHTS_DRC = 2;
    /* @} */
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
                Log.w("is_drm", "OverViewActivityUtilsAddon extended_data -- "+values);
                drmType = values.getAsString("extended_data");
            }
        }catch(Exception e){
            Log.e("is_drm","OverViewActivityUtilsAddon get extended_data error");
        }
        // SPRD: Modify for bug615723.
        final Uri uri = Uri.fromFile(file);
        /* SPRD: Modify for bug640525. @{ */
        boolean isDrmEnabled = DRMFileUtil.isDrmEnabled();
        boolean isDrmFile = DRMFileUtil.isDrmFile(filePath);

        Log.d(TAG, "clickDRMFile(): file = " + file + "; drmMimeType = " + drmMimeType);
        if (intent != null) {
            intent = intent.setDataAndType(uri, drmMimeType);
            final Intent drmIntent = intent;
            if (isDrmEnabled && isDrmFile) {
                if (DRMFileUtil.isDrmValid(filePath)) {
                    if (drmType != null && drmType.toString().equals(DRMTYPE_FL)) {
                        if (drmMimeType != null && drmMimeType.startsWith(TYPE_IMAGE)
                                || drmMimeType.startsWith(TYPE_VIDEO)
                                || drmMimeType.startsWith(TYPE_AUDIO)
                                || drmMimeType.equals(TYPE_APPLICATION_OGG)) {
                            /* SPRD 506702 @{ */
                            try {
                                // SPRD: Add for bug615723.
                                StrictMode.disableDeathOnFileUriExposure();
                                activity.startActivity(drmIntent);
                            } catch (ActivityNotFoundException anfe) {
                                DRMFileUtil.showToast(activity, R.string.drm_no_application_open, Toast.LENGTH_LONG);
                                anfe.printStackTrace();
                                // SPRD: Add for bug615723.
                            } finally {
                                StrictMode.enableDeathOnFileUriExposure();
                            }
                            /* @} */
                        } else {
                            DRMFileUtil.showToast(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent,
                                    Toast.LENGTH_LONG);
                        }
                        return true;
                    }
                    if (drmMimeType != null && drmMimeType.startsWith(TYPE_IMAGE) || drmMimeType.startsWith(TYPE_VIDEO)
                            || drmMimeType.startsWith(TYPE_AUDIO) || drmMimeType.equals(TYPE_APPLICATION_OGG)) {
                        new AlertDialog.Builder(view.getContext())
                                .setTitle(mAddonContext.getString(R.string.drm_consume_title))
                                .setMessage(mAddonContext.getString(R.string.drm_consume_hint))
                                .setPositiveButton(com.sprd.fileexplorer.R.string.common_text_ok,
                                        new AlertDialog.OnClickListener() {
                                            @Override
                                            public void onClick(DialogInterface dialog, int which) {
                                                /* SPRD 506702 @{ */
                                                try {
                                                    // SPRD: Add for bug615723.
                                                    StrictMode.disableDeathOnFileUriExposure();
                                                    activity.startActivity(drmIntent);
                                                } catch (ActivityNotFoundException anfe) {
                                                    DRMFileUtil.showToast(activity, R.string.drm_no_application_open,
                                                            Toast.LENGTH_LONG);
                                                    anfe.printStackTrace();
                                                    // SPRD: Add for bug615723.
                                                } finally {
                                                    StrictMode.enableDeathOnFileUriExposure();
                                                }
                                                /* @} */
                                            }
                                        }).setNegativeButton(com.sprd.fileexplorer.R.string.common_text_cancel, null)
                                .show();
                    } else {
                        DRMFileUtil.showToast(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent,
                                Toast.LENGTH_LONG);
                    }

                } else {
                    mAdapter.refresh();
                    if (drmMimeType == null) {
                        DRMFileUtil.showToast(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent,
                                Toast.LENGTH_LONG);
                    } else if (drmMimeType.startsWith(TYPE_IMAGE) || drmMimeType.startsWith(TYPE_VIDEO)
                            || drmMimeType.startsWith(TYPE_AUDIO) || drmMimeType.equals(TYPE_APPLICATION_OGG)) {
                        String mimeType = "application/vnd.oma.drm.content";
                        try {
                            Intent drm_Intent = new Intent(ACTION_VIEW_DOWNLOADS_DRM);
                            drm_Intent.putExtra(FILE_NAME, filePath);
                            drm_Intent.putExtra(MIME_TYPE, mimeType);
                            drm_Intent.putExtra(IS_RENEW, true);
                            activity.startActivity(drm_Intent);
                        } catch (Exception e) {
                            Log.e(TAG, "clickDRMFile(): get activity to handle intent error!");
                            DRMFileUtil.showToast(activity, R.string.drm_no_application_open, Toast.LENGTH_LONG);
                            e.printStackTrace();
                        }
                        intent = intent.setDataAndType(uri, drmMimeType);
                    } else {
                        DRMFileUtil.showToast(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent,
                                Toast.LENGTH_LONG);
                    }
                }

            } else {
                if (!isDrmEnabled && (filePath.endsWith(SUFFIX_DCF))) {
                    DRMFileUtil.showToast(activity, com.sprd.fileexplorer.R.string.msg_invalid_intent,
                            Toast.LENGTH_LONG);
                } else {
                    activity.startActivity(intent);
                }
            }

        } else {
            int rightsFile = rightsFileType(filePath);
            if (isDrmEnabled && (rightsFile != RIGHTS_NOT)) {
                String mimeType = "";
                if (rightsFile == RIGHTS_DR) {
                    mimeType = "application/vnd.oma.drm.rights+xml";
                } else {
                    mimeType = "application/vnd.oma.drm.rights+wbxml";
                }
                try {
                    Intent drm_Intent = new Intent(ACTION_VIEW_DOWNLOADS_DRM);
                    drm_Intent.putExtra(FILE_NAME, filePath);
                    drm_Intent.putExtra(MIME_TYPE, mimeType);
                    drm_Intent.putExtra(IS_RENEW, false);
                    activity.startActivity(drm_Intent);
                } catch (Exception e) {
                    DRMFileUtil.showToast(activity, R.string.drm_no_application_open, Toast.LENGTH_LONG);
                    e.printStackTrace();
                }
            }
        }

        if (isDrmFile || (rightsFileType(filePath) != RIGHTS_NOT)) {
            return true;
        } else {
            return false;
        }
        /* @} */
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

    /* SPRD: Add for bug640525. @{ */
    public int rightsFileType(String filePath) {
        String suffix = DRMFileType.getFileType(mAddonContext).getSuffixByName(filePath);
        int rights = -1;
        if (suffix.endsWith(SUFFIX_DR)) {
            rights = RIGHTS_DR;
        } else if (suffix.endsWith(SUFFIX_DRC)) {
            rights = RIGHTS_DRC;
        }
        return rights;
    }
    /* @} */
}
