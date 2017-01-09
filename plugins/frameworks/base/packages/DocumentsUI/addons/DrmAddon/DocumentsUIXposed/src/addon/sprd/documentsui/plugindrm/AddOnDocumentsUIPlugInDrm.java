/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package addon.sprd.documentsui.plugindrm;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;
import android.provider.MediaStore;
import android.net.Uri;
import android.content.Intent;
import android.content.DialogInterface;
import android.app.Activity;
import android.os.SystemProperties;
import android.os.Handler;
import android.drm.DrmManagerClient;
import android.database.Cursor;
import com.android.documentsui.model.DocumentInfo;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.provider.DocumentsContract.Document;
import android.drm.DrmStore.RightsStatus;
import com.android.documentsui.PlugInDrm.DocumentsUIPlugInDrm;
import com.android.documentsui.MimePredicate;
import android.media.MediaFile;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import java.io.File;

public class AddOnDocumentsUIPlugInDrm extends DocumentsUIPlugInDrm implements AddonManager.InitialCallback {

    private Context mContext;
    private Context sContext;
    private boolean isDrmEnable = false;
    public final static int ERROR = 0;
    public final static int NORMAL_FILE = 0;
    public final static int DRM_SD_FILE = 1;
    public final static int DRM_OTHER_FILE = 2;
    public final static int ABNORMAL_FILE = -1;
    public static final String DRM_DCF_FILE = ".dcf";
    private static String TAG = "AddOnDocumentsUIPlugInDrm";
    public String mDrmPath;
    public boolean mIsDrm;
    public final static int RESULT_DRM_OK = 1;
    public final static int RESULT_DRM_NOT_SELECTED = 2;
    public final static int RESULT_DRM_MIME_ERROR = 3;
    public final static int RESULT_DRM_NOT_SHARED = 4;

    public final static int RIGHTS_NOT = -1;
    public final static int RIGHTS_DR = 1;
    public final static int RIGHTS_DRC = 2;

    public AddOnDocumentsUIPlugInDrm(){
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        sContext = context;
        return clazz;
    }

    @Override
    public void getDocumentsActivityContext (Context context){
        mContext = context;
        return;
    }

   @Override
    public int checkDrmError(Activity activity , Uri[] uris, String[] acceptMimes){
        Log.i(TAG, "checkDrmError");
        for (int i = 0; i < uris.length; i++) {
            Uri uri = uris[i];
            String path = getDrmPath(mContext,uri);
            Log.i(TAG, "checkDrmError, path =  "+ path);
            if (getIsDrm(mContext,path)){
                String mime = getDocMimeType(mContext, path, "application/vnd.oma.drm.content");
                Log.i(TAG, "checkDrmError, mime =  "+ mime);
                boolean wallpaperExtra = activity.getIntent().hasExtra("applyForWallpaper");
                boolean outExtra = activity.getIntent().hasExtra(MediaStore.EXTRA_OUTPUT);
                if (wallpaperExtra || outExtra){
                    return RESULT_DRM_NOT_SELECTED;
                }
                if (!MimePredicate.mimeMatches(acceptMimes, mime)) {
                    return RESULT_DRM_MIME_ERROR;
                }
                if (!DocumentsUiDrmHelper.isDrmSDFile(mContext, path, null)){
                    return RESULT_DRM_NOT_SHARED;
                }
            }
        }
        return RESULT_DRM_OK;
    }

    @Override
    public boolean alertDrmError(int check_ret){
        boolean ret = true;
        Log.i(TAG, "alertDrmError, check_ret =  "+ check_ret);
        switch (check_ret) {
            case RESULT_DRM_NOT_SELECTED:
                Toast.makeText(sContext, R.string.drm_not_be_selected, Toast.LENGTH_SHORT).show();
                break;

            case RESULT_DRM_MIME_ERROR:
                Toast.makeText(sContext, R.string.file_type_wrong, Toast.LENGTH_SHORT).show();
                break;

            case RESULT_DRM_NOT_SHARED:
                Toast.makeText(sContext, R.string.drm_not_be_shared, Toast.LENGTH_SHORT).show();
                break;

            default:
                ret = false;
                break;
        }
        return ret;
    }

