/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.browser;

import android.app.DownloadManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.ContentResolver;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.provider.Downloads;
import android.content.ContentUris;
import android.util.Log;

/**
 * This {@link BroadcastReceiver} handles clicks to notifications that
 * downloads from the browser are in progress/complete.  Clicking on an
 * in-progress or failed download will open the download manager.  Clicking on
 * a complete, successful download will open the file.
 */
public class OpenDownloadReceiver extends BroadcastReceiver {
    private static Handler sAsyncHandler;
    static {
        HandlerThread thr = new HandlerThread("Open browser download async");
        thr.start();
        sAsyncHandler = new Handler(thr.getLooper());
    }
    @Override
    public void onReceive(final Context context, Intent intent) {
        String action = intent.getAction();

        /**
         * Add for oma download
         *@{
         */
        Log.i("OpenDownloadReceiver", "OpenDownloadReceiver--action==="+action);
        if(DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(action)){
            final Intent i  = intent;
            Runnable worker = new Runnable() {
                @Override
                public void run() {
                    omaDownloaded(context, i);
                }
            };
            sAsyncHandler.post(worker);
            return;
        }
        /*@}*/
        if (!DownloadManager.ACTION_NOTIFICATION_CLICKED.equals(action)) {
            openDownloadsPage(context);
            return;
        }
        long ids[] = intent.getLongArrayExtra(
                DownloadManager.EXTRA_NOTIFICATION_CLICK_DOWNLOAD_IDS);
        if (ids == null || ids.length == 0) {
            openDownloadsPage(context);
            return;
        }
        final long id = ids[0];
        final PendingResult result = goAsync();
        Runnable worker = new Runnable() {
            @Override
            public void run() {
                onReceiveAsync(context, id);
                result.finish();
            }
        };
        sAsyncHandler.post(worker);
    }

