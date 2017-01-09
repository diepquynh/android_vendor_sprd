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

import java.net.URISyntaxException;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.ParseException;
import android.net.Uri;
import android.net.WebAddress;
import android.os.Bundle;
import android.provider.BrowserContract;
import com.android.browser.provider.BrowserProvider2;
import android.text.TextUtils;
import android.util.Log;

public class HomepageSettingsReceiver extends BroadcastReceiver {
    private static final String TAG = "HomepageSettingsReceiver";

    private static final String OMAOTA_ACTION = "com.andorid.BrowserDataConfig";
    private static final int OMAOTA_ACTION_RESULTCODE = 4;

    private Context mContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        mContext = context;
        String action = intent.getAction();
        Log.d(TAG, "HomepageSettingsReceiver,action = " +action);

        if(OMAOTA_ACTION.equals(action)) {
            setResultCode(OMAOTA_ACTION_RESULTCODE);
            final Bundle bundle= intent.getExtras();
            if (bundle == null) {
                Log.d(TAG, "bundle = null");
                return;
            }

            String homepage = bundle.getString("startpage");
            Log.d(TAG, "homepage = "+homepage);
            if (homepage != null) {
                BrowserSettings.getInstance().setHomePage(UrlUtils.smartUrlFilter(homepage,false));
            }

            final int bookmarkSize = bundle.getInt("bookmark_size",0);
            if (bookmarkSize > 0) {
                Thread setThread = new Thread() {
                    @Override
                    public void run() {
                        for (int i=1; i<=bookmarkSize; i++) {
                            Log.d(TAG, "set bookmark i = "+i);
                            Bundle bundleBookmark = bundle.getBundle("bookmark" + i);
                            if (bundleBookmark != null) {
                                String uri = bundleBookmark.getString("uri");
                                String name = bundleBookmark.getString("name");
                                Log.d(TAG, "uri = "+uri+", name = "+name);
                                if (!TextUtils.isEmpty(uri) && !TextUtils.isEmpty(name)) {
                                    setBookmark(UrlUtils.smartUrlFilter(uri,false), name);
                                }
                            }
                        }
                    }
                };
                setThread.start();
           }
       }
    }

    private void setBookmark(String url, String title) {
        ContentValues values = new ContentValues();
        Cursor cursor_url = null;
        Cursor cursor_title = null;
        ContentResolver cr = mContext.getContentResolver();

        try {
            cursor_url = getUrlLike(cr, url);
            cursor_title = getTitleLike(cr, title);

            if (cursor_url != null && cursor_url.moveToFirst()) {
                String title_f = cursor_url.getString(cursor_url.getColumnIndex(BrowserContract.Bookmarks.TITLE));
                if (title_f == null || !title_f.equals(title)) {
                    values.put(BrowserContract.Bookmarks.TITLE, title);
                    cr.update(BrowserContract.Bookmarks.CONTENT_URI, values, "_id = " + cursor_url.getInt(0),
                            null);
                    Log.d(TAG, "update bookmarks, id=" +cursor_url.getInt(0));
                }
            } else if (cursor_title != null && cursor_title.moveToFirst()) {
                String url_f = cursor_title.getString(cursor_title.getColumnIndex(BrowserContract.Bookmarks.URL));
                if (url_f == null || !url_f.equals(url)) {
                    values.put(BrowserContract.Bookmarks.URL, url);
                    cr.update(BrowserContract.Bookmarks.CONTENT_URI, values, "_id = " + cursor_title.getInt(0),
                            null);
                    Log.d(TAG, "update bookmarks, id=" +cursor_title.getInt(0));
                }
            } else {
                Bookmarks.addBookmark(mContext,false,url,title,null,BrowserProvider2.FIXED_ID_ROOT);
                Log.d(TAG, "insert bookmarks");
            }
        } catch (Exception e) {
            Log.e(TAG, "add setting error", e);
        } finally {
            if (cursor_url != null)
                cursor_url.close();
            if (cursor_title != null)
                cursor_title.close();
        }
    }

    private static final Cursor getUrlLike(ContentResolver cr, String url) {
        String queryUrl = null;
        try {
            if (Bookmarks.urlHasAcceptableScheme(url)) {
                queryUrl = url;
            } else {
                WebAddress address;
                try {
                    address = new WebAddress(url);
                } catch (ParseException e) {
                    throw new URISyntaxException("", "");
                }
                if (address.getHost().length() == 0) {
                    throw new URISyntaxException("", "");
                }
                queryUrl = address.toString();
            }
        } catch (URISyntaxException  e) {
            Log.e(TAG,"Exception when WebAddress url :"+e);
            return null;
        }

        return cr.query(BrowserContract.Bookmarks.CONTENT_URI,
                new String[] { "_id","title","url" },
                "url = ? AND parent = 1 AND folder = 0", new String[] {queryUrl}, null);
    }

    private static final Cursor getTitleLike(ContentResolver cr, String title) {
        return cr.query(BrowserContract.Bookmarks.CONTENT_URI,
                new String[] { "_id","title","url" },
                "title = ? AND parent = 1 AND folder = 0", new String[] {title}, null);
    }

}
