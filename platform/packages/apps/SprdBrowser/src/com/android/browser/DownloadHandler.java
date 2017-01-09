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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.DownloadManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.net.WebAddress;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.CookieManager;
import android.widget.Toast;
import android.content.DialogInterface;
import java.io.File;
import com.sprd.common.Common;
import android.Manifest;
import android.webkit.MimeTypeMap;

import com.android.browser.util.URLUtil;

/**
 * Handle download requests
 */
public class DownloadHandler {

    private static final boolean LOGD_ENABLED =
            com.android.browser.Browser.LOGD_ENABLED;

    private static final String LOGTAG = "DLHandler";
    private static AlertDialog mAlertDialog;

    //check invalid mimetype from download server
    public static final String[] INVALID_MIMETYPE = {
        "text/plain",
        "application/octet-stream",
        "application/force-download"
    };

    /**
     * Notify the host application a download should be done, or that
     * the data should be streamed if a streaming viewer is available.
     * @param activity Activity requesting the download.
     * @param url The full url to the content that should be downloaded
     * @param userAgent User agent of the downloading application.
     * @param contentDisposition Content-disposition http header, if present.
     * @param mimetype The mimetype of the content reported by the server
     * @param referer The referer associated with the downloaded url
     * @param privateBrowsing If the request is coming from a private browsing tab.
     */
    public static void onDownloadStart(Activity activity, String url,
            String userAgent, String contentDisposition, String mimetype,
            String referer, boolean privateBrowsing) {
        Log.i(LOGTAG, "onDownloadStart--mimeType=="+mimetype+", url=="+url);

        if (mimetype != null){
            if (IsInvalidMimeType(mimetype)) {
                String modifyMimeType =
                        MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                            MimeTypeMap.getFileExtensionFromUrl(url));
                Log.i(LOGTAG, "modifyMimeType  =  "+modifyMimeType);
                if (modifyMimeType != null) {
                    mimetype = modifyMimeType;
                }
            }
        }