    private void onReceiveAsync(Context context, long id) {
        DownloadManager manager = (DownloadManager) context.getSystemService(
                Context.DOWNLOAD_SERVICE);
        Uri uri = manager.getUriForDownloadedFile(id);
        if (uri == null) {
            // Open the downloads page
            openDownloadsPage(context);
        } else {
            Intent launchIntent = new Intent(Intent.ACTION_VIEW);
            launchIntent.setDataAndType(uri, manager.getMimeTypeForDownloadedFile(id));
            launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(launchIntent);
            } catch (ActivityNotFoundException e) {
                openDownloadsPage(context);
            }
        }
    }

    /**
     * Open the Activity which shows a list of all downloads.
     * @param context
     */
    private void openDownloadsPage(Context context) {
        Intent pageView = new Intent(DownloadManager.ACTION_VIEW_DOWNLOADS);
        pageView.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(pageView);
    }

    /**
     * Add for oma download
     *@{
     */
    private void omaDownloaded(Context context,Intent intent){
        ContentResolver cr = context.getContentResolver();
        long id = intent.getLongExtra(DownloadManager.EXTRA_DOWNLOAD_ID, -1);
        DownloadManager manager = (DownloadManager) context.getSystemService(
                Context.DOWNLOAD_SERVICE);

        if(id == -1){
            Log.d("OpenDownloadReceiver","id == -1");
            return;
        }

        Cursor cursor = null;
        try {
            cursor = cr.query(ContentUris.withAppendedId(Downloads.Impl.CONTENT_URI, id),
                    new String[] { Downloads.Impl._DATA,
                    Downloads.Impl.COLUMN_MIME_TYPE, Downloads.Impl.COLUMN_STATUS, Downloads.Impl.COLUMN_DELETED },
                    null, null, null);
            if ((cursor != null) && (cursor.moveToFirst())) {
                String mdrm = "content://drm/";
                String filename = cursor.getString(0);
                String mimetype = cursor.getString(1);
                int status = cursor.getInt(2);
                int deleted = cursor.getInt(3);

                if (deleted == 1) {
                    Log.d("OpenDownloadReceiver","already deleted, ignore");
                    return;
                }

                if (status == Downloads.Impl.STATUS_INSUFFICIENT_SPACE_ERROR) {
                    Intent it2 = new Intent(
                            Controller.SPACE_INSUFFICIENT_ACTION);
                    it2.putExtra("ID", id);
                    context.sendBroadcast(it2);
                    manager.remove(id);
                }

                if(status == 413){
                    Intent intentState = new Intent(Controller.DOWNLOAD_FILE_TOOLONG);
                    intentState.putExtra("ID", id);
                    context.sendBroadcast(intentState);
                }
                if(status == 200){
                    if(mimetype == null || !mimetype.equals("application/vnd.oma.dd+xml")){
                      Intent intentsuccess = new Intent(Controller.DOWNLOAD_FILE_SUCCESS);
                      intentsuccess.putExtra("filename", filename);
                      Log.d("OpenDownloadReceiver","download filename:"+filename);
                      context.sendBroadcast(intentsuccess);
                  }
                }
                /*
                 * notification url (if the OMA file had been downloaded,it
                 * will send the notification to server.) or ""
                 */
                if (mimetype != null
                        && mimetype.equals("application/vnd.oma.dd+xml")
                        && status == Downloads.Impl.STATUS_SUCCESS
                        && filename != null) { /* judge the download file is DD( OMA file descriptor ) */
                    BrowserXmlParser xml = new BrowserXmlParser(filename); /* create a XML object for parsing the XML file */
                    if(xml.root == null){
                        String errordd = getErrorMsg(context,
                                906);
                        sendErrorReport(context,errordd);
                        manager.remove(id);
                        return;
                    }
                    StringBuffer sb = new StringBuffer(512);
                    String URI, Notify, name, vendor, descr, version;
                    String mim = null, contnetMimeType = null, nextURL = null;
                    String size;
                    try {
                        /*
                         * now parse this XML file, and read the node info
                         * which we need,such as object url
                         */
                        version = xml.getValue("DDVersion");
                        if(version != null && !"1.0".equals(version)){
                            String errorversion= getErrorMsg(context,
                                    951);
                            sendErrorReport(context,errorversion);
                            manager.remove(id);
                            return;
                        }
                        URI = xml.getValue("objectURI"); /* read object url */
                        Notify = xml.getValue("installNotifyURI"); /* read notified url */
                        // mim = xml.getValue("type"); /* read the file's
                        // type which we will download */
                        String tempMimeType = xml.getValue("type");
                        contnetMimeType = xml.getValueSecond("type");
                        if (tempMimeType != null &&(tempMimeType.equalsIgnoreCase("application/vnd.oma.drm.message")
                                || tempMimeType.equalsIgnoreCase("application/vnd.oma.drm.content"))) {
                            contnetMimeType = tempMimeType;
                            mim = contnetMimeType;
                        } else {
                            mim = tempMimeType;
                        }
                        if(mim != null && !mim.equalsIgnoreCase("application/vnd.oma.drm.message")
                                && !mim.equalsIgnoreCase("application/vnd.oma.drm.content")){
                            Intent mimeintent = new Intent(Intent.ACTION_VIEW);
                            mimeintent.setDataAndType(Uri.fromParts("file", "", null), mim);
                            ResolveInfo info = context.getPackageManager().resolveActivity(mimeintent,
                                    PackageManager.MATCH_DEFAULT_ONLY);
                            if(info == null){
                                String errorType = null;
                                if (mim.equalsIgnoreCase("error/error")){
                                    errorType = getErrorMsg(context, 905);
                                }else{
                                    errorType = getErrorMsg(context,Downloads.Impl.STATUS_NOT_ACCEPTABLE);
                                }
                                sendErrorReport(context, errorType);
                                manager.remove(id);
                                return;
                            }
                        }
                        name = xml.getValue("name"); /* read the name */
                        vendor = xml.getValue("vendor"); /* read the vendor */
                        descr = xml.getValue("description"); /*read the description*/
                        size = xml.getValue("size"); /* read the size */
                        if (name == null){
                            name = "";
                        }
                        if (vendor == null){
                            vendor = "";
                        }
                        if (descr == null){
                            descr = "";
                        }
                        if("0".equalsIgnoreCase(size) || size == null){
                            sendErrorReport(context, context.getString(R.string.oma_dd_description_size_zero));
                            manager.remove(id);
                            return;
                        }
                        nextURL = xml.getValue("nextURL");
                        xml = null; /* we needn't it, just release */
                        if (Notify != null) {
                            if (URI == null || mim == null) { /*Invalid descriptor */
                                String statusMsgs = getErrorMsg(context,906);
                                sendErrorReport(context, statusMsgs);
                                doOMANotify(statusMsgs, Notify);
                                manager.remove(id);
                                return;
                            }
                        }
                        /* make the description of the file for user */
                        sb.append(context.getString(R.string.oma_name) + name + "\n");
                        sb.append(context.getString(R.string.oma_type) + mim + "\n");
                        sb.append(context.getString(R.string.oma_size) + size + " B\n");
                        sb.append(context.getString(R.string.oma_vendor) + vendor + "\n");
                        sb.append(context.getString(R.string.oma_description) + descr + "\n");

                        /* send the intent for downloading the OMA file */
                        Intent it = new Intent(
                                Controller.OMA_DD_DOWNLOAD_ACTION);
                        it.putExtra("OMADD", sb.toString());
                        if(URI.contains(" ")){
                            URI = URI.replace(" ", "%20");
                        }
                        it.putExtra("uri", URI);
                        it.putExtra("meta_type", contnetMimeType);
                        it.putExtra("notify_uri", nextURL + ":-:" + Notify);
                        it.putExtra("name", name);
                        it.putExtra("nexturl", nextURL);

                        context.sendBroadcast(it);
                        sb = null;

                        manager.remove(id);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            } else {
              Log.d("OpenDownloadReceiver","no cursor data");
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }
    }

    /**
     * Notify the OMA download status result to the sever
     *
     * @param status
     *            the status code of the download
     * @param OMANotifyUri
     *            the URL of the server to report the result
     * @return void
     */
    private void doOMANotify(final String status, String OMANotifyUri) {
        try {

            /* use post method to send the status to the server. */
            final String urlString = OMANotifyUri.replaceAll("&amp;", "&"); /* &amp has no sense,so just delete it here.*/
            Runnable run = new Runnable() {
                @Override
                public void run() {
                    OutputStream out = null;
                    try {
                        URL url = new URL(urlString);
                        HttpURLConnection httpc = (HttpURLConnection) url.openConnection();
                        httpc.setDoOutput(true);
                        httpc.setRequestMethod("POST");
                        out = httpc.getOutputStream();
                        if (out != null) {
                            out.write(status.getBytes());
                            out.flush();
                        }
                        httpc.getResponseCode();
                    } catch (IOException e) {
                        e.printStackTrace();
                    } finally {
                        if (out != null) {
                            try {
                                out.close();
                            } catch (Exception e) {
                            }
                        }
                    }
                }
            };
            /* if ok, the http status is 200 */
            new Thread(run).start();

        } catch (Exception e) {
            Log.v("OpenDownloadReceiver","catch the exception in doOMANotify");
        }

    }

    /**
     *dd file error send intent ,open error dialog
     *@param context
     *@param msg
     *          error message
     */
    private void sendErrorReport(Context context, String msg) {
        Intent errorIntent = new Intent(
                Controller.OMA_DD_DOWNLOAD_RESULT_ACTION);
        errorIntent.putExtra("ERRORMAG", msg);
        context.sendBroadcast(errorIntent);
    }

    /**
     * Get the detail message from the status code
     *
     * @param status
     *            the status code of the download
     * @return String the detail message
     */
    private String getErrorMsg(Context context, int status) {
        StringBuffer suc = new StringBuffer(32); /* the status message */
        switch (status) {
        case Downloads.Impl.STATUS_SUCCESS:
            suc.append(context.getString(R.string.oma_download_success));
            break;
        case Downloads.Impl.STATUS_INSUFFICIENT_SPACE_ERROR:
            suc.append(context
                    .getString(R.string.oma_download_Insufficient_memory));
            break;
        case 413:
                suc.append(context
                        .getString(R.string.oma_download_File_toobig));
            break;
        case Downloads.Impl.STATUS_CANCELED: /* user canceled */
            suc.append(context.getString(R.string.oma_download_user_cancelled));
            break;
        case 903:
            suc.append(context.getString(R.string.oma_download_loss_service));
            break;
        case 905:
            suc.append(context
                    .getString(R.string.oma_download_attribute_mismatch));
            break;
        case 906:
            suc.append(context
                    .getString(R.string.oma_download_invalid_descripter));
            break;
        case 951:
            suc.append(context
                    .getString(R.string.oma_download_invalid_ddversion));
            break;
        case 404:
            suc.append(context
                    .getString(R.string.oma_download_file_not_found));
            break;
        case Downloads.Impl.STATUS_DEVICE_NOT_FOUND_ERROR:
            suc.append(context.getString(R.string.oma_download_device_aborted));
            break;
        case Downloads.Impl.STATUS_NOT_ACCEPTABLE:
            suc.append(context
                    .getString(R.string.oma_download_non_acceptablecontent));
            break;
        case 954:
        default:
            suc.append(context.getString(R.string.oma_download_default_error));
            break;
        }
        return suc.toString();
    }
    /*@}*/
}
