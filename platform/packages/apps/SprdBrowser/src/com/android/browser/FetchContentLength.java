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
 * for download_storage_save_path
 */

package com.android.browser;

import android.webkit.CookieManager;
import android.app.Dialog;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.app.DownloadManager;
import android.content.Context;
import android.net.Proxy;
import android.os.Environment;
import android.util.Log;
import android.webkit.MimeTypeMap;
import android.webkit.URLUtil;
import java.net.HttpURLConnection;
import java.net.URL;
import java.io.IOException;

class FetchContentLength extends Thread {

    private final static String LOGTAG = "FetchContentLength";

    private Context mContext;
    private String mUri;
    //private String mCookies;
    private String mUserAgent;
    private Dialog mDialog;
    private Handler mHandler;

    public FetchContentLength(Context context, String uri, String userAgent, boolean privateBrowsing, Dialog dialog, Handler handler) {
        mContext = context.getApplicationContext();
        mUri = uri;
        //mCookies = CookieManager.getInstance().getCookie(uri, privateBrowsing);
        mUserAgent = userAgent;
        mDialog=dialog;
        mHandler = handler;
    }

    @Override
    public void run() {
        HttpURLConnection connection = null;
        int contentLength = -1;
        if (DataUri.isDataUri(mUri)) {
            try {
                DataUri uri = new DataUri(mUri);
                contentLength = uri.getData().length;
            } catch (IOException e) {
                Log.i(LOGTAG, "Could not save data URL");
            }
        } else {
            try {
                URL url = new URL(mUri);
                connection = (HttpURLConnection) url.openConnection();
                if (mUserAgent != null) {
                    connection.addRequestProperty("User-Agent", mUserAgent);
                }
                if (connection.getResponseCode() == 200) {
                    contentLength = connection.getContentLength();
                }
            } catch (IOException ignored) {
            } finally {
                if (connection != null) {
                    connection.disconnect();
                }
            }
        }
        Log.i (LOGTAG,"content length is "+contentLength);
        final String size=mContext.getResources().getString(R.string.file_size,convertToHumanReadableSize(contentLength));
        Message msg = Message.obtain(mHandler, Controller.SET_TITLE, size);
        mHandler.sendMessageDelayed(msg, 1000);
    }

    private final static int ONE_KB = 1024;
    private final static int ONE_MB = 1024*ONE_KB;
    private String unknowSize;
    private String convertToHumanReadableSize(long contentLength) {
        unknowSize = mContext.getResources().getString(R.string.unknown_file_size);
        if (contentLength==-1) {
            return unknowSize;
        }
        if (contentLength<ONE_KB) {
            return contentLength+"B";
        }
        if (contentLength>= ONE_KB && contentLength < ONE_MB) {
            return contentLength/ONE_KB+"KB";
        }
        if (contentLength>=ONE_MB) {
            return contentLength/ONE_MB+"MB";
        }
            return unknowSize;
    }
}
