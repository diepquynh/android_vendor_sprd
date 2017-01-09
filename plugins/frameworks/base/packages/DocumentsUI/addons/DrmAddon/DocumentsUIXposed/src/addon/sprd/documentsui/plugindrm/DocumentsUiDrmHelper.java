/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *
 */

package addon.sprd.documentsui.plugindrm;

import android.R.integer;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.drm.DrmEvent;
import android.drm.DrmInfo;
import android.drm.DrmInfoRequest;
import android.drm.DrmInfoStatus;
import android.drm.DrmManagerClient;
import android.drm.DrmRights;
import android.drm.DrmManagerClient.OnEventListener;
import android.net.Uri;
import android.util.Log;
import android.provider.Downloads;
import android.drm.DrmStore;
import android.drm.DrmStore.RightsStatus;
import android.media.MediaFile;
import addon.sprd.documentsui.plugindrm.AddOnDocumentsUIPlugInDrm;

import java.io.File;
import java.io.IOException;

public class DocumentsUiDrmHelper {
    /** The MIME type of special DRM files */
    public static final String MIMETYPE_DRM_MESSAGE = "application/vnd.oma.drm.message";

    /** The extensions of special DRM files */
    public static final String EXTENSION_DRM_MESSAGE = ".dm";

    public static final String EXTENSION_INTERNAL_FWDL = ".fl";

    public static final String DRM_CONTENT_TYPE = "application/vnd.oma.drm.content";
    public static final String DRM_RIGHT_TYPE ="application/vnd.oma.drm.rights+xml";
    public static final String DRM_TRIGGER_TYPE = "application/vnd.oma.drm.message";
    private static String TAG = "DocumentUiDrmHelper";
    private static DrmManagerClient mClient = new DrmManagerClient(null);
    /**
     * Checks if the Media Type is a DRM Media Type
     *
     * @param drmManagerClient A DrmManagerClient
     * @param mimetype Media Type to check
     * @return True if the Media Type is DRM else false
     */
    public static boolean isDrmMimeType(Context context, String path, String mimetype, boolean isDrmEnable) {
        if (!isDrmEnable){
            return false;
        }
        if (path != null) {
            if (!isDrmFile(path)) {
                return false;
            }
        }
        boolean result = false;
        if (context != null) {
            try {
                if (mimetype != null){
                    String tempPath = null;
                    result = mClient.canHandle(tempPath, mimetype);
                }else{
                    result = mClient.canHandle(path, mimetype);
                }

            } catch (IllegalArgumentException e) {
                Log.w(TAG, "DrmManagerClient instance could not be created, context is Illegal.");
            } catch (IllegalStateException e) {
                Log.w(TAG, "DrmManagerClient didn't initialize properly.");
            }
        }
        return result;
    }

    /**
     * Checks if the Media Type needs to be DRM converted
     *
     * @param mimetype Media type of the content
     * @return True if convert is needed else false
     */
    public static boolean isDrmConvertNeeded(String mimetype) {
        return MIMETYPE_DRM_MESSAGE.equals(mimetype);
    }

    /**
     * Modifies the file extension for a DRM Forward Lock file NOTE: This
     * function shouldn't be called if the file shouldn't be DRM converted
     */
    public static String modifyDrmFwLockFileExtension(String filename) {
        if (filename != null) {
            int extensionIndex;
            extensionIndex = filename.lastIndexOf(".");
            if (extensionIndex != -1) {
                filename = filename.substring(0, extensionIndex);
            }
            filename = filename.concat(EXTENSION_INTERNAL_FWDL);
        }
        return filename;
    }

