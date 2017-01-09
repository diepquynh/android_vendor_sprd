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

package addon.sprd.downloadprovider;

import static android.app.DownloadManager.COLUMN_URI;
import java.io.File;
import android.R.bool;
import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.DownloadManager;
import android.app.ProgressDialog;
import android.app.DownloadManager.Query;
import android.app.FragmentManager;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.drm.DrmStore.DrmObjectType;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import com.android.providers.downloads.Constants;
import com.android.providers.downloads.OpenHelper;
import libcore.io.IoUtils;
import android.os.SystemProperties;
import android.drm.DrmStore.RightsStatus;
import addon.sprd.downloadprovider.DownloadDrmHelper;
import addon.sprd.downloadprovider.DownloadDrmHelper.DrmClientOnErrorListener;
import static android.app.DownloadManager.COLUMN_MEDIA_TYPE;

/**
 * Intercept all download clicks to provide special behavior. For example,
 * PackageInstaller really wants raw file paths.
 */
public class DownloadDRMUltil {
    private static final String TAG = "DownloadProviderHelper";
    public static final int SHOW_DOwnLOADS = 1;
    public static final int SHOW_CONSUME_DIALOG = 2;
    public static final int SHOW_OPEN_FILE = 3;
    public static final int SHOW_RENEW_DRM_FILE = 4;

