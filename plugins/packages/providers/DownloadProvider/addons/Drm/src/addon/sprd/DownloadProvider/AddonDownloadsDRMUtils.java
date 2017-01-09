package addon.sprd.downloadprovider;

import java.io.File;

//import com.android.providers.downloadsplugin.Cursor;
import com.android.providers.downloads.DownloadThread.DownloadInfoDelta;
import com.android.providers.downloads.DownloadInfo;
//import com.android.providers.downloadsplugin.Intent;
import android.content.ContentUris;
import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.os.AsyncTask;
import com.android.providers.downloadsplugin.DownloadsDRMUtils;
import java.io.IOException;
import java.util.Random;
import java.util.Set;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import com.android.providers.downloads.DownloadThread;
import com.android.providers.downloads.Constants;
import android.provider.Downloads;
import com.android.providers.downloads.DownloadReceiver;
import static android.provider.Downloads.Impl.STATUS_BAD_REQUEST;
import static android.provider.Downloads.Impl.STATUS_CANNOT_RESUME;
import static android.provider.Downloads.Impl.STATUS_FILE_ERROR;
import static android.provider.Downloads.Impl.STATUS_HTTP_DATA_ERROR;
import static android.provider.Downloads.Impl.STATUS_SUCCESS;
import static android.provider.Downloads.Impl.STATUS_TOO_MANY_REDIRECTS;
import static android.provider.Downloads.Impl.STATUS_UNHANDLED_HTTP_CODE;
import static android.provider.Downloads.Impl.STATUS_UNKNOWN_ERROR;
import static android.provider.Downloads.Impl.STATUS_WAITING_FOR_NETWORK;
import static android.provider.Downloads.Impl.STATUS_WAITING_TO_RETRY;
import android.net.Uri;
import android.app.DownloadManager;
import android.content.ContentValues;
import android.text.TextUtils;
import android.content.ActivityNotFoundException;
import static android.app.DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED;
import static android.app.DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_ONLY_COMPLETION;
import android.app.DownloadManager.Request;
import android.app.NotificationManager;
import android.app.Notification;

