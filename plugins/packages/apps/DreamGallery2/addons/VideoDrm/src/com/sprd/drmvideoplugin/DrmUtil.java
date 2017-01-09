package com.sprd.drmvideoplugin;

import android.content.ContentValues;
import android.content.Context;
import android.content.CursorLoader;
import android.database.Cursor;
import android.database.CursorIndexOutOfBoundsException;
import android.drm.DrmManagerClientEx;
import android.drm.DrmStore;
import android.net.Uri;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.util.Log;

import com.android.gallery3d.app.MovieActivity;

public class DrmUtil {

    private static final String TAG = "DrmUtil";


    public static final String DRM_FILE_TYPE = "extended_data";
    public static String FL_DRM_FILE = "fl";
    public static String CD_DRM_FILE = "cd";
    public static String SD_DRM_FILE = "sd";

    public static boolean isDrmFile(String filePath, String mimeType) {
        DrmManagerClientEx drmManagerClient = AddonVideoDrmUtils.getDrmManagerClient();

        boolean isDRMFile = false;
        try {
            /**SPRD:Bug474646 Add Drm feature @{ @{ */
        //    isDRMFile = drmManagerClient.canHandle(filePath, mimeType);
            if (drmManagerClient.canHandle(filePath, mimeType)
                    && drmManagerClient.getMetadata(filePath) != null) {
                isDRMFile=true;
            }
            /**@}*/
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "filePath is null or empty string.");
            return false;
        } catch (IllegalStateException ex) {
            Log.w(TAG, "DrmManagerClient didn't initialize properly.");
            return false;
        }
        return isDRMFile;
    }

    public static DrmManagerClientEx getDrmManagerClient() {
        return AddonVideoDrmUtils.getDrmManagerClient();
    }

    public static boolean isDrmValid(String filePath) {
        return DrmStore.RightsStatus.RIGHTS_VALID == getDrmRightsStatus(filePath);
    }

    public static int getDrmRightsStatus(String filePath) {
        DrmManagerClientEx drmManagerClient = AddonVideoDrmUtils.getDrmManagerClient();
        return drmManagerClient.checkRightsStatus(filePath);
    }

    public static boolean isDrmSupportTransfer(String path) {
        DrmManagerClientEx drmManagerClient = AddonVideoDrmUtils.getDrmManagerClient();
        if (isDrmFile(path, null)) {
            return DrmStore.RightsStatus.RIGHTS_VALID
                == drmManagerClient.checkRightsStatus(path, DrmStore.Action.TRANSFER);
        } else {
            return true;
        }
    }

    /* add for bug 318505 start @{ */
    public static String getDrmFileType(String filePath) {
        String drmFileType = "";
        DrmManagerClientEx drmManagerClient = AddonVideoDrmUtils.getDrmManagerClient();
        try {
            ContentValues values = drmManagerClient.getMetadata(filePath);
            if (values != null) {
                drmFileType = values.getAsString(DRM_FILE_TYPE);
            }
        } catch (Exception e) {
            Log.e(TAG, "Get extended_data error");
        }
        return drmFileType;
    }
    /* @} add for bug 318505 end */

    public static String getFilePathByUri(Uri uri,Context context) {
        Log.d(TAG,"Uri="+uri.toString());
        String filePath = "";
        if (uri.getScheme().compareTo("content") == 0) {
            String[] projection = { MediaStore.Images.Media.DATA };

            CursorLoader loader = new CursorLoader(context, uri, projection, null, null, null);
            Cursor cursor = null;
            try {
                cursor = loader.loadInBackground();
                if(cursor!=null){
                    int column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                    cursor.moveToFirst();
                    filePath = cursor.getString(column_index);
                    //remove cursor close to finally for bug 450517
                    //cursor.close();
                }
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            } catch (NullPointerException e) {
                e.printStackTrace();
                // add for bug 309910
            } catch(CursorIndexOutOfBoundsException e) {
                e.printStackTrace();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            } finally {
                if(cursor!=null){
                    Log.d(TAG,"cursor close");
                    cursor.close();
                    cursor = null;
                }
            }

        } else if (uri.getScheme().compareTo("file") == 0) {
            filePath = uri.getPath();
        }
        Log.d(TAG,"path="+filePath);
        return filePath;
    }
}
