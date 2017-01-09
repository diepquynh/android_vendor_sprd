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

package addon.sprd.downloadprovider;

import android.R.integer;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.drm.DrmConvertedStatus;
import android.drm.DrmErrorEvent;
import android.drm.DrmEvent;
import android.drm.DrmInfo;
import android.drm.DrmInfoEvent;
import android.drm.DrmInfoRequest;
import android.drm.DrmInfoStatus;
import android.drm.DrmManagerClient;
import android.drm.ProcessedData;
import android.drm.DrmManagerClient.OnErrorListener;
import android.drm.DrmRights;
import android.drm.DrmManagerClient.OnEventListener;
import android.net.Uri;
import android.util.Log;
import android.provider.Downloads;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import addon.sprd.downloadprovider.DownloadDRMUltil;
import android.app.DownloadManager;
import com.android.providers.downloads.Helpers;

public class DownloadDrmHelper {

    /** The MIME type of special DRM files */
    public static final String MIMETYPE_DRM_MESSAGE = "application/vnd.oma.drm.message";

    /** The extensions of special DRM files */
    public static final String EXTENSION_DRM_MESSAGE = ".dm";

    public static final String EXTENSION_INTERNAL_FWDL = ".fl";

    public static final String DRM_CONTENT_TYPE = "application/vnd.oma.drm.content";
    public static final String DRM_RIGHT_TYPE ="application/vnd.oma.drm.rights+xml";
    public static final String DRM_TRIGGER_TYPE = "application/vnd.oma.drm.message";
    private static String TAG = "DownloadDrmHelper";
    public static DrmManagerClient mClient = new DrmManagerClient(null);
    /**
     * Checks if the Media Type is a DRM Media Type
     *
     * @param drmManagerClient A DrmManagerClient
     * @param mimetype Media Type to check
     * @return True if the Media Type is DRM else false
     */
    public static boolean isDrmMimeType(Context context, String path, String mimetype) {
        if (!DownloadDRMUltil.isDrmEnabled()){
            return false;
        }
        boolean result = false;
        if (context != null) {
            try {
                if (path == null){
                    return false;
                }
                result = mClient.canHandle(path, mimetype);
            } catch (IllegalArgumentException e) {
                Log.w(TAG, "DrmManagerClient instance could not be created, context is Illegal.");
            } catch (Exception e) {
                Log.w(TAG, "DrmManagerClient didn't initialize properly."+e);
            }
        }
        return result;
    }

