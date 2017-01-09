/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.browser.sitenavigation;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ParseException;
import android.net.Uri;
import android.net.WebAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.webkit.WebChromeClient;

import com.android.browser.R;
import com.android.browser.UrlUtils;
import android.util.Log;
import com.android.browser.util.Util;
import android.text.InputFilter;

import java.io.ByteArrayOutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;

/**
 *  This class provides UI and methods that allow user to
 *  add new website or edit existing website.
 */
public class SiteNavigationAddDialog extends Activity {

    private static final String LOGTAG = "browser/AddSiteNavigationPage";

    private EditText    mName;
    private EditText    mAddress;
    private Button      mButtonOK;
    private Button      mButtonCancel;
    private Bundle      mMap;
    // The original url that is editting
    private String      mItemUrl;
    private String      mItemName;
    private boolean     mIsAdding;
    private TextView    mDialogText;

    // Message IDs
    private static final int SAVE_SITE_NAVIGATION = 100;

    private Handler mHandler;

    private View.OnClickListener mOKListener = new View.OnClickListener() {
        public void onClick(View v) {
            if (save()) {
                SiteNavigationAddDialog.this.setResult(Activity.RESULT_OK, (new Intent()).putExtra("need_refresh", true));
                finish();
            }
        }
    };

    private View.OnClickListener mCancelListener = new View.OnClickListener() {
        public void onClick(View v) {
            finish();
        }
    };

    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.site_navigation_add);

        String name = null;
        String url = null;

        mMap = getIntent().getExtras();
        Log.d(LOGTAG, "onCreate mMap is : " + mMap);
        if (mMap != null) {
            Bundle b = mMap.getBundle("websites");
            if (b != null) {
                mMap = b;
            }
            name = mMap.getString("name");
            url = mMap.getString("url");
            mIsAdding = mMap.getBoolean("isAdding");
        }

        //The original url that is editting
        mItemUrl = url;
        mItemName = name;

        mName = (EditText) findViewById(R.id.title);
        mName.setText(name);
        InputFilter[] nameFilter = Util.getLengthFilter(this,50);
        mName.setFilters(nameFilter);

        mAddress = (EditText) findViewById(R.id.address);
        if (url.equals(SiteNavigation.BROWSER_NAVIGATION)) {
            mAddress.setHint("http://");
        } else {
            mAddress.setText(url);
        }
        int max_length = getResources().getInteger(R.integer.max_url_length);
        InputFilter[] addressFilter = Util.getLengthFilter(this,max_length);
        mAddress.setFilters(addressFilter);

        mDialogText = (TextView) findViewById(R.id.dialog_title);
        if (mIsAdding) {
            mDialogText.setText(R.string.add_favorite);
        }

        mButtonOK = (Button) findViewById(R.id.OK);
        mButtonOK.setOnClickListener(mOKListener);

        mButtonCancel = (Button) findViewById(R.id.cancel);
        mButtonCancel.setOnClickListener(mCancelListener);

        if (!getWindow().getDecorView().isInTouchMode()) {
            mButtonOK.requestFocus();
        }
    }

    /**
     * Runnable to save a website, so it can be performed in its own thread.
     */
   private class SaveSiteNavigationRunnable implements Runnable {
        private Message mMessage;
        private Context mContext;
        public SaveSiteNavigationRunnable(Context ctx, Message msg) {
            mContext = ctx;
            mMessage = msg;
        }
        public void run() {
            // Unbundle website data.
            Bundle bundle = mMessage.getData();
            String title = bundle.getString("title");
            String url = bundle.getString("url");
            String itemUrl = bundle.getString("itemUrl");
            Boolean isAdding = bundle.getBoolean("isAdding");
            Boolean needUpdateThumbnail = bundle.getBoolean("needUpdateThumbnail");
            // Save to the site navigation DB.
            ContentResolver cr = mContext.getContentResolver();
            Cursor cursor = null;

            ContentValues values = new ContentValues();
            values.put(SiteNavigation.TITLE, title);
            values.put(SiteNavigation.URL, url);
            values.put(SiteNavigation.DATE_CREATED, 0 + "");
            values.put(SiteNavigation.WEBSITE, 1 + "");
            if (needUpdateThumbnail) {
                ByteArrayOutputStream os = new ByteArrayOutputStream();
                Bitmap bm = BitmapFactory.decodeResource(mContext.getResources(),
                        R.raw.sitenavigation_thumbnail_default );
                bm.compress(Bitmap.CompressFormat.PNG, 100, os);
                values.put(SiteNavigation.THUMBNAIL, os.toByteArray());
            }

            if (!isAdding) {
                try {
                    cursor = cr.query(SiteNavigation.SITE_NAVIGATION_URI,
                            new String[] {SiteNavigation.ID}, "url = ?", new String[] {itemUrl}, null);
                    if (cursor != null && cursor.moveToFirst()) {
                        Uri uri = ContentUris.withAppendedId(SiteNavigation.SITE_NAVIGATION_URI, cursor.getLong(0));
                        Log.d(LOGTAG, "SaveSiteNavigationRunnable uri is : " + uri);
                        cr.update(uri, values, null, null);
                    } else {
                        Log.e(LOGTAG, "saveSiteNavigationItem the item does not exist!");
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "saveSiteNavigationItem", e);
                } finally {
                    if (null != cursor) {
                        cursor.close();
                    }
                }
            } else {
                cr.insert(SiteNavigation.SITE_NAVIGATION_URI, values);
            }
        }
    }

    /**
     * Parse the data entered in the dialog and post a message to update the Site Navigation database.
     */
    boolean save() {

        String name = mName.getText().toString().trim();
        name = name.replaceAll("\r|\n", " ").trim();
        String unfilteredUrl = UrlUtils.fixUrl(mAddress.getText().toString());
        boolean emptyTitle = name.length() == 0;
        boolean emptyUrl = unfilteredUrl.trim().length() == 0;
        Resources r = getResources();
        if (emptyTitle || emptyUrl) {
            if (emptyTitle) {
                mName.setError(r.getText(R.string.favorite_needs_title));
            }
            if (emptyUrl) {
                mAddress.setError(r.getText(R.string.favorite_needs_url));
            }
            return false;
        }

        if (!name.equals(mItemName) && isSiteNavigationTitle(this, name)) {
            mName.setError(r.getText(R.string.duplicate_site_navigation_title));
            return false;
        }
        String url = unfilteredUrl.trim();
        try {
            // We allow website with a javascript: scheme, but these will in most cases
            // fail URI parsing, so don't try it if that's the kind of bookmark we have.

            if (!url.toLowerCase().startsWith("javascript:")) {
                URI uriObj = new URI(url);
                String scheme = uriObj.getScheme();
                if (!urlHasAcceptableScheme(url)) {
                    // If the scheme was non-null, let the user know that we
                    // can't save their website. If it was null, we'll assume
                    // they meant http when we parse it in the WebAddress class.
                    if (scheme != null) {
                        mAddress.setError(r.getText(R.string.favorite_cannot_save_url));
                        return false;
                    }
                    WebAddress address;
                    try {
                        address = new WebAddress(unfilteredUrl);
                    } catch (ParseException e) {
                        throw new URISyntaxException("", "");
                    }
                    if (address.getHost().length() == 0) {
                        throw new URISyntaxException("", "");
                    }
                    url = address.toString();
                } else {
                    String mark = "://";
                    int iRet = -1;
                    if (null != url) {
                        iRet = url.indexOf(mark);
                    }
                    if (iRet > 0 && url.indexOf("/", iRet + mark.length()) < 0) {
                        url = url + "/";
                        Log.d(LOGTAG, "URL=" + url);
                    }
                }

                try {
                    byte[] bytes = url.getBytes("UTF-8");
                    if (url.length() != bytes.length) {
                        throw new URISyntaxException("", "");
                    }
                } catch (UnsupportedEncodingException e) {
                    throw new URISyntaxException("", "");
                }
            }
        } catch (URISyntaxException e) {
            mAddress.setError(r.getText(R.string.bookmark_url_not_valid));
            return false;
        }

        // When it is adding, avoid duplicate url that already existing in the database
        if (!mItemUrl.equals(url)) {
            boolean exist = isSiteNavigationUrl(this, url);
            if (exist) {
                mAddress.setError(r.getText(R.string.duplicate_site_navigation_url));
                return false;
            }
        }

        // Post a message to write to the DB.
        Bundle bundle = new Bundle();
        bundle.putString("title", name);
        bundle.putString("url", url);
        bundle.putString("itemUrl", mItemUrl);
        bundle.putBoolean("isAdding", mIsAdding);
        if (!mItemUrl.equals(url)) {
            bundle.putBoolean("needUpdateThumbnail", true);
        } else {
            bundle.putBoolean("needUpdateThumbnail", false);
        }
        Message msg = Message.obtain(mHandler, SAVE_SITE_NAVIGATION);
        msg.setData(bundle);
        // Start a new thread so as to not slow down the UI
        Thread t = new Thread(new SaveSiteNavigationRunnable(SiteNavigationAddDialog.this, msg));
        t.start();

        return true;
    }

    public static boolean isSiteNavigationUrl(Context context, String itemUrl) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = cr.query(SiteNavigation.SITE_NAVIGATION_URI,
                    new String[] {SiteNavigation.TITLE}, "url = ?", new String[] {itemUrl}, null);
            if (null != cursor && cursor.moveToFirst()) {
                Log.d(LOGTAG, "isSiteNavigationUrl will return true.");
                return true;
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "isSiteNavigationUrl", e);
        } finally {
            if (null != cursor) {
                cursor.close();
            }
        }

        return false;
    }

    public static boolean isSiteNavigationTitle(Context context, String itemTitle) {
        ContentResolver cr = context.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = cr.query(SiteNavigation.SITE_NAVIGATION_URI,
                    new String[] {SiteNavigation.TITLE}, "title = ?", new String[] {itemTitle}, null);
            if (null != cursor && cursor.moveToFirst()) {
                Log.d(LOGTAG, "isSiteNavigationTitle will return true.");
                return true;
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "isSiteNavigationTitle", e);
        } finally {
            if (null != cursor) {
                cursor.close();
            }
        }
        return false;
    }

    private static final String ACCEPTABLE_WEBSITE_SCHEMES[] = {
        "http:",
        "https:",
        "about:",
        "data:",
        "javascript:",
        /*remove file schemes*/
        //"file:",
        "content:",
        "rtsp:"
    };
    private static boolean urlHasAcceptableScheme(String url) {
        if (url == null) {
            return false;
        }

        for (int i = 0; i < ACCEPTABLE_WEBSITE_SCHEMES.length; i++) {
            if (url.startsWith(ACCEPTABLE_WEBSITE_SCHEMES[i])) {
                return true;
            }
        }
        return false;
    }
}