    public static boolean isShowDrmConsumeDilog(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimeType);
        if (isDrm){
            int drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
            if (drmType == DrmStore.DrmObjectType.CONTENT){
                int rightsStates = DownloadDrmHelper.getRightsStatus(context, filename, mimeType);
                if (rightsStates == RightsStatus.RIGHTS_VALID){
                    if (DownloadDrmHelper.isDrmFLFile(context, filename, mimeType)){
                        return false;
                    }else{
                        return true;
                    }
                }
            }
        }else{
            return false;
        }
        return false;
    }

    public static boolean isDrmCDFile(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimeType);
        if (isDrm){
            int drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
            if (drmType == DrmStore.DrmObjectType.CONTENT){
                int rightsStates = DownloadDrmHelper.getRightsStatus(context, filename, mimeType);
                if (rightsStates == RightsStatus.RIGHTS_VALID){
                    return false;
                }else{
                    if (DownloadDrmHelper.isDrmCDFile(context, filename, mimeType)){
                        return true;
                    }
                }
            }
        }
        return false;
    }

    public static boolean isShowDrmRenewDilog(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename,mimeType);
        Log.d(TAG, "isDrm --  "+isDrm);
        if (isDrm){
            int drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
//            final String originalMimeType = DownloadDrmHelper.getOriginalMimeType(c, drmPath, mimeType);
            //mimeType = originalMimeType;
            Log.d(TAG, "drmType --  " + drmType + " DrmStore.DrmObjectType.CONTENT = " + DrmStore.DrmObjectType.CONTENT);
            if (drmType == DrmStore.DrmObjectType.CONTENT){

                Log.d(TAG, "isShowDrmRenewDilog --  DrmStore.DrmObjectType.CONTENT =" + DrmStore.DrmObjectType.CONTENT);
                int rightsStates = DownloadDrmHelper.getRightsStatus(context, filename, mimeType);
                Log.d(TAG, "isShowDrmRenewDilog --rightsStates ="+rightsStates + "RightsStatus.RIGHTS_VALID = " +RightsStatus.RIGHTS_VALID);
                if (rightsStates == RightsStatus.RIGHTS_VALID){
                    return false;
                }else{
                    return true;
                }
            }
        }else{
            return false;
        }
        return false;
    }

    public static boolean isShowDrmInstallDilog(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimeType);
        Log.d(TAG, "isDrm --  " + isDrm + "filename:" + filename + "mimeType" + mimeType);
        if (isDrm){
            int drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
            if (drmType == DrmStore.DrmObjectType.CONTENT){
                return false;
            }else if (drmType == DrmStore.DrmObjectType.RIGHTS_OBJECT){
                return true;
            }else if (drmType == DrmStore.DrmObjectType.TRIGGER_OBJECT){
                return true;
            }
        }else{
            return false;
        }
        return false;
    }

    public static int getInstallDrmType(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimeType);
        int drmType = -1;
        Log.d(TAG, "isDrm" + isDrm + "filename:" + filename + "mimeType" + mimeType);
        if (isDrm){
             drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
        }
        Log.d(TAG, "drmType" + drmType);
        return drmType;
    }

    public static String getOriginalMimeType(Context context,String filename, String mimeType){
        String type = mimeType;
        Log.d(TAG, "getOriginalMimeType type" + type);
        if (filename != null){
            type = DownloadDrmHelper.getOriginalMimeType(context, filename, mimeType);
        }
        Log.d(TAG, "getOriginalMimeType after type" + type);
        return type;
    }

    public static boolean isDrmType(Context context, String filename, String mimetype){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimetype);
        Log.d(TAG, "isDrmType isDrm = " + isDrm);
        return isDrm;
    }

    public static boolean isSupportDRMType(Context context,String filename,String mimetype){
        String type = mimetype;
        type = DownloadDrmHelper.getOriginalMimeType(context, filename, mimetype);
        Log.d(TAG, "isSupportDRMType type = " + type);
        if (type != null){
              if (type.startsWith("video/") || type.startsWith("image/")
                      || type.startsWith("audio/") || type.equalsIgnoreCase("application/ogg")){
                    return true;
             }
        }
        return false;
    }

  //add for drm
    public static boolean isDrmEnabled() {
        String prop = SystemProperties.get("drm.service.enabled");
        Log.d(TAG, "isDrmEnabled type = " + prop);
        return prop != null && prop.equals("true");
    }

    public static int getRightsIntoDownloads(Context context, String filename, String mimetype){
        Log.d(TAG, "getRightsIntoDownloads:" + filename + "mimetype:" + mimetype);
        if (DownloadDrmHelper.isDrmMimeType(context, filename, mimetype)){
            if (!DownloadDrmHelper.isSupportDRMType(context, filename, mimetype)){
                return SHOW_DOwnLOADS;
            }
            if (filename != null && filename.endsWith(".dm")){
                return SHOW_DOwnLOADS;
            }
            if (mimetype.equals(DownloadDrmHelper.DRM_CONTENT_TYPE)){
                int rightsStates = DownloadDrmHelper.getRightsStatus(context, filename, mimetype);
                if (rightsStates == RightsStatus.RIGHTS_VALID){
                    if (!DownloadDrmHelper.isDrmFLFile(context, filename, mimetype)){
                        return SHOW_CONSUME_DIALOG;
                    }else{
                        return SHOW_OPEN_FILE;
                    }
                }else {
                    return SHOW_RENEW_DRM_FILE;
                }
            }else{
                return SHOW_DOwnLOADS;
            }
        }
        return SHOW_OPEN_FILE;
    }

    public static Intent startPluginViewIntent(Context context, long id, int intentFlags){
        Intent intent = OpenHelper.buildViewIntent(context, id);
        Log.d(TAG, "startPluginViewIntent: id = " + id + "intentFlags:" + intentFlags);
        if (intent != null){
            Log.d(TAG, "startPluginViewIntent: intent != null");
            intent.addFlags(intentFlags);
        }
        return intent;
    }

    public static String getDRMPluginMimeType(Context context, File file, String mimeType, Cursor cursor) {
        Log.d(TAG, "getDRMPluginMimeType:mimeType =  " + mimeType);
        return cursor.getString(cursor.getColumnIndexOrThrow(COLUMN_MEDIA_TYPE));
    }

    public static Intent setDRMPluginIntent(File file, String mimeType) {
        Log.d(TAG, "getDRMPluginMimeType:mimeType =  " + mimeType);
        return (new Intent(Intent.ACTION_VIEW));
    }

    public static boolean isDrmFLFileInValid(Context context, String filename, String mimeType){
        boolean isDrm = DownloadDrmHelper.isDrmMimeType(context, filename, mimeType);
        Log.d(TAG, "isDrmFLFileInValid isDrm--- " + isDrm);
        if (isDrm){
            int drmType = DownloadDrmHelper.getDrmObjectType(context, filename, mimeType);
            Log.d(TAG, "isDrmFLFileInValid drmType--- " + drmType);
            if (drmType == DrmStore.DrmObjectType.CONTENT){
                int rightsStates = DownloadDrmHelper.getRightsStatus(context, filename, mimeType);
                Log.d(TAG, "isDrmFLFileInValid rightsStates--- " + rightsStates);
                if (rightsStates == RightsStatus.RIGHTS_VALID){
                    Log.d(TAG, "isDrmFLFileInValid rightsStates--- " + rightsStates);
                    return false;
                }else{
                    if (DownloadDrmHelper.isDrmFLFile(context, filename, mimeType)){
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /* SPRD: add downloadprovider drm plugin start, 2014-11-3. @{ */
    /*
    public void checkCanHandleDownload(Context context, String mimeType, int destination,
            boolean isPublicApi) throws StopRequestException {
        if (isPublicApi) {
            return;
        }

        if (destination == Downloads.Impl.DESTINATION_EXTERNAL
                || destination == Downloads.Impl.DESTINATION_CACHE_PARTITION_PURGEABLE) {
            if (mimeType == null) {
                throw new StopRequestException(Downloads.Impl.STATUS_NOT_ACCEPTABLE,
                        "external download with no mime type not allowed");
            }
            if (!DownloadDrmHelper.isDrmMimeType(context, null, mimeType)) {
                // Check to see if we are allowed to download this file. Only files
                // that can be handled by the platform can be downloaded.
                // special case DRM files, which we should always allow downloading.
                Intent intent = new Intent(Intent.ACTION_VIEW);

                // We can provide data as either content: or file: URIs,
                // so allow both.  (I think it would be nice if we just did
                // everything as content: URIs)
                // Actually, right now the download manager's UId restrictions
                // prevent use from using content: so it's got to be file: or
                // nothing

                PackageManager pm = context.getPackageManager();
                intent.setDataAndType(Uri.fromParts("file", "", null), mimeType);
                ResolveInfo ri = pm.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
                //Log.i(Constants.TAG, "*** FILENAME QUERY " + intent + ": " + list);

                if (ri == null) {
                    if (Constants.LOGV) {
                        Log.v(Constants.TAG, "no handler found for type " + mimeType);
                    }
                    throw new StopRequestException(Downloads.Impl.STATUS_NOT_ACCEPTABLE,
                            "no handler found for this download type");
                }
            }
        }
    }
    */
    /* SPRD: add downloadprovider drm plugin end, 2014-11-3. @} */

}