    /**
     * Gets the original mime type of DRM protected content.
     *
     * @param context The context
     * @param path Path to the file
     * @param containingMime The current mime type of of the file i.e. the
     *            containing mime type
     * @return The original mime type of the file if DRM protected else the
     *         currentMime
     */
    public static String getOriginalMimeType(Context context, String path, String containingMime, boolean isDrmEnable) {
        String result = containingMime;
        try {
            if (isDrmEnable && mClient.canHandle(path, null)) {
                if (path != null && !path.endsWith(".dcf")){
                    return containingMime;
                }
                result = mClient.getOriginalMimeType(path);
                Log.i(TAG,"getOriginalMimeType -- type  "+result+"   path   "+path);
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "getOriginalMimeType IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "getOriginalMimeType IllegalStateException "+ex.toString());
        }
        return result;
    }

    public static int getDrmObjectType(Context context, String path, String mimeType) {
        int result = 0;
        try {
            result = mClient.getDrmObjectType(path, mimeType);
            Log.i(TAG,"getDrmObjectType -- result  "+result+"   path   "+path+" mimeType  "+mimeType);
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "getDrmObjectType IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "getDrmObjectType IllegalStateException "+ex.toString());
        }
        return result;
    }

    public static int saveDrmObjectRights(Context context, String path, String mimeType) {
        int result = -1;
        try {
            DrmRights drmRights = new DrmRights(new File(path), mimeType);
            result = mClient.saveRights(drmRights, null, null);
            Log.i(TAG,"saveDrmObjectRights -- result  "+result+"   path   "+path+" mimeType  "+mimeType);
        }  catch (IllegalArgumentException ex) {
            Log.w(TAG, "saveDrmObjectRights IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "saveDrmObjectRights IllegalStateException "+ex.toString());
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return result;
    }

    //porcess rights and not locked
  //porcess rights and not locked
    public static int processDrmInfo(final Context context, final String path, final String mimeType, final String destinationPath) {
        int result = -1;
        try {
            mClient.setOnEventListener(new OnEventListener() {
                @Override
                public void onEvent(DrmManagerClient client, DrmEvent event) {
                    // TODO Auto-generated method stub
                    DrmInfoStatus status = (DrmInfoStatus)event.getAttribute(DrmEvent.DRM_INFO_STATUS_OBJECT);
                    if (status.statusCode == status.STATUS_OK){
                        if (path != null && new File(path).exists()){
                            boolean delete = new File(path).delete();
                            Log.i(TAG, "processDrmInfo status.STATUS_OK delete file  "+path);
                        }
                        Log.i(TAG, "processDrmInfo status_ok path "+path+"  mimeType  "+mimeType);
                        String desfile = null;
                        String name = null;
                        String title = null;
                        if (path != null){
                            int index = path.lastIndexOf("/");
                            int dotIndex = path.lastIndexOf(".");
                            if (dotIndex > 0){
                                desfile = path.substring(0, dotIndex) + ".dcf";
                            }
                            name = path.substring(index + 1, dotIndex);
                            title = name+".dcf";
                        }
                        Cursor cursor = context.getContentResolver().query(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                                new String[]{Downloads.Impl._ID},
                                Downloads.Impl._DATA + "=?",
                                new String[] { path },
                                null);

                        if(cursor != null){
                            int idColumn = cursor.getColumnIndexOrThrow(Downloads.Impl._ID);
                            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                                 long id = cursor.getLong(idColumn);
                                 Uri uri = ContentUris.withAppendedId(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                                         id);
                                     ContentValues values = new ContentValues();
                                     values.put(Downloads.Impl._DATA, desfile);
                                     values.put(Downloads.Impl.COLUMN_TITLE, title);
                                     values.put(Downloads.Impl.COLUMN_MIME_TYPE, DocumentsUiDrmHelper.DRM_CONTENT_TYPE);
                                     values.put(Downloads.Impl.COLUMN_FILE_NAME_HINT, "file://"+desfile);
                                     int updatenum = context.getContentResolver().update(uri, values, null, null);
                                     Log.i(TAG, "processDrmInfo  mClient.acquireDrmInfo desFile "+desfile + "  updatenum --  "+updatenum);
                                 }

                       }
                       if (cursor != null){
                           cursor.close();
                       }

                    }else{
                        Log.v(TAG, "processDrmInfo  onEvent  status.statusCode == "+status.statusCode);
                    }
                }
            });
          String destinationfile = null;
          if (path != null){
              int dotIndex = path.lastIndexOf(".");
              if (dotIndex > 0){
                  destinationfile = path.substring(0, dotIndex) + ".dcf";
              }
          }
          DrmInfoRequest reqest = new DrmInfoRequest(DrmInfoRequest.TYPE_REGISTRATION_INFO, mimeType);
          reqest.put("file_in", path);
          reqest.put("file_out", destinationfile);
          DrmInfo info = mClient.acquireDrmInfo(reqest);
          result = mClient.processDrmInfo(info);
          Log.i(TAG, "processDrmInfo -- result  "+result+"destination   "+destinationfile);
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "processDrmInfo IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "processDrmInfo IllegalStateException "+ex.toString());
        }

        return result;
    }


    public static int getRightsStatus(Context context, String path, String mimeType) {
        int result = 0;
        try {
            result = mClient.checkRightsStatus(path);
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "getRightsStatus IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "getRightsStatus IllegalStateException "+ex.toString());
        }
        return result;
    }

    public static boolean isDrmFLFile(Context context, String path, String mimeType) {
        String flType = null;
        try {
            ContentValues values = mClient.getMetadata(path);
            if (values != null){
                flType = values.getAsString("extended_data");
                Log.w(TAG, "isDrmFile  -- "+flType);
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "isDrmFLFile IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "isDrmFLFile IllegalStateException "+ex.toString());
        }
        if (flType != null && flType.equals("fl")){
            return true;
        }
        return false;
    }

    public static boolean isDrmSDFile(Context context, String path, String mimeType) {
        String sdType = null;
        try {
            ContentValues values = mClient.getMetadata(path);
            Log.e(TAG, "values -- "+values);
            if (values != null){
                sdType = values.getAsString("extended_data");
                Log.w(TAG, "isDrmSDFile  -- "+sdType);
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "isDrmSDFile IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "isDrmSDFile IllegalStateException "+ex.toString());
        }

        if (sdType != null && sdType.equals("sd")){
            return true;
        }
        return false;
    }

    public static String renewDrmRightsDownload(Context context, String path, String mimeType) {
        String url = null;
        try {
            ContentValues values = mClient.getMetadata(path);
            Log.i(TAG, "getMetadata contentvalues  "+values);
            if (values != null){
                Log.w(TAG, "renew rights -- "+values);
                String httpUrl = values.getAsString("rights_issuer");
                url = httpUrl;
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "renewDrmRightsDownload IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "renewDrmRightsDownload IllegalStateException "+ex.toString());
        }
        Log.w(TAG, "renewDrmRightsDownload  -- url  "+url);
        return url;
    }

    public static boolean isDrmCanTransfer(Context context, String path, String mimeType) {
        int status;
        try {
            status = mClient.checkRightsStatus(path, DrmStore.Action.TRANSFER);
            if (status == RightsStatus.RIGHTS_VALID){
                return true;
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "isDrmSDFile IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "isDrmSDFile IllegalStateException "+ex.toString());
        }

        return false;
    }

    public static boolean isDrmFile(String filePath){
        if(MediaFile.getFileType(filePath) != null
                && (MediaFile.isDrmFileType(MediaFile.getFileType(filePath).fileType))){
            return true;
        }
        return false;
    }

    public static boolean isDrmValid(String filePath) {
        try {
            int status = mClient.checkRightsStatus(filePath);
            if (status == RightsStatus.RIGHTS_VALID){
                return true;
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "isDrmValid IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "isDrmValid IllegalStateException "+ex.toString());
        }

        return false;
    }
}