        // if we're dealing wih A/V content that's not explicitly marked
        //     for download, check if it's streamable.
        if (contentDisposition == null
                || !contentDisposition.regionMatches(
                        true, 0, "attachment", 0, 10)) {
            // query the package manager to see if there's a registered handler
            //     that matches.
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.parse(url), mimetype);
            ResolveInfo info = activity.getPackageManager().resolveActivity(intent,
                    PackageManager.MATCH_DEFAULT_ONLY);
            if (info != null) {
                ComponentName myName = activity.getComponentName();
                // If we resolved to ourselves, we don't want to attempt to
                // load the url only to try and download it again.
                if (!myName.getPackageName().equals(
                        info.activityInfo.packageName)
                        || !myName.getClassName().equals(
                                info.activityInfo.name)) {
                    // someone (other than us) knows how to handle this mime
                    // type with this scheme, don't download.
                    try {
                        activity.startActivity(intent);
                        return;
                    } catch (ActivityNotFoundException ex) {
                        if (LOGD_ENABLED) {
                            Log.d(LOGTAG, "activity not found for " + mimetype
                                    + " over " + Uri.parse(url).getScheme(),
                                    ex);
                        }
                        // Best behavior is to fall back to a download in this
                        // case
                    }
                }
            }
        }
        onDownloadStartNoStream(activity, url, userAgent, contentDisposition,
                mimetype, referer, privateBrowsing);
    }

    // This is to work around the fact that java.net.URI throws Exceptions
    // instead of just encoding URL's properly
    // Helper method for onDownloadStartNoStream
    private static String encodePath(String path) {
        char[] chars = path.toCharArray();

        boolean needed = false;
        for (char c : chars) {
            if (c == '[' || c == ']' || c == '|') {
                needed = true;
                break;
            }
        }
        if (needed == false) {
            return path;
        }

        StringBuilder sb = new StringBuilder("");
        for (char c : chars) {
            if (c == '[' || c == ']' || c == '|') {
                sb.append('%');
                sb.append(Integer.toHexString(c));
            } else {
                sb.append(c);
            }
        }

        return sb.toString();
    }

    /**
     * Notify the host application a download should be done, even if there
     * is a streaming viewer available for thise type.
     * @param activity Activity requesting the download.
     * @param url The full url to the content that should be downloaded
     * @param userAgent User agent of the downloading application.
     * @param contentDisposition Content-disposition http header, if present.
     * @param mimetype The mimetype of the content reported by the server
     * @param referer The referer associated with the downloaded url
     * @param privateBrowsing If the request is coming from a private browsing tab.
     */
    /*package */ static void onDownloadStartNoStream(Activity activity,
            String url, String userAgent, String contentDisposition,
            String mimetype, String referer, boolean privateBrowsing) {

        String filename = URLUtil.guessFileName(url,
                contentDisposition, mimetype);

        // Check to see if we have an SDCard
        String status = Environment.getExternalStorageState();
        if (!status.equals(Environment.MEDIA_MOUNTED)) {
            int title;
            String msg;

            // Check to see if the SDCard is busy, same as the music app
            if (status.equals(Environment.MEDIA_SHARED)) {
                msg = activity.getString(R.string.download_sdcard_busy_dlg_msg);
                title = R.string.download_sdcard_busy_dlg_title;
            } else {
                msg = activity.getString(R.string.download_no_sdcard_dlg_msg, filename);
                title = R.string.download_no_sdcard_dlg_title;
            }

            new AlertDialog.Builder(activity)
                .setTitle(title)
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setMessage(msg)
                .setPositiveButton(R.string.ok, null)
                .show();
            return;
        }

        // java.net.URI is a lot stricter than KURL so we have to encode some
        // extra characters. Fix for b 2538060 and b 1634719
        WebAddress webAddress;
        try {
            webAddress = new WebAddress(url);
            webAddress.setPath(encodePath(webAddress.getPath()));
        } catch (Exception e) {
            // This only happens for very bad urls, we want to chatch the
            // exception here
            Log.e(LOGTAG, "Exception trying to parse url:" + url);
            return;
        }

        String addressString = webAddress.toString();
        Uri uri = Uri.parse(addressString);
        final DownloadManager.Request request;
        try {
            request = new DownloadManager.Request(uri);
        } catch (IllegalArgumentException e) {
            Toast.makeText(activity, R.string.cannot_download, Toast.LENGTH_SHORT).show();
            return;
        }
        request.setMimeType(mimetype);
        // set downloaded file destination to /sdcard/Download.
        // or, should it be set to one of several Environment.DIRECTORY* dirs depending on mimetype?
        try {
            request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, filename);
        } catch (IllegalStateException ex) {
            // This only happens when directory Downloads can't be created or it isn't a directory
            // this is most commonly due to temporary problems with sdcard so show appropriate string
            Log.w(LOGTAG, "Exception trying to create Download dir:", ex);
            Toast.makeText(activity, R.string.download_sdcard_busy_dlg_title,
                    Toast.LENGTH_SHORT).show();
            return;
        }
        // let this downloaded file be scanned by MediaScanner - so that it can
        // show up in Gallery app, for example.
        request.allowScanningByMediaScanner();
        request.setDescription(webAddress.getHost());
        // XXX: Have to use the old url since the cookies were stored using the
        // old percent-encoded url.
        String cookies = CookieManager.getInstance().getCookie(url, privateBrowsing);
        request.addRequestHeader("cookie", cookies);
        request.addRequestHeader("User-Agent", userAgent);
        request.addRequestHeader("Referer", referer);
        request.setNotificationVisibility(
                DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        if (mimetype == null) {
            if (TextUtils.isEmpty(addressString)) {
                return;
            }
            // We must have long pressed on a link or image to download it. We
            // are not sure of the mimetype in this case, so do a head request
            new FetchUrlMimeType(activity, request, addressString, cookies,
                    userAgent).start();
        } else {
            //modify for download permission
            if (activity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                Log.i(LOGTAG, "no storage permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
                if (Controller.mPermissionObj == null) {
                    Controller.mPermissionObj = request;
                    Log.i(LOGTAG, "requestPermissions storage permission");
                    activity.requestPermissions(
                            new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                            Controller.PERMISSIONS_REQUEST_STORAGE_READ_WRITE);
                    return;
                }
            } else {
                final DownloadManager manager
                        = (DownloadManager) activity.getSystemService(Context.DOWNLOAD_SERVICE);
                new Thread("Browser download") {
                    public void run() {
                        try {
                            manager.enqueue(request);
                        } catch (Exception e) {
                            Log.e("DownloadHandler", "Exception in DownloadManage enqueue  "+e.toString());
                        }
                    }
                }.start();
            }
        }
        Toast.makeText(activity, R.string.download_pending, Toast.LENGTH_SHORT)
                .show();
    }

    /*
     * for download_storage_save_path
     *@{
     */

    public static void onDownloadStart(Controller controller,Activity activity, String url,
        String userAgent, String contentDisposition, String mimetype, boolean privateBrowsing, final boolean closeOnExit, boolean directDownload) {
        Log.i(LOGTAG, "DownloadHandler->onDownloadStart--mimeType=="+mimetype+", url=="+url);

        if(!StorageUtils.isExternalStorageMounted() && !StorageUtils.isInternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()){
            Log.i(LOGTAG, "onDownloadStart, no available storage");
            Toast.makeText(activity, R.string.no_storage, Toast.LENGTH_SHORT).show();
            return;
        }

        if (mimetype != null){
            if (IsInvalidMimeType(mimetype)) {
                String modifyMimeType =
                        MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                            MimeTypeMap.getFileExtensionFromUrl(url));
                Log.i(LOGTAG, "modifyMimeType  =  "+modifyMimeType);
                if (modifyMimeType != null) {
                    mimetype = modifyMimeType;
                /* Add for Bug:474368 Get minetype from  filename extension 2015.09.15 start */
                } else {
                    String fileName = URLUtil.guessFileName(url, contentDisposition, mimetype);
                    int dotPos = fileName.lastIndexOf('.');
                    if(0 <= dotPos) {
                        String newMimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(fileName.substring(dotPos + 1));
                        if(newMimeType != null) {
                            mimetype = newMimeType;
                        }
                    }
                    Log.i("DownloadHandler","filename="+fileName+"---mimetype ="+mimetype);
                /* Add for Bug:474368 Get minetype from  filename extension 2015.09.15 end */
                }
            }
        }

        /*if direct download from menu, do not start activity or show dialog*/
        if (DataUri.isDataUri(url) || directDownload) {
            controller.onDownloadStartSavePath(url, userAgent, contentDisposition, mimetype, privateBrowsing,closeOnExit);
            return;
        }

        if (contentDisposition == null || !contentDisposition.regionMatches(true, 0, "attachment", 0, 10)) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.parse(url), mimetype);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            ResolveInfo info = activity.getPackageManager().resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
            if (info != null) {
                ComponentName myName = activity.getComponentName();
                if (!myName.getPackageName().equals(info.activityInfo.packageName) || !myName.getClassName().equals(info.activityInfo.name)) {
                    final Intent myintent = intent;
                    final String mytype = mimetype;
                    final String myurl = url;
                    final String myuserAgent = userAgent;
                    final String mycontentDisposition = contentDisposition;
                    final boolean myprivateBrowsing = privateBrowsing;
                    final Controller mycontroller = controller;
                    final Activity myActivity = activity;
                    mAlertDialog = new AlertDialog.Builder(activity)
                    .setTitle(R.string.select_open_download)
                    .setMessage(R.string.open_download)
                    .setPositiveButton(R.string.mopen,
                        new DialogInterface.OnClickListener(){
                            public void onClick(DialogInterface dialog, int which) {
                                try {
                                    myActivity.startActivity(myintent);
                                    return;
                                } catch (ActivityNotFoundException ex) {
                                    Log.d(LOGTAG, "activity not found for "+ mytype + " over "+ Uri.parse(myurl).getScheme(), ex);
                                }finally{
                                    if(closeOnExit){
                                        mycontroller.goBackOnePageOrQuit();
                                    }
                                }
                            }
                        })
                    .setNeutralButton(R.string.mdownload,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mycontroller.onDownloadStartSavePath(myurl, myuserAgent,mycontentDisposition, mytype, myprivateBrowsing,closeOnExit);
                            }
                        }).show();
                    return;
                }
            }else{
                PackageManager pm = activity.getPackageManager();
                intent.setDataAndType(Uri.fromParts("file", "", null), mimetype);
                ResolveInfo ri = pm.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY);
                final String myurl = url;
                final String myuserAgent = userAgent;
                final String mycontentDisposition = contentDisposition;
                final String mytype = mimetype;
                final boolean myprivateBrowsing = privateBrowsing;
                final Controller mycontroller = controller;
                if(ri == null){
                    mAlertDialog = new AlertDialog.Builder(activity)
                    .setTitle(R.string.type_no_support)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setMessage(R.string.type_message)
                    .setPositiveButton(R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                if(closeOnExit){
                                    mycontroller.goBackOnePageOrQuit();
                                }
                            }
                        })
                    .setNegativeButton(R.string.ok_filename,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mycontroller.onDownloadStartSavePath(myurl, myuserAgent,mycontentDisposition, mytype, myprivateBrowsing, closeOnExit);
                            }
                        }).show();
                    return;
                }
            }
        }
        controller.onDownloadStartSavePath(url, userAgent, contentDisposition, mimetype, privateBrowsing,closeOnExit);
    }

    static void onDownloadStartNoStream(Controller controller,Activity activity,
        String url, String userAgent, String contentDisposition,
        String mimetype, boolean privateBrowsing, boolean closeOnExit) {

        if(!StorageUtils.isExternalStorageMounted() && !StorageUtils.isInternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()){
            Log.i(LOGTAG, "onDownloadStartNoStream, no available storage");
            Toast.makeText(activity, R.string.no_storage, Toast.LENGTH_SHORT).show();
            return;
        }

        WebAddress webAddress;
        try {
            webAddress = new WebAddress(url);
            webAddress.setPath(encodePath(webAddress.getPath()));
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception trying to parse url:" + url);
            return;
        }
        String addressString = webAddress.toString();
        String filename = URLUtil.guessFileName(url, contentDisposition, mimetype);
        Log.i(LOGTAG, "DownloadHandler onDownloadStartNostreamController--filename="+filename+", mimetype="+mimetype);
        filename = replaceInvalidVfatCharacters(filename);
        Log.i(LOGTAG, "DownloadHandler onDownloadStartNostreamController--filename=="+filename);

/*
        String status = null;
        String path = null;
        status = Environment.getExternalStorageState();
        path = Environment.getExternalStorageDirectory().getPath();
        if ((null == status) || (path == null) || (controller.getDownloadPath() == null)){
            return;
        }

        if (!status.equals(Environment.MEDIA_MOUNTED) && controller.getDownloadPath().startsWith(path)) {
            int title;
            String msg;
            if (status.equals(Environment.MEDIA_SHARED)) {
                msg = activity.getString(R.string.download_sdcard_busy_dlg_msg);
                title = R.string.download_sdcard_busy_dlg_title;
            } else {
                msg = activity.getString(R.string.download_no_sdcard_dlg_msg, filename);
                title = R.string.download_no_sdcard_dlg_title;
            }
            new AlertDialog.Builder(activity)
                .setTitle(title)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setMessage(msg)
                .setPositiveButton(R.string.ok, null)
                .show();
            return;
        }
*/
        Uri uri = Uri.parse(addressString);
        final DownloadManager.Request request;
        try {
            request = new DownloadManager.Request(uri);
        } catch (IllegalArgumentException e) {
            Toast.makeText(activity, R.string.cannot_download, Toast.LENGTH_SHORT).show();
            return;
        }
        request.setMimeType(mimetype);
        request.allowScanningByMediaScanner();
        request.setDescription(webAddress.getHost());
        String cookies = CookieManager.getInstance().getCookie(url, privateBrowsing);
        request.addRequestHeader("cookie", cookies);
        request.addRequestHeader("User-Agent", userAgent);
        request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
        Uri pathUri = null;
        pathUri = Common.getBrowserDrmPlugIn().getDrmPath(url, mimetype, filename);
        if  (pathUri != null) {
            request.setDestinationUri(pathUri);
        }else{
            request.setDestinationUri(Uri.fromFile(new File(controller.getDownloadPath()+filename)));
        }
        if (mimetype == null) {
            if (TextUtils.isEmpty(addressString)) {
                return;
            }
            /* SPRD: Modify for Bug:501454 @{ */
            new FetchUrlMimeType(controller, activity, request, addressString, cookies, userAgent).start();
            /*@}*/
        } else {
            //modify for download permission
            if (activity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                Log.i(LOGTAG, "no storage permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
                if (Controller.mPermissionObj == null) {
                    Controller.mPermissionObj = request;
                    Log.i(LOGTAG, "requestPermissions storage permission");
                    activity.requestPermissions(
                            new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                            Controller.PERMISSIONS_REQUEST_STORAGE_READ_WRITE);
                    if(closeOnExit){
                        controller.goBackOnePageOrQuit();
                    }
                    return;
                }
            } else {
                final DownloadManager manager = (DownloadManager) activity.getSystemService(Context.DOWNLOAD_SERVICE);
                new Thread("Browser download") {
                    public void run() {
                        try {
                            manager.enqueue(request);
                        } catch (Exception e) {
                            Log.e("DownloadHandler", "Exception in DownloadManage enqueue  "+e.toString());
                        }
                    }
                }.start();
            }
        }
        Toast.makeText(activity, R.string.download_pending, Toast.LENGTH_SHORT).show();
        if(closeOnExit){
            controller.goBackOnePageOrQuit();
        }
    }

    //check invalid mimetype from download server
    public static boolean IsInvalidMimeType(String mimetype) {
        boolean invalid_mime = false;
        for (String mime : DownloadHandler.INVALID_MIMETYPE) {
            if (mime.equalsIgnoreCase(mimetype)) {
                invalid_mime = true;
                break;
            }
        }
        return invalid_mime;
    }

    public static String replaceInvalidVfatCharacters(String filename) {
        final char START_CTRLCODE = 0x00;
        final char END_CTRLCODE = 0x1f;
        final char QUOTEDBL = 0x22;
        final char ASTERISK = 0x2A;
        final char SLASH = 0x2F;
        final char COLON = 0x3A;
        final char LESS = 0x3C;
        final char GREATER = 0x3E;
        final char QUESTION = 0x3F;
        final char BACKSLASH = 0x5C;
        final char BAR = 0x7C;
        final char DEL = 0x7F;
        final char UNDERSCORE = 0x5F;

        StringBuffer sb = new StringBuffer();
        char ch;
        boolean isRepetition = false;
        for (int i = 0; i < filename.length(); i++) {
            ch = filename.charAt(i);
            if ((START_CTRLCODE <= ch &&
                ch <= END_CTRLCODE) ||
                ch == QUOTEDBL ||
                ch == ASTERISK ||
                ch == SLASH ||
                ch == COLON ||
                ch == LESS ||
                ch == GREATER ||
                ch == QUESTION ||
                ch == BACKSLASH ||
                ch == BAR ||
                ch == DEL){
                if (!isRepetition) {
                    sb.append(UNDERSCORE);
                    isRepetition = true;
                }
            } else {
                sb.append(ch);
                isRepetition = false;
            }
        }
        return sb.toString();
    }
    /*@}*/

}
