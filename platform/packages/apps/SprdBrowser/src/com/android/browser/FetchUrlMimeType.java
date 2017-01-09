/*
 * Copyright (C) 2008 The Android Open Source Project
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

import java.net.HttpURLConnection;
import java.net.URL;

import android.app.DownloadManager;
import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.webkit.MimeTypeMap;
import android.net.Uri;

import java.io.IOException;
import java.io.File;
import android.Manifest;
import android.content.pm.PackageManager;
import android.app.Activity;

import com.android.browser.util.URLUtil;
import com.sprd.common.Common;
import android.content.Intent;
import com.android.browser.util.Util;

/**
 * This class is used to pull down the http headers of a given URL so that
 * we can analyse the mimetype and make any correction needed before we give
 * the URL to the download manager.
 * This operation is needed when the user long-clicks on a link or image and
 * we don't know the mimetype. If the user just clicks on the link, we will
 * do the same steps of correcting the mimetype down in
 * android.os.webkit.LoadListener rather than handling it here.
 *
 */
class FetchUrlMimeType extends Thread {

    private final static String LOGTAG = "FetchUrlMimeType";

    /* modify for download permission
     * Orignal android code:private Context mContext;
     */
    private Activity mContext;
    private DownloadManager.Request mRequest;
    private String mUri;
    private String mCookies;
    private String mUserAgent;
    private Controller mController = null;

    /* modify for download permission
     * Orignal android code:
        public FetchUrlMimeType(Context context, DownloadManager.Request request,
                String uri, String cookies, String userAgent) {
            mContext = context.getApplicationContext();
     */
    public FetchUrlMimeType(Activity context, DownloadManager.Request request,
            String uri, String cookies, String userAgent) {
        mContext = context;
        mRequest = request;
        mUri = uri;
        mCookies = cookies;
        mUserAgent = userAgent;
    }

    /* SPRD: Modify for Bug:501454 @{ */
    public FetchUrlMimeType(Controller controller, Activity context, DownloadManager.Request request,
            String uri, String cookies, String userAgent) {
        mController = controller;
        mContext = context;
        mRequest = request;
        mUri = uri;
        mCookies = cookies;
        mUserAgent = userAgent;
    }
    /* @} */

    @Override
    public void run() {
        String mimeType = null;
        String contentDisposition = null;
        HttpURLConnection connection = null;
        try {
            URL url = new URL(mUri);
            connection = (HttpURLConnection) url.openConnection();
            /* SPRD:508408 rm the method to avoid connection response error@{ */
            //connection.setRequestMethod("HEAD");
            /*@}*/

            if (mUserAgent != null) {
                connection.addRequestProperty("User-Agent", mUserAgent);
            }

            if (mCookies != null && mCookies.length() > 0) {
                connection.addRequestProperty("Cookie", mCookies);
            }

            if (connection.getResponseCode() == 200) {
                mimeType = connection.getContentType();
                Log.i(LOGTAG,"mimeType = " + mimeType);
                if (mimeType != null) {
                    mimeType = Intent.normalizeMimeType(mimeType);
                }
                if (mimeType != null) {
                    final int colonIndex = mimeType.indexOf(',');
                    if (colonIndex != -1) {
                        mimeType = mimeType.substring(0, colonIndex);
                    }
                }
                if (mimeType != null) {
                    if (DownloadHandler.IsInvalidMimeType(mimeType)) {
                        String newMimeType =
                                MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                                        MimeTypeMap.getFileExtensionFromUrl(mUri));
                        if (newMimeType != null) {
                            mimeType = newMimeType;
                        }
                    }
                } else {//modify for 638347: mime is null
                    String newMimeType =
                            MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                                        MimeTypeMap.getFileExtensionFromUrl(mUri));
                    Log.d(LOGTAG,"get newMimeType== "+newMimeType);
                    if (newMimeType != null) {
                        mimeType = newMimeType;
                    }
                }
                Log.i(LOGTAG,"after normalize mimeType = " + mimeType);
                contentDisposition = connection.getHeaderField("Content-Disposition");
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG,"Download failed: " + ioe);
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (mimeType != null) {
            mRequest.setMimeType(mimeType);
            String filename = URLUtil.guessFileName(mUri, contentDisposition,
                    mimeType);
            /*
             * for download_storage_save_path
             *@{
             */
            /* SPRD: Modify for Bug:501454 @{ */
            if(Util.SUPPORT_SELECT_DOWNLOAD_PATH && mController != null){
                filename = DownloadHandler.replaceInvalidVfatCharacters(filename);
                Log.i(LOGTAG,"support select path,setDestination filename = " + filename);

                Uri pathUri = null;
                pathUri = Common.getBrowserDrmPlugIn().getDrmPath(mUri, mimeType, filename);
                if  (pathUri != null) {
                    Log.i(LOGTAG,"Drm Path = " + pathUri);
                    mRequest.setDestinationUri(pathUri);
                } else {
                    mRequest.setDestinationUri(Uri.fromFile(new File(mController.getDownloadPath()+filename)));
                }
            } else {
                Log.i(LOGTAG,"setDestination filename = " + filename);
                mRequest.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, filename);
            }
            /* @} */
            /*@}*/
        }

        //modify for download permission
        if (mContext.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            Log.i(LOGTAG, "no storage permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
            if (Controller.mPermissionObj == null) {
                Controller.mPermissionObj = mRequest;
                Log.i(LOGTAG, "requestPermissions storage permission");
                mContext.requestPermissions(
                        new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                        Controller.PERMISSIONS_REQUEST_STORAGE_READ_WRITE);
            }
        } else {
            // Start the download
            DownloadManager manager = (DownloadManager) mContext.getSystemService(
                    Context.DOWNLOAD_SERVICE);
            try {
                manager.enqueue(mRequest);
            } catch (Exception e) {
                Log.e(LOGTAG, "Exception in DownloadManage enqueue  "+e.toString());
            }
        }
    }

}
