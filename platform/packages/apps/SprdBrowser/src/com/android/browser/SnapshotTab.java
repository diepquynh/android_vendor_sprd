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
 */
package com.android.browser;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.WebView;
import com.android.browser.provider.SnapshotProvider.Snapshots;
import java.io.ByteArrayInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Map;
import java.util.zip.GZIPInputStream;
import android.database.sqlite.SQLiteException;

public class SnapshotTab extends Tab {

    private static final String LOGTAG = "SnapshotTab";

    private long mSnapshotId;
    private LoadData mLoadTask;
    private WebViewFactory mWebViewFactory;
    private int mBackgroundColor;
    private long mDateCreated;

    public SnapshotTab(WebViewController wvcontroller, long snapshotId) {
        super(wvcontroller, null, null);
        mSnapshotId = snapshotId;
        mWebViewFactory = mWebViewController.getWebViewFactory();
        WebView web = mWebViewFactory.createWebView(false);
        setWebView(web);
        loadData();
    }

    @Override
    void putInForeground() {
        if (getWebView() == null) {
            WebView web = mWebViewFactory.createWebView(false);
            if (mBackgroundColor != 0) {
                web.setBackgroundColor(mBackgroundColor);
            }
            setWebView(web);
            loadData();
        }
        super.putInForeground();
    }

    @Override
    void putInBackground() {
        if (getWebView() == null) return;
        super.putInBackground();
    }

    void loadData() {
        if (mLoadTask == null) {
            mLoadTask = new LoadData(this, mContext);
            mLoadTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }

    @Override
    void addChildTab(Tab child) {
        if (!isSnapshot()) {
            super.addChildTab(child);
        } else {
            throw new IllegalStateException("Snapshot tabs cannot have child tabs!");
        }
    }

    @Override
    public boolean isSnapshot() {
        return super.getUrl().startsWith("file:");
    }

    public long getSnapshotId() {
        return mSnapshotId;
    }

    @Override
    public ContentValues createSnapshotValues() {
        if (!isSnapshot()) {
            return super.createSnapshotValues();
        }
        return null;
    }

    @Override
    public Bundle saveState() {
        /* SPRD: Modify for bug 566788 {@ */
        /*if (!isSnapshot()) {
            return super.saveState();
        }*/
        /* @} */
        return null;
    }

    public long getDateCreated() {
        return mDateCreated;
    }

    @Override
    public boolean canGoBack() {
        return super.canGoBack() || !isSnapshot();
    }

    @Override
    public boolean canGoForward() {
        return !isSnapshot() && super.canGoForward();
    }

    @Override
    public void goBack() {
        if (super.canGoBack()) {
            super.goBack();
        } else {
            getWebView().stopLoading();
            loadData();
        }
    }

    class LoadData extends AsyncTask<Void, Void, Cursor> {

        final String[] PROJECTION = new String[] {
            Snapshots._ID, // 0
            Snapshots.URL, // 1
            Snapshots.TITLE, // 2
            Snapshots.FAVICON, // 3
            Snapshots.VIEWSTATE, // 4
            Snapshots.BACKGROUND, // 5
            Snapshots.DATE_CREATED, // 6
            Snapshots.VIEWSTATE_PATH, // 7
            Snapshots.PATH,       //8
            Snapshots.ROOT_PATH,  //9
            Snapshots.WEBARCHIVE_PATH,
        };
        static final int SNAPSHOT_ID = 0;
        static final int SNAPSHOT_URL = 1;
        static final int SNAPSHOT_TITLE = 2;
        static final int SNAPSHOT_FAVICON = 3;
        static final int SNAPSHOT_VIEWSTATE = 4;
        static final int SNAPSHOT_BACKGROUND = 5;
        static final int SNAPSHOT_DATE_CREATED = 6;
        static final int SNAPSHOT_VIEWSTATE_PATH = 7;

        private SnapshotTab mTab;
        private ContentResolver mContentResolver;
        //private Context mContext;

        public LoadData(SnapshotTab t, Context context) {
            mTab = t;
            mContentResolver = context.getContentResolver();
            //mContext = context;
        }

        @Override
        protected Cursor doInBackground(Void... params) {
            Cursor cursor = null;
            long id = mTab.mSnapshotId;
            Uri uri = ContentUris.withAppendedId(Snapshots.CONTENT_URI, id);
            try {
                cursor =  mContentResolver.query(uri, PROJECTION, null, null, null);
            } catch (SQLiteException e) {
                Log.e(LOGTAG, "LoadData:", e);
            }
            return cursor;
        }
        @Override
        protected void onPostExecute(Cursor result) {
            if (result != null && result.moveToFirst()) {
                mCurSnapshotUri = result.getString(result.getColumnIndex(Snapshots.URL));
                Log.i(LOGTAG, "mCurSnapshotUri = " + mCurSnapshotUri);
                String webArchive = result.getString(result.getColumnIndex(Snapshots.WEBARCHIVE_PATH));
                mTab.mDateCreated = result.getLong(SNAPSHOT_DATE_CREATED);
                if(mTab.getWebView() != null){
                    mTab.getWebView().loadUrl("file://" + webArchive);
                }
            }
            if(result != null) {
                result.close();
            }
        }

    }

    @Override
    protected void persistThumbnail() {
        if (!isSnapshot()) {
            super.persistThumbnail();
        }
    }
    @Override
    String getUrl() {
        // TODO Auto-generated method stub
        if(isSnapshot()){
            return mCurSnapshotUri;
        }else{
            return super.getUrl();
        }
    }

    private String mCurSnapshotUri;

}