    @Override
    public boolean sendDRMFileIntent(Context context, Uri mUri, Handler mMainThreadHandler){
        final String filePath = getDrmPath(context,mUri);
        final Context mContext = context;
        final Uri uri = mUri;
        Log.d(TAG,"sendDRMFileIntent filePath = "+filePath+", uri = "+uri);

        boolean isDrm = DocumentsUiDrmHelper.isDrmFile(filePath);

        if (isDrm) {
            mMainThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (isDrmEnable) {
                        String drmMimeType =  DocumentsUiDrmHelper.getOriginalMimeType(mContext, filePath, null,isDrmEnable);
                        Log.d(TAG,"sendDRMFileIntent drmMimeType = "+drmMimeType);
                        if (DocumentsUiDrmHelper.isDrmValid(filePath)) {
                            if (DocumentsUiDrmHelper.isDrmFLFile(mContext, filePath, null)) {
                                if (drmMimeType != null && drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                        || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")) {
                                    try {
                                        Intent drmIntent = new Intent();
                                        drmIntent = drmIntent.setAction(Intent.ACTION_VIEW);
                                        drmIntent = drmIntent.setDataAndType(Uri.fromFile(new File(filePath)), drmMimeType);
                                        mContext.startActivity(drmIntent);
                                    } catch(Exception e) {
                                        Toast.makeText(mContext, sContext.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                                    }
                                } else {
                                    Toast.makeText(mContext, sContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }
                                return;
                            }
                            if (drmMimeType != null && drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
                                    || drmMimeType.startsWith("audio") || drmMimeType.equals("application/ogg")) {
                                final String typeDrm = drmMimeType;

                                new AlertDialog.Builder(mContext).
                                    setTitle(sContext.getString(R.string.drm_consume_title)).
                                    setMessage(sContext.getString(R.string.drm_consume_hint)).
                                    setPositiveButton(sContext.getString(R.string.ok_drm_rights_consume), new AlertDialog.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            // TODO Auto-generated method stub
                                            try {
                                                Intent drm_intent = new Intent();
                                                drm_intent.setAction(Intent.ACTION_VIEW);
                                                drm_intent.setDataAndType(Uri.fromFile(new File(filePath)), typeDrm);
                                                mContext.startActivity(drm_intent);
                                            } catch(Exception e) {
                                                Toast.makeText(mContext, sContext.getString(R.string.drm_no_application_open), Toast.LENGTH_LONG).show();
                                            }
                                        }
                                    }).setNegativeButton(sContext.getString(R.string.cancel_drm_rights_consume), null).show();
                            } else {
                                    Toast.makeText(mContext, sContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                            }
                        } else {
                                if(drmMimeType == null){
                                    Toast.makeText(mContext, sContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                } else if(drmMimeType.startsWith("image") || drmMimeType.startsWith("video")
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
                                                sContext.getString(R.string.drm_no_application_open),
                                                Toast.LENGTH_LONG).show();
                                        Log.e(TAG, "sendDRMFileIntent: 1. get activity to handle Intent error!");
                                        e.printStackTrace();
                                    }
                                } else {
                                    Toast.makeText(mContext, sContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                                }
                        }
                    } else {
                        Toast.makeText(mContext, sContext.getString(R.string.msg_invalid_intent), Toast.LENGTH_LONG).show();
                    }
                 }
            });

        } else {
            mMainThreadHandler.post(new Runnable() {

                @Override
                public void run() {
                    int rightsFile = rightsFileType(filePath);
                    if(isDrmEnable && (rightsFile != RIGHTS_NOT)){
                        String mimeType;
                        if (rightsFile == RIGHTS_DR) {
                            mimeType = "application/vnd.oma.drm.rights+xml";
                        } else {
                            mimeType = "application/vnd.oma.drm.rights+wbxml";
                        }
                        try {
                            Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
                            drm_Intent.putExtra("filename", filePath);
                            drm_Intent.putExtra("mimetype", mimeType);
                            drm_Intent.putExtra("isrenew", false);
                            mContext.startActivity(drm_Intent);
                        } catch (Exception e) {
                            Toast.makeText(mContext, sContext.getString(R.string.drm_no_application_open),
                                    Toast.LENGTH_LONG).show();
                            Log.e(TAG, "sendDRMFileIntent: 2. get activity to handle Intent error!");
                            e.printStackTrace();
                        }
                    }
                }
            });

        }
        if (isDrm || (rightsFileType(filePath) != RIGHTS_NOT)) {
            return true;
        } else {
            return false;
        }
    }

    public int rightsFileType(String filePath) {
        if (filePath.endsWith(".dr") || filePath.endsWith(".DR")) {
            return RIGHTS_DR;
        } else if(filePath.endsWith(".drc") || filePath.endsWith(".DRC")) {
            return RIGHTS_DRC;
        }
        return RIGHTS_NOT;
    }

    @Override
    public boolean isDrmEnabled() {
        return isDrmEnable;
    }

    @Override
    public void getDrmEnabled() {
        String prop = SystemProperties.get("drm.service.enabled");
        isDrmEnable =  prop != null && prop.equals("true");
    }

    public String getDocMimeType(Context context, String path, String mimeType){
        String type = mimeType;
        boolean drm = DocumentsUiDrmHelper.isDrmMimeType(context, path, mimeType,isDrmEnable);
        if (drm){
            String originType = DocumentsUiDrmHelper.getOriginalMimeType(context, path, mimeType,isDrmEnable);
            type = originType;
        }
        return type;
    }

    @Override
    public boolean setDocMimeType(DocumentInfo doc){
        String path = DocumentUriUtil.getPath(mContext, doc.derivedUri);
        Log.i(TAG, "setDocMimeType, path =  "+ path);
        String mimeType = "";
        if (path != null) {
            mimeType = MediaFile.getMimeTypeForFile(path);
        } else {
            mimeType = doc.mimeType;
        }
        Log.i(TAG, "setDocMimeType, mimeType =  "+ mimeType + ", doc.mimeType = " + doc.mimeType);
        int fileType = getDrmCanSharedType(mContext, doc.derivedUri, mimeType);
        Log.i(TAG, "setDocMimeType, fileType =  "+ fileType);
        if (fileType == DRM_SD_FILE){
            doc.mimeType = DocumentsUiDrmHelper.getOriginalMimeType(mContext, path, doc.mimeType,isDrmEnable);
        }else if (fileType == DRM_OTHER_FILE){
            Toast.makeText(sContext, R.string.drm_not_be_shared, Toast.LENGTH_SHORT).show();
            return false;
        }else if (fileType == ABNORMAL_FILE){
            Toast.makeText(sContext, R.string.error_in_shared, Toast.LENGTH_SHORT).show();
            return false;
        }
        return true;
    }

    private int getDrmCanSharedType(Context context, Uri uri, String mimetype){
        if (!DocumentsUiDrmHelper.isDrmMimeType(context, null, mimetype,isDrmEnable)){
            return NORMAL_FILE;
        }
        String path = DocumentUriUtil.getPath(context, uri);
        Log.i(TAG, "isDrmCanShard -- uri -- "+uri);
        Log.i(TAG, "isDrmCanShard -- path -- "+path);
        if (path != null) {
            if (DocumentsUiDrmHelper.isDrmCanTransfer(context, path, mimetype)){
                return DRM_SD_FILE;
            }else{
                return DRM_OTHER_FILE;
            }
        }
        return ABNORMAL_FILE;
    }

    public int getDrmIconMimeDrawableId(Context context, String type, String name) {
        String originType = null;
        originType = DocumentsUiDrmHelper.getOriginalMimeType(context, name, type,isDrmEnable);
        Log.i(TAG, "getOriginalMimeType  result --  " + originType);
        int rightsStates = DocumentsUiDrmHelper.getRightsStatus(context, name, type);
        if (originType == null){
            return R.drawable.ic_doc_generic_am;
        }
        if (originType.startsWith("image/")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_image_unlock;
            } else {
                return R.drawable.drm_image_lock;
            }

        } else if (originType.startsWith("audio/") || originType.equalsIgnoreCase("application/ogg")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_audio_unlock;
            } else {
                return R.drawable.drm_audio_lock;
            }
        } else if (originType.startsWith("video/")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_video_unlock;
            } else {
                return R.drawable.drm_video_lock;
            }
        } else {
            return R.drawable.ic_doc_generic_am;
        }
    }

    public String getDrmFilenameFromPath(String path){
        String drmName = path;
        if (path != null){
            int index = path.lastIndexOf("/");
            if (index > 0){
                drmName = path.substring(index+1);
            }
        }
        return drmName;
    }

    @Override
    public String getDrmPath(Context context , Uri uri){
        String mDrmPath;
        mDrmPath = DocumentUriUtil.getPath(context, uri);
        return mDrmPath;
    }

    @Override
    public boolean getIsDrm(Context context , String drmPath){
        boolean mIsDrm = false;
        if (drmPath != null){
            mIsDrm = DocumentsUiDrmHelper.isDrmMimeType(context, drmPath, null,isDrmEnable);
        }
        return mIsDrm;
    }

    @Override
    public Bitmap getIconBitmap(Context context , String docMimeType , String docDisplayName){
        if (isDrmEnable && docMimeType != null && docMimeType.equals(DocumentsUiDrmHelper.DRM_CONTENT_TYPE)) {
            int icon_id =  getDrmIconMimeDrawableId(context, docMimeType, docDisplayName);
            if (icon_id != ERROR) {
                return BitmapFactory.decodeResource(sContext.getResources(), icon_id);
            }
        }
        return null;
    }
}