    public static boolean isSupportDRMType(Context context,String filename,String mimetype){
        String type = mimetype;
        type = DownloadDrmHelper.getOriginalMimeType(context, filename, mimetype);
        if (type != null){
              if (type.startsWith("video/") || type.startsWith("image/")
                      || type.startsWith("audio/") || type.equalsIgnoreCase("application/ogg")){
                    return true;
             }
        }
        return false;
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
    public static String getOriginalMimeType(Context context, String path, String containingMime) {
        String result = containingMime;

        try {
            if (path != null && !path.endsWith(".dcf")){
                return containingMime;
            }
            if (DownloadDRMUltil.isDrmEnabled() && mClient.canHandle(path, null)) {
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
                    Log.i(TAG, "processInfo  onEvent");
                    DrmInfoStatus status = (DrmInfoStatus)event.getAttribute(DrmEvent.DRM_INFO_STATUS_OBJECT);
                    if (status.statusCode == status.STATUS_OK){
                        if (new File(path).exists()){
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
                                     values.put(Downloads.Impl.COLUMN_MIME_TYPE, DownloadDrmHelper.DRM_CONTENT_TYPE);
                                     values.put(Downloads.Impl.COLUMN_FILE_NAME_HINT, "file://"+desfile);
                                     int updatenum = context.getContentResolver().update(uri, values, null, null);
                                     Log.i(TAG, "processDrmInfo  mClient.acquireDrmInfo desFile "+desfile + "  updatenum --  "+updatenum);
                                 }
                       }
                       if (cursor != null){
                           cursor.close();
                           android.media.MediaScannerConnection.scanFile(context,
                                   new String[]{ desfile }, null, null);
                       }
                    }else{
                        Log.v(TAG, "processDrmInfo  onEvent  status.statusCode == "+status.statusCode);
                    }
                }
            });
          mClient.setOnErrorListener(new OnErrorListener() {

            @Override
            public void onError(DrmManagerClient client, DrmErrorEvent event) {
                // TODO Auto-generated method stub
                Log.w(TAG, "onErrorListener onError");
                if (mDce != null){
                    mDce.onError();
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
        } catch (Exception e) {
            // TODO: handle exception
            Log.e(TAG, "Exception  -- "+e.toString());
        }

        return result;
    }
    public static String getDRMDownloadPath(String path){
        String downloadPath = null;
        if (path != null){
            int slashIndex = path.lastIndexOf("/");
            if (slashIndex > 0){
                downloadPath = path.substring(0, slashIndex+1);
            }
        }
        return downloadPath;
    }
    public static String getDRMDownloadName(String path){
        String filename = null;
        if (path != null){
            int slashIndex = path.lastIndexOf("/");
            int dotIndex = path.lastIndexOf(".");
            if (slashIndex > 0 && dotIndex > 0){
                filename = path.substring(slashIndex+1, dotIndex);
            }
        }
        return filename;
    }

    public static String getHiddenDataDirectory(String path){
        String hiddenDir = null;
        if (path != null){
            int slashIndex = path.lastIndexOf("/");
            if (slashIndex > 0){
                hiddenDir = path.substring(0, slashIndex+1);
            }
        }
        return hiddenDir;
    }
    public static String createHiddenDirectory(String path){
        String hiddenPath = getHiddenDataDirectory(path);
        String dir = hiddenPath+".tempdrm";
        File file = new File(dir);
        if (file.exists()){
            if (file.isDirectory()){
                return dir;
            }else{
                boolean sucCreate = file.mkdir();
                if (sucCreate){
                    return dir;
                }else{
                    return null;
                }
            }

        }else{
            boolean sucCreate = file.mkdir();
            if (sucCreate){
                return dir;
            }else{
                return null;
            }
        }
    }
    public static String getDRMTempDestinationPath(String original){
        String tempFile = null;
        String dir = createHiddenDirectory(original);
        String name = getDRMDownloadName(original);
        if (dir != null){
            tempFile = dir + "/" + name + ".dcf";
        }
        return tempFile;
    }

    public static String getDRMDestinationPath(String original){
        String file = null;
        String path = getDRMDownloadPath(original);
        String name = getDRMDownloadName(original);
        file = path + name + ".dcf";
        return file;
    }
    private static int getConvertSessionId(String mimeType){
        int id = mClient.openConvertSession(mimeType);
        return id;
    }
    private static void operateProcessDrmInfoOnError(Context context, String path){
        Log.i("DRMinfoOnError", "operateProcessDrmInfoOnError  path   "+path);
        if (path != null){
            if (new File(path).exists()){
               boolean deleteFile = new File(path).delete();
               boolean deleteTemp = new File(getDRMTempDestinationPath(path)).delete();
               Log.i("DRMinfoOnError", " deleteFile-   "+deleteFile+" --  deleteTemp  "+deleteTemp);
            }
        }
        Cursor cursor = context.getContentResolver().query(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                new String[]{Downloads.Impl._ID},
                Downloads.Impl._DATA + "=?",
                new String[] { path },
                null);

        if(cursor != null){
            int idColumn = cursor.getColumnIndexOrThrow(Downloads.Impl._ID);
            final DownloadManager dm = (DownloadManager) context.getSystemService(
                    Context.DOWNLOAD_SERVICE);
            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                 long id = cursor.getLong(idColumn);
                 Log.d("DRMinfoOnError", "delete database--id  "+id);
                 dm.setAccessAllDownloads(true);
                 dm.remove(id);
            }
            Helpers.getDownloadNotifier(context).update();
       }
       if (cursor != null){
           cursor.close();
       }
    }
    private static void deleteDownloadFromDB(Context context, String path){
        String name = null;
        String title = null;
        String newFileName = null;
        if (path != null){
            if (new File(path).exists()){
               boolean delete = new File(path).delete();
               Log.i("deleteDownloadFromDB", "processConvertDrmInfo --delete  "+delete);
               boolean rename = new File(getDRMTempDestinationPath(path)).renameTo(new File(getDRMDestinationPath(path)));
               Log.i("deleteDownloadFromDB", "rename  "+rename+"  getDRMDestinationPath(path) "+getDRMDestinationPath(path));
            }
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
                     values.put(Downloads.Impl._DATA, getDRMDestinationPath(path));
                     values.put(Downloads.Impl.COLUMN_TITLE, getDRMDownloadName(path)+".dcf");
                     values.put(Downloads.Impl.COLUMN_MIME_TYPE, DownloadDrmHelper.DRM_CONTENT_TYPE);
                     values.put(Downloads.Impl.COLUMN_FILE_NAME_HINT, "file://"+getDRMDestinationPath(path));
                     int updatenum = context.getContentResolver().update(uri, values, null, null);
                     Log.i(TAG, "processDrmInfo  mClient.acquireDrmInfo desFile "+getDRMDestinationPath(path) + "  updatenum --  "+updatenum);
                 }

       }
       if (cursor != null){
           cursor.close();
           android.media.MediaScannerConnection.scanFile(context,
                   new String[]{ getDRMDestinationPath(path) }, null, null);

       }
    }
    public static int processConvertDrmInfo(final Context context, final String path, final String mimeType, final String destinationPath) {
        int result = -1;
        final int id = getConvertSessionId(mimeType);
        final DrmManagerClient processClient = new DrmManagerClient(null);
        Log.i("processConvertDrmInfo", "processConvertSession --id   "+id + "  path  "+path);
        try {
            processClient.setOnEventListener(new OnEventListener() {
                @Override
                public void onEvent(DrmManagerClient client, DrmEvent event) {
                    // TODO Auto-generated method stub
                    Log.i("processConvertDrmInfo", "processInfo  onEvent path  "+path);
                    FileOutputStream stream = null;
                    try {
                        File file = new File(getDRMTempDestinationPath(path));
                        if (file == null){
                            Log.i("processConvertDrmInfo", "file == null");
                        }
                        if (file.exists()){
                            boolean delete = file.delete();
                            if (delete){
                                boolean create = file.createNewFile();
                                Log.i("processConvertDrmInfo", "file exist createFile  "+create);
                            }else{
                                Log.i("closeConvertSession", "close convert session file delete  "+delete);
                                DrmConvertedStatus close = processClient.closeConvertSession(id);
                                return;
                            }
                        } else {
                            boolean create = file.createNewFile();
                            Log.i("processConvertDrmInfo", "file not exist createFile  "+create);
                        }
                        DrmInfoStatus status = (DrmInfoStatus)event.getAttribute(DrmEvent.DRM_INFO_STATUS_OBJECT);
                        ProcessedData data = status.data;
                        byte [] header = data.getData();
                        Log.i("processConvertDrmInfo", "processConvertSession --header id  "+id+"   header length  "+header.length);
                        stream = new FileOutputStream(getDRMTempDestinationPath(path));
                        stream.write(header);
                        while(true){
                            DrmConvertedStatus convertSatus = processClient.convertData(id, new byte[1]);
                            Log.w("processConvertDrmInfo", "convertStatus --- "+convertSatus);

                            if (convertSatus.statusCode == DrmConvertedStatus.STATUS_OK){
                                final byte[] body = convertSatus.convertedData;
                                if (body != null){
                                    Log.w("processConvertDrmInfo", "processConvertSession --body id  "+id+"  body length  "+body.length);
                                    if (body.length > 0){
                                        stream.write(body);
                                    }else{
                                        DrmConvertedStatus close = processClient.closeConvertSession(id);
                                        Log.i("closeConvertSession", "processConvertSession --close   "+close);
                                        final byte[] padding = close.convertedData;
                                        Log.i("processConvertDrmInfo", "processConvertSession --padding  id  "+id+"   padding.length   "+padding.length);
                                        stream.write(padding);
                                        break;
                                    }
                                }else{
                                    DrmConvertedStatus close = processClient.closeConvertSession(id);
                                    Log.i("closeConvertSession", "processConvertSession -else -close   "+close);
                                    final byte[] padding = close.convertedData;
                                    Log.i("processConvertDrmInfo", "processConvertSession --padding   id  "+id+"   padding.length   "+padding.length);
                                    stream.write(padding);
                                    break;
                                }

                            }else if (convertSatus.statusCode == DrmConvertedStatus.STATUS_ERROR){
                                Log.i("closeConvertSession", "processConvertSession --statusError  id "+id);
                                DrmConvertedStatus close = processClient.closeConvertSession(id);
                                final byte[] padding = close.convertedData;
                                stream.write(padding);
                                break;
                            }
                            Thread.sleep(3);
                        }
                        deleteDownloadFromDB(context,path);
                        if (mDce != null){
                            mDce.onSuccess();
                        }
                        if (stream != null){
                            stream.close();
                            stream = null;
                        }
                    } catch (Exception e) {
                        Log.e("closeConvertSession", "exception in writeFile----path "+path+"  exception " + e.toString());
                        operateProcessDrmInfoOnError(context,path);
                        DrmConvertedStatus close = processClient.closeConvertSession(id);
                        if (mDce != null){
                            mDce.onError();
                        }
                        try {
                            if (stream != null){
                                stream.close();
                                stream = null;
                            }
                        } catch (Exception e2) {
                            Log.e("converDataOnEvent", "exception in stream close---path --  "+path +"  exception  "+e.toString());
                        }
                    }
                }
            });
            processClient.setOnErrorListener(new OnErrorListener() {

            @Override
            public void onError(DrmManagerClient client, DrmErrorEvent event) {
                // TODO Auto-generated method stub
                DrmConvertedStatus close = processClient.closeConvertSession(id);
                operateProcessDrmInfoOnError(context,path);
                Log.e("converDataOnError", "exception in onError---path "+path);
                if (mDce != null){
                    mDce.onError();
                }
            }
        });

            DrmInfoRequest reqest = new DrmInfoRequest(DrmInfoRequest.TYPE_REGISTRATION_INFO, mimeType);
            reqest.put("file_in", path);
            reqest.put("file_out", getDRMTempDestinationPath(path));
            reqest.put("convert_id", String.valueOf(id));
            DrmInfo info = processClient.acquireDrmInfo(reqest);
            result = processClient.processDrmInfo(info);
            Log.i(TAG, "processDrmInfo -- result  "+result+"destination   "+getDRMTempDestinationPath(path));

        } catch (Exception e) {
            // TODO: handle exception
            operateProcessDrmInfoOnError(context, path);
            DrmConvertedStatus close = processClient.closeConvertSession(id);
            Log.e("closeConvertSession", "exception in processConvertDrmInfo--- "+e.toString()+"  cause  "+e.getCause());
            Log.e(TAG, "Exception  -- "+e.toString());
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

    public static boolean isDrmCDFile(Context context, String path, String mimeType) {
        String cdType = null;
        try {
            ContentValues values = mClient.getMetadata(path);
            if (values != null){
                cdType = values.getAsString("extended_data");
                Log.w(TAG, "isDrmCDFile  -- "+cdType);
            }
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "isDrmSDFile IllegalArgumentException "+ex.toString());
        } catch (IllegalStateException ex) {
            Log.w(TAG, "isDrmSDFile IllegalStateException "+ex.toString());
        }

        if (cdType != null && cdType.equals("cd")){
            return true;
        }
        return false;
    }

    public interface DrmClientOnErrorListener{
        public void onError();
        public void onSuccess();
    }
    public static DrmClientOnErrorListener mDce;
    public static void setDrmClientOnErrorListener(DrmClientOnErrorListener dce) {
        DownloadDrmHelper.mDce = dce;
    }
}