public class AddonDownloadsDRMUtils extends DownloadsDRMUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "AddonDownloadsDRMUtils";
    public static final int DRM_PLUGIN = 0;
    public static final int HELPER_FULLNAME_RETURN = 1;
    public static final int HELPER_SUQENCE_CONTINUE = 2;
    private static final int SHOW_DOwnLOADS = 1;
    private static final int SHOW_CONSUME_DIALOG = 2;
    private static final int SHOW_OPEN_FILE = 3;
    private static final int SHOW_RENEW_DRM_FILE = 4;
    public static final String MIMETYPE_DRM_MESSAGE = "application/vnd.oma.drm.message";

    public AddonDownloadsDRMUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        Log.d("DownloaduiDRMUtils", "AddonDownloadsDRMUtils clazz: " + clazz);
        return clazz;
    }

    @Override
    public void deleteDatabase(Context context, Intent intent) {
        final Bundle bundle = intent.getExtras();
        final Context pluginContext = context;
        Log.d(TAG, "AddonDownloadsDRMUtils deleteDatabase --  ");
        new AsyncTask<Void, Void, String>() {
            @Override
            protected String doInBackground(Void... params) {
                // TODO Auto-generated method stub
                String path = bundle.getString("path");
                boolean renewDownload = bundle.getBoolean("renew_drm", false);
                String downloadUri = null;
                Cursor cursor = pluginContext.getContentResolver().query(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                        new String[]{Downloads.Impl._ID,Downloads.Impl.COLUMN_URI},
                        Downloads.Impl._DATA + "=?",
                        new String[] { path },
                        null);
                if(cursor != null){
                    if (cursor.moveToFirst()){
                        int idColumn = cursor.getColumnIndexOrThrow(Downloads.Impl._ID);
                        int idUri = cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_URI);
                        long id = cursor.getLong(idColumn);
                        downloadUri = cursor.getString(idUri);
                        Uri uri = ContentUris.withAppendedId(Downloads.Impl.ALL_DOWNLOADS_CONTENT_URI,
                                id);
                        Log.d(TAG, "AddonDownloadsDRMUtils DownloadReceiver delete file and database--id  "+id);
                        /* Fix Bug:522585 After downloading the dr file, the file cannot be opened 2016.01.08 */
                        ContentValues values = new ContentValues();
                        values.put(Downloads.Impl.COLUMN_DELETED, 1);
                        pluginContext.getContentResolver().update(uri, values, null, null);
                    }
                }

                if (cursor != null){
                    cursor.close();
                }
                if (renewDownload && downloadUri != null){
                    Log.d(TAG, "AddonDownloadsDRMUtils drm downloadUri -- "+downloadUri);
                    return downloadUri;
                }
                return null;
            }

            @Override
            protected void onPostExecute(String result) {
                // TODO Auto-generated method stub
                super.onPostExecute(result);
                if (result != null){
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.setDataAndType(Uri.parse(result), "text/html");
                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    pluginContext.startActivity(intent);
                }
            }
        }.execute();

    }

    @Override
    public boolean handleDRMNotificationBroadcast(Context context, Intent intent) {
        final String action = intent.getAction();

        Uri uri = intent.getData();
        Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);

        Log.d(TAG, "AddonDownloadsDRMUtils handleDRMNotificationBroadcast action= " + action);
        if (cursor == null) {
            Log.d(TAG, "AddonDownloadsDRMUtils handleDRMNotificationBroadcast cursor == null");
            return false;
        }
        try {
            if (!cursor.moveToFirst()) {
                Log.d(TAG, "AddonDownloadsDRMUtils handleDRMNotificationBroadcast !cursor.moveToFirst()");
                return false;
            }

            if (Constants.ACTION_LIST.equals(action)) {
                final long[] ids = intent.getLongArrayExtra(
                        DownloadManager.EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS);
                sendNotificationClickedIntent(context, intent, cursor);

            } else if (Constants.ACTION_OPEN.equals(action)) {
                openDRMDownload(context, cursor);
                hideNotification(context, uri, cursor);
            } else if (Constants.ACTION_HIDE.equals(action)) {
                hideNotification(context, uri, cursor);
            }

        } finally {
            cursor.close();
        }
        Log.d(TAG, "AddonDownloadsDRMUtils handleDRMNotificationBroadcast --action =   " + action);
        return true;
    }

    @Override
    public String getDRMDisplayName(Cursor cursor, String mimeType) {
        String displayName = null;

        Log.d(TAG, "AddonDownloadsDRMUtils getDRMDisplayName: getDRMDisplayName, mimeType = " + mimeType);

        if (mimeType != null) {
            if (DownloadDRMUltil.isDrmEnabled() && mimeType.equals(DownloadDrmHelper.DRM_CONTENT_TYPE)){
                Log.d(TAG, "AddonDownloadsDRMUtils isDRM ");
                displayName = cursor.getString(
                        cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_FILENAME));
            }else{
                displayName = cursor.getString(
                        cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_TITLE));
            }
        }
        Log.d(TAG, "AddonDownloadsDRMUtils displayName = " + displayName);
        return displayName;
    }

    @Override
    public boolean isSupportDRM() {
        Log.d(TAG, "isSupportDRM true");
        return true;
    }

    @Override
    public String getDRMFileName(String filename, String extension) {
        String fullFilename = filename + extension;
        Log.d(TAG, "AddonDownloadsDRMUtils getDRMFileName fullFilename" + fullFilename);
        if (extension != null && extension.endsWith(".dm")){
//            if (new File(filename+".dcf").exists()){
                fullFilename = filename + ".dcf";
//            }
        }
        Log.d(TAG, "AddonDownloadsDRMUtils getDRMFileName fullFilename" + fullFilename);
        return fullFilename;
    }

    @Override
    public boolean checkDRMFileName(String filename, int sequence, String extension) {
        String fullFilename = filename + Constants.FILENAME_SEQUENCE_SEPARATOR + sequence + extension;

        if (extension != null && extension.endsWith(".dm")){
            String drmName = filename + sequence + ".dcf";
            if (!new File(drmName).exists() && !new File(fullFilename).exists()) {
                return true;
            }
        }
        return false;
    }

    @Override
    public int getDRMSequence(String filename, String extension, int sequence) {
        Log.d(TAG, "AddonDownloadsDRMUtils getDRMSequence filename" + filename);
        if (extension != null && extension.endsWith(".dm")){
            String drmName = filename + sequence + ".dcf";
            if (!new File(drmName).exists() && !new File(filename).exists()) {
                return HELPER_FULLNAME_RETURN;
            }else{
                return HELPER_SUQENCE_CONTINUE;
            }
        }
        return DRM_PLUGIN;
    }

    //@Override
    public boolean isDrmConvertNeeded(String mimetype) {
        boolean isConvert = MIMETYPE_DRM_MESSAGE.equals(mimetype);
        Log.d(TAG, "AddonDownloadsDRMUtils isDrmConvertNeeded isConvert " + isConvert);
        return isConvert;
    }

    @Override
    public String modifyDrmFwLockFileExtension(String filename) {
        Log.d(TAG, "AddonDownloadsDRMUtils modifyDrmFwLockFileExtension filename =  " + filename);
        if (filename != null) {
            int extensionIndex;
            extensionIndex = filename.lastIndexOf(".");
            if (extensionIndex != -1) {
                filename = filename.substring(0, extensionIndex);
            }
            filename = filename.concat(DownloadDrmHelper.EXTENSION_INTERNAL_FWDL);
        }
        Log.d(TAG, "AddonDownloadsDRMUtils modifyDrmFwLockFileExtension DRMfilename =  " + filename);
        return filename;
    }

    @Override
    public String getDRMPluginMimeType(Context context, File file, String mimeType, Cursor cursor) {
        Log.d(TAG, "AddonDownloadsDRMUtils getDRMPluginMimeType ==  ");
        if ((mimeType != null) && (mimeType.equals("image/jpg"))){
            mimeType = "image/jpeg";
        }
        return DownloadDrmHelper.getOriginalMimeType(context, file.toString(), mimeType);
    }

    @Override
    public Intent setDRMPluginIntent(File file, String mimeType) {
        final Intent intent = new Intent(Intent.ACTION_VIEW);
        Log.d(TAG, "AddonDownloadsDRMUtils setDRMPluginIntent ==  ");
        intent.putExtra("drmpath", file.getPath());
        intent.putExtra("drmtype", mimeType);
        return intent;
        }

    @Override
    public void notifyDownloadCompleted(DownloadThread downloadThread, Context context, DownloadInfoDelta state, int finalStatus, String errorMsg, int numFailed) {
        //downloadThread.notifyThroughDatabase(state, finalStatus, errorMsg, numFailed);
        Log.d(TAG, "AddonDownloadsDRMUtils notifyDownloadCompleted ==  ");
        /* notifyThroughDatabase start */
        ContentValues values = new ContentValues();
        values.put(Downloads.Impl.COLUMN_STATUS, finalStatus);
        values.put(Downloads.Impl._DATA, state.mFileName);
        values.put(Downloads.Impl.COLUMN_MIME_TYPE, state.mMimeType);
        values.put(Downloads.Impl.COLUMN_LAST_MODIFICATION, System.currentTimeMillis());
        values.put(Downloads.Impl.COLUMN_FAILED_CONNECTIONS, numFailed);
        values.put(Constants.RETRY_AFTER_X_REDIRECT_COUNT, state.mRetryAfter);

        if (!TextUtils.equals(downloadThread.mInfo.mUri, state.mUri.toString())) {
            values.put(Downloads.Impl.COLUMN_URI, state.mUri.toString());
        }

        // save the error message. could be useful to developers.
        if (!TextUtils.isEmpty(errorMsg)) {
            values.put(Downloads.Impl.COLUMN_ERROR_MSG, errorMsg);
        }
        context.getContentResolver().update(downloadThread.mInfo.getAllDownloadsUri(), values, null, null);
        /* notifyThroughDatabase end */

        if (Downloads.Impl.isStatusCompleted(finalStatus)|| (downloadThread.mInfo.mDescription!=null && downloadThread.mInfo.mDescription.startsWith("OMA NOTIFICATION:"))
                || finalStatus == Downloads.Impl.STATUS_INSUFFICIENT_SPACE_ERROR) {
            Log.d(TAG, "AddonDownloadsDRMUtils DownloadSuccess  finalStatus-- "+finalStatus);
            final DownloadInfo mDRMInfo = downloadThread.mInfo;
            final Context drmContext = context;

            if (finalStatus == 200){
                if (DownloadDrmHelper.isDrmMimeType(context, state.mFileName, state.mMimeType)){
                    int installType = DownloadDrmHelper.getDrmObjectType(context, state.mFileName, state.mMimeType);
                    Log.d(TAG, "AddonDownloadsDRMUtils isntallType -- "+installType+" state.mFilename  "+ state.mFileName);
                    if (installType == android.drm.DrmStore.DrmObjectType.RIGHTS_OBJECT){
                        Log.d(TAG, "AddonDownloadsDRMUtils DownloadSuccess  isRights_object");
                        DownloadDrmHelper.saveDrmObjectRights(context, state.mFileName, state.mMimeType);
                        if (new File(state.mFileName).exists()){
                            boolean delete = new File(state.mFileName).delete();
                        }
                        Intent intent = new Intent("com.android.downloads.DELETE_DOWNLOAD_INFORMATION");
                        intent.putExtra("path", state.mFileName);
                        context.sendBroadcast(intent);
                    }else if (installType == android.drm.DrmStore.DrmObjectType.TRIGGER_OBJECT){
                        Log.d(TAG, "AddonDownloadsDRMUtils DownloadSuccess - --filename    "+state.mFileName + "  mStates --  "+finalStatus+"  mTitle --  ");
                        int index = state.mFileName.lastIndexOf("/");
                        String desDir = state.mFileName.substring(0, index);
                        Log.d(TAG, "AddonDownloadsDRMUtils desDir -- " + desDir);
                        DownloadDrmHelper.DrmClientOnErrorListener listener = new DownloadDrmHelper.DrmClientOnErrorListener() {
                            @Override
                            public void onError() {
                                Intent intentsuccess = new Intent("com.android.providers.DOWNLOAD_DRM_FAILED");
                                drmContext.sendBroadcast(intentsuccess);
                                Log.i(TAG, "AddonDownloadsDRMUtils DownloadThread sendBroadcast -> onError");
                            }

                            @Override
                            public void onSuccess() {
                                // TODO Auto-generated method stub
                                //Log.d(TAG, "AddonDownloadsDRMUtils onSuccess sendIntentIfRequest");
                                //mDRMInfo.sendIntentIfRequested();
                                Log.d(TAG,"AddonDownloadsDRMUtils DownloadThread DrmClientOnErrorListener -> onSuccess");
                            }

                        };
                        DownloadDrmHelper.setDrmClientOnErrorListener(listener);
                        DownloadDrmHelper.processConvertDrmInfo(context, state.mFileName, state.mMimeType, desDir);
                    }else{
                        Log.i(TAG, "AddonDownloadsDRMUtils DownloadSuccess  is else");
                    }
                }else{
                    Log.i(TAG, "AddonDownloadsDRMUtils notifyDownloadCompleted - no drm  "+state.mFileName);
                }
            }
            /* SPRD:506263 should not send intent here since it been sent in downloadThread @{ */
            /*if (DownloadDrmHelper.isDrmMimeType(context, state.mFileName, state.mMimeType)){
                if (downloadThread.mInfo.mFileName.endsWith(".dcf") || downloadThread.mInfo.mFileName.endsWith(".dr")){
                    downloadThread.mInfo.sendIntentIfRequested();
                }
             }else{
                 downloadThread.mInfo.sendIntentIfRequested();
             }*/
             /* }@ */
        }
    }

    private boolean openDRMDownload(Context context, Cursor cursor) {
        Log.d(TAG, "AddonDownloadsDRMUtils openDRMDownload " );
        String filename = cursor.getString(cursor.getColumnIndexOrThrow(Downloads.Impl._DATA));
        String mimetype =
            cursor.getString(cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_MIME_TYPE));
        Uri path = Uri.parse(filename);
        // If there is no scheme, then it must be a file
        if (path.getScheme() == null) {
            path = Uri.fromFile(new File(filename));
        }
        String originnalMimetype = DownloadDrmHelper.getOriginalMimeType(context, filename, mimetype);
        Intent activityIntent = new Intent(Intent.ACTION_VIEW);
        int status = DownloadDRMUltil.getRightsIntoDownloads(context, filename, mimetype);
        Log.i("AddonDownloadsDRMUtils", "DownloadReceiver  openDownload status  "+status);
        if (status == SHOW_DOwnLOADS){
            Intent downloadIntent = new Intent(DownloadManager.ACTION_NOTIFICATION_CLICKED);
            String pckg = cursor.getString(
                    cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_NOTIFICATION_PACKAGE));
            String clazz = cursor.getString(
                    cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_NOTIFICATION_CLASS));
            if (clazz != null) {
                downloadIntent.setClassName(pckg, clazz);
            }
            downloadIntent.setPackage(pckg);
            context.sendBroadcast(downloadIntent);
        }else if (status == SHOW_CONSUME_DIALOG){
            String mimeType = DownloadDrmHelper.DRM_CONTENT_TYPE;
            Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
            drm_Intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            drm_Intent.putExtra("filename", filename);
            drm_Intent.putExtra("mimetype", mimeType);
            drm_Intent.putExtra("isrenew", false);
            try {
                context.startActivity(drm_Intent);
            }catch (ActivityNotFoundException ex) {
                Log.i(TAG, "no activity for " + originnalMimetype, ex);
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 start*/
                Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_LONG).show();
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 end*/
            }
        }else if (status == SHOW_RENEW_DRM_FILE){
            String mimeType = DownloadDrmHelper.DRM_CONTENT_TYPE;
            Intent drm_Intent = new Intent("sprd.android.intent.action.VIEW_DOWNLOADS_DRM");
            drm_Intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            drm_Intent.putExtra("filename", filename);
            drm_Intent.putExtra("mimetype", mimeType);
            drm_Intent.putExtra("isrenew", true);
            try {
                context.startActivity(drm_Intent);
            }catch (ActivityNotFoundException ex) {
                Log.i(TAG, "no activity for " + originnalMimetype, ex);
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 start*/
                Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_LONG).show();
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 end*/
            }
        }else if (status == SHOW_OPEN_FILE){
            activityIntent.setDataAndType(path, originnalMimetype);
            activityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(activityIntent);
            } catch (ActivityNotFoundException ex) {
                Log.i(Constants.TAG, "no activity for " + originnalMimetype, ex);
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 start*/
                Toast.makeText(context, mAddonContext.getString(R.string.download_no_application_title), Toast.LENGTH_LONG).show();
                /* Add  for Bug:466427 Show toast when can't open 2015.09.02 end*/
            }
        }
        return true;
    }

    /**
     * Notify the owner of a running download that its notification was clicked.
     */
    private void sendNotificationClickedIntent(Context context, Intent intent, Cursor cursor) {
        String pckg = cursor.getString(
                cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_NOTIFICATION_PACKAGE));
        if (pckg == null) {
            return;
        }

        String clazz = cursor.getString(
                cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_NOTIFICATION_CLASS));
        boolean isPublicApi =
                cursor.getInt(cursor.getColumnIndex(Downloads.Impl.COLUMN_IS_PUBLIC_API)) != 0;

        Intent appIntent = null;
        if (isPublicApi) {
            appIntent = new Intent(DownloadManager.ACTION_NOTIFICATION_CLICKED);
            if (clazz != null) {
                appIntent.setClassName(pckg, clazz);
            }
            appIntent.setPackage(pckg);
            // send id of the items clicked on.
            if (intent.getBooleanExtra("multiple", false)) {
                // broadcast received saying click occurred on a notification with multiple titles.
                // don't include any ids at all - let the caller query all downloads belonging to it
                // TODO modify the broadcast to include ids of those multiple notifications.
            } else {
                appIntent.putExtra(DownloadManager.EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS,
                        new long[] {
                                cursor.getLong(cursor.getColumnIndexOrThrow(Downloads.Impl._ID))});
            }
        } else { // legacy behavior
            if (clazz == null) {
                return;
            }

            appIntent = new Intent(DownloadManager.ACTION_NOTIFICATION_CLICKED);
            appIntent.setClassName(pckg, clazz);
            if (intent.getBooleanExtra("multiple", true)) {
                appIntent.setData(Downloads.Impl.CONTENT_URI);
            } else {
                long downloadId = cursor.getLong(cursor.getColumnIndexOrThrow(Downloads.Impl._ID));
                appIntent.setData(
                        ContentUris.withAppendedId(Downloads.Impl.CONTENT_URI, downloadId));
            }
        }

        context.sendBroadcast(appIntent);
    }

    /**
     * Mark the given {@link DownloadManager#COLUMN_ID} as being acknowledged by
     * user so it's not renewed later.
     */
    private void hideNotification(Context context, Uri uri, Cursor cursor) {
        NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel((int) (ContentUris.parseId(uri)));
        Log.v("AddonDownloadsDRMUtils", "hideNotification cancelNotification id----"+ContentUris.parseId(uri));

        int statusColumn = cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_STATUS);
        int status = cursor.getInt(statusColumn);
        int visibilityColumn =
                cursor.getColumnIndexOrThrow(Downloads.Impl.COLUMN_VISIBILITY);
        int visibility = cursor.getInt(visibilityColumn);
        Log.v("AddonDownloadsDRMUtils", "hideNotification visibility  "+visibility);
        if (Downloads.Impl.isStatusCompleted(status)){
            if ((visibility == Downloads.Impl.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
                    || (visibility == Request.VISIBILITY_VISIBLE_NOTIFY_ONLY_COMPLETION)){
                Log.v("AddonDownloadsDRMUtils", "update uri  "+uri);
                ContentValues values = new ContentValues();
                values.put(Downloads.Impl.COLUMN_VISIBILITY,
                        Downloads.Impl.VISIBILITY_VISIBLE);
                context.getContentResolver().update(uri, values, null, null);
            }
        }
    }
//    @Override
//    public void setTextColor
}
