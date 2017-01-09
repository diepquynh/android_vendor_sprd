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
import android.app.SearchManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.nfc.NfcAdapter;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.Browser;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.util.Log;
import android.util.Patterns;

import com.android.browser.UI.ComboViews;
import com.android.browser.search.SearchEngine;
import com.android.browser.util.Util;
import com.android.common.Search;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Locale;
import java.util.Map;
import com.android.browser.util.Util;
import com.android.browser.search.OpenSearchSearchEngine;

/**
 * Handle all browser related intents
 */
public class IntentHandler {

    private static final String TAG = "IntentHandler";

    // "source" parameter for Google search suggested by the browser
    final static String GOOGLE_SEARCH_SOURCE_SUGGEST = "browser-suggest";
    // "source" parameter for Google search from unknown source
    final static String GOOGLE_SEARCH_SOURCE_UNKNOWN = "unknown";

    private final static String LOGTAG = "IntentHandler";

    /* package */ static final UrlData EMPTY_URL_DATA = new UrlData(null);

    private static final String[] SCHEME_WHITELIST = {
        "http",
        "https",
        "about",
    };

    private Activity mActivity;
    private Controller mController;
    private TabControl mTabControl;
    private BrowserSettings mSettings;

    public IntentHandler(Activity browser, Controller controller) {
        mActivity = browser;
        mController = controller;
        mTabControl = mController.getTabControl();
        mSettings = controller.getSettings();
    }

    /**
     *Add for wtai feature
     *Original Android code:
     * void onNewIntent(Intent intent) {
     *@{
     */
    void onNewIntent(Intent intent,UrlHandler handler) {
    /*@}*/
        Uri uri = intent.getData();
        if (uri != null && isForbiddenUri(uri)) {
            Log.e(TAG, "Aborting intent with forbidden uri, \"" + uri + "\"");
            return;
        }

        Tab current = mTabControl.getCurrentTab();
        // When a tab is closed on exit, the current tab index is set to -1.
        // Reset before proceed as Browser requires the current tab to be set.
        if (current == null) {
            // Try to reset the tab in case the index was incorrect.
            current = mTabControl.getTab(0);
            if (current == null) {
                // No tabs at all so just ignore this intent.
                return;
            }
            mController.setActiveTab(current);
        }
        final String action = intent.getAction();
        final int flags = intent.getFlags();
        if (Intent.ACTION_MAIN.equals(action) ||
                (flags & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) != 0) {
            // just resume the browser
            return;
        }
        if (BrowserActivity.ACTION_SHOW_BOOKMARKS.equals(action)) {
            mController.bookmarksOrHistoryPicker(ComboViews.Bookmarks);
            return;
        }

        // In case the SearchDialog is open.
        ((SearchManager) mActivity.getSystemService(Context.SEARCH_SERVICE))
                .stopSearch();
        if (Intent.ACTION_VIEW.equals(action)
                || NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)
                || Intent.ACTION_SEARCH.equals(action)
                || MediaStore.INTENT_ACTION_MEDIA_SEARCH.equals(action)
                || Intent.ACTION_WEB_SEARCH.equals(action)) {
            // If this was a search request (e.g. search query directly typed into the address bar),
            // pass it on to the default web search provider.
            /**
             *Modify for wtai feature
             *Original Android code:
             * if (handleWebSearchIntent(mActivity, mController, intent)) {
             *@{
             */
            if (handleWebSearchIntent(mActivity, mController, intent,handler)) {
            /*@}*/
                return;
            }

            UrlData urlData = getUrlDataFromIntent(intent);
            if (urlData.isEmpty()) {
                urlData = new UrlData(mSettings.getHomePage());
            }
            /**
             * Add for rtsp feature
             *@{
            */
            if (urlData.mUrl.startsWith("rtsp:") || urlData.mUrl.startsWith("tel:")){
                Intent rtspIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(urlData.mUrl));
                rtspIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                mActivity.startActivity(rtspIntent);
                return;
            }
            /*@}*/

            if (intent.getBooleanExtra(Browser.EXTRA_CREATE_NEW_TAB, false)
                  || urlData.isPreloaded()) {
                //Tab t = mController.openTab(urlData);
                mController.openTab(urlData);
                return;
            }
            /*
             * If the URL is already opened, switch to that tab
             * phone: Reuse tab with same appId
             * tablet: Open new tab
             */
            final String appId = intent
                    .getStringExtra(Browser.EXTRA_APPLICATION_ID);
            if (Intent.ACTION_VIEW.equals(action)
                    && (appId != null)
                    && appId.startsWith(mActivity.getPackageName())) {
                Tab appTab = mTabControl.getTabFromAppId(appId);
                if ((appTab != null) && (appTab == mController.getCurrentTab())) {
                    mController.switchToTab(appTab);
                    mController.loadUrlDataIn(appTab, urlData);
                    return;
                }
            }
            if (Intent.ACTION_VIEW.equals(action)
                     && !mActivity.getPackageName().equals(appId)) {
                if (!BrowserActivity.isTablet(mActivity)
                        && !mSettings.allowAppTabs()) {
                    Tab appTab = mTabControl.getTabFromAppId(appId);
                    if (appTab != null) {
                        mController.reuseTab(appTab, urlData);
                        return;
                    }
                }
                // No matching application tab, try to find a regular tab
                // with a matching url.
                Tab appTab = mTabControl.findTabWithUrl(urlData.mUrl);
                if (appTab != null) {
                    // Transfer ownership
                    appTab.setAppId(appId);
                    if (current != appTab) {
                        mController.switchToTab(appTab);
                    }
                    // Otherwise, we are already viewing the correct tab.
                } else {
                    // if FLAG_ACTIVITY_BROUGHT_TO_FRONT flag is on, the url
                    // will be opened in a new tab unless we have reached
                    // MAX_TABS. Then the url will be opened in the current
                    // tab. If a new tab is created, it will have "true" for
                    // exit on close.
                    Tab tab = mController.openTab(urlData);
                    if (tab != null) {
                        tab.setAppId(appId);
                        if ((intent.getFlags() & Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT) != 0) {
                            tab.setCloseOnBack(true);
                        }
                    }
                }
            } else {
                // Get rid of the subwindow if it exists
                mController.dismissSubWindow(current);
                // If the current Tab is being used as an application tab,
                // remove the association, since the new Intent means that it is
                // no longer associated with that application.
                current.setAppId(null);
                mController.loadUrlDataIn(current, urlData);
            }
        }
    }

    protected static UrlData getUrlDataFromIntent(Intent intent) {
        String url = "";
        Map<String, String> headers = null;
        PreloadedTabControl preloaded = null;
        String preloadedSearchBoxQuery = null;
        if (intent != null
                && (intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) == 0) {
            final String action = intent.getAction();
            if (Intent.ACTION_VIEW.equals(action) ||
                    NfcAdapter.ACTION_NDEF_DISCOVERED.equals(action)) {
                url = UrlUtils.smartUrlFilter(intent.getData());
                if (url != null && url.startsWith("http")) {
                    final Bundle pairs = intent
                            .getBundleExtra(Browser.EXTRA_HEADERS);
                    if (pairs != null && !pairs.isEmpty()) {
                        Iterator<String> iter = pairs.keySet().iterator();
                        headers = new HashMap<String, String>();
                        while (iter.hasNext()) {
                            String key = iter.next();
                            headers.put(key, pairs.getString(key));
                        }
                    }
                }
                if (intent.hasExtra(PreloadRequestReceiver.EXTRA_PRELOAD_ID)) {
                    String id = intent.getStringExtra(PreloadRequestReceiver.EXTRA_PRELOAD_ID);
                    preloadedSearchBoxQuery = intent.getStringExtra(
                            PreloadRequestReceiver.EXTRA_SEARCHBOX_SETQUERY);
                    preloaded = Preloader.getInstance().getPreloadedTab(id);
                }
            } else if (Intent.ACTION_SEARCH.equals(action)
                    || MediaStore.INTENT_ACTION_MEDIA_SEARCH.equals(action)
                    || Intent.ACTION_WEB_SEARCH.equals(action)) {
                url = intent.getStringExtra(SearchManager.QUERY);
                if (url != null) {
                    // In general, we shouldn't modify URL from Intent.
                    // But currently, we get the user-typed URL from search box as well.
                    url = UrlUtils.fixUrl(url);
                    url = UrlUtils.smartUrlFilter(url);
                    String searchSource = "&source=android-" + GOOGLE_SEARCH_SOURCE_SUGGEST + "&";
                    if (url.contains(searchSource)) {
                        String source = null;
                        final Bundle appData = intent.getBundleExtra(SearchManager.APP_DATA);
                        if (appData != null) {
                            source = appData.getString(Search.SOURCE);
                        }
                        if (TextUtils.isEmpty(source)) {
                            source = GOOGLE_SEARCH_SOURCE_UNKNOWN;
                        }
                        url = url.replace(searchSource, "&source=android-"+source+"&");
                    }
                }
            }
        }
        return new UrlData(url, headers, intent, preloaded, preloadedSearchBoxQuery);
    }

    /**
     * Launches the default web search activity with the query parameters if the given intent's data
     * are identified as plain search terms and not URLs/shortcuts.
     * @return true if the intent was handled and web search activity was launched, false if not.
     */
    /**
     *Modify for wtai feature
     *Original Android code:
     * static boolean handleWebSearchIntent(Activity activity,
     *     Controller controller, Intent intent) {
     *@{
     */
    static boolean handleWebSearchIntent(Activity activity,
            Controller controller, Intent intent,UrlHandler handler) {
    /*@}*/
        Log.d(LOGTAG, "IntentHandler -> handleWebSearchIntent intent = " + intent);
        if (intent == null) return false;

        String url = null;
        final String action = intent.getAction();
        if (Intent.ACTION_VIEW.equals(action)) {
            Uri data = intent.getData();
            if (data != null) url = data.toString();
        } else if (Intent.ACTION_SEARCH.equals(action)
                || MediaStore.INTENT_ACTION_MEDIA_SEARCH.equals(action)
                || Intent.ACTION_WEB_SEARCH.equals(action)) {
            url = intent.getStringExtra(SearchManager.QUERY);
        }
        Log.d(LOGTAG, "IntentHandler -> handleWebSearchIntent url = " + url);
        if (url!=null) {
            if (url.startsWith("mailto:")||url.startsWith("tel:")) {
                Intent specialIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                specialIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                activity.startActivity(specialIntent);
                return true;
            }
        }

        /**
         * Add for wtai feature
         *@{
         */
        if (url != null && handler != null && url.startsWith(UrlHandler.SCHEME_WTAI)) {
            if (handler.parseWtai(url)) {
                return true;
            }
        }
        /*@}*/

        return handleWebSearchRequest(activity, controller, url,
                intent.getBundleExtra(SearchManager.APP_DATA),
                intent.getStringExtra(SearchManager.EXTRA_DATA_KEY));
    }

    /**
     * Launches the default web search activity with the query parameters if the given url string
     * was identified as plain search terms and not URL/shortcut.
     * @return true if the request was handled and web search activity was launched, false if not.
     */
    private static boolean handleWebSearchRequest(Activity activity,
            Controller controller, String inUrl, Bundle appData,
            String extraData) {
        Log.v(LOGTAG, "IntentHandler -> handleWebSearchRequest inUrl = " + inUrl);
        if (inUrl == null) return false;

        // In general, we shouldn't modify URL from Intent.
        // But currently, we get the user-typed URL from search box as well.
        String url = UrlUtils.fixUrl(inUrl).trim();
        if(url == null) return false;

        // URLs are handled by the regular flow of control, so
        // return early.
        if (Util.isMatches(Util.WEB_URL.matcher(url))
                || Util.isMatches(UrlUtils.ACCEPTED_URI_SCHEMA.matcher(url))) {
            return false;
        }

        final ContentResolver cr = activity.getContentResolver();
        final String newUrl = url;
        if(!TextUtils.isEmpty(newUrl)){
            if (controller == null || controller.getTabControl() == null
                    || controller.getTabControl().getCurrentWebView() == null
                    || !controller.getTabControl().getCurrentWebView()
                    .isPrivateBrowsingEnabled()) {
                new AsyncTask<Void, Void, Void>() {
                    @Override
                    protected Void doInBackground(Void... unused) {
                        Util.addSearchUrl(cr, newUrl);
                        return null;
                    }
                }.execute();
            }
        }

        SearchEngine searchEngine = BrowserSettings.getInstance().getSearchEngine();
        if (searchEngine == null) return false;
        searchEngine.startSearch(activity, url, appData, extraData);
        /*do not finish the activity when use open search engine*/
        /*Bug537894: Return false,or it will search twice when search from internal with OpenSearchSearchEngine*/
        if (searchEngine instanceof OpenSearchSearchEngine && controller == null) {
            return false;
        }

        return true;
    }

    private static boolean isForbiddenUri(Uri uri) {
        String scheme = uri.getScheme();
        // Allow URIs with no scheme
        if (scheme == null) {
            return false;
        }

        scheme = scheme.toLowerCase(Locale.US);
        for (String allowed : SCHEME_WHITELIST) {
            if (allowed.equals(scheme)) {
                return false;
            }
        }
        return true;
    }

    /**
     * A UrlData class to abstract how the content will be sent to WebView.
     * This base class uses loadUrl to show the content.
     */
    static class UrlData {
        final String mUrl;
        final Map<String, String> mHeaders;
        final PreloadedTabControl mPreloadedTab;
        final String mSearchBoxQueryToSubmit;
        final boolean mDisableUrlOverride;

        UrlData(String url) {
            this.mUrl = url;
            this.mHeaders = null;
            this.mPreloadedTab = null;
            this.mSearchBoxQueryToSubmit = null;
            this.mDisableUrlOverride = false;
        }

        UrlData(String url, Map<String, String> headers, Intent intent) {
            this(url, headers, intent, null, null);
        }

        UrlData(String url, Map<String, String> headers, Intent intent,
                PreloadedTabControl preloaded, String searchBoxQueryToSubmit) {
            this.mUrl = url;
            this.mHeaders = headers;
            this.mPreloadedTab = preloaded;
            this.mSearchBoxQueryToSubmit = searchBoxQueryToSubmit;
            if (intent != null) {
                mDisableUrlOverride = intent.getBooleanExtra(
                        BrowserActivity.EXTRA_DISABLE_URL_OVERRIDE, false);
            } else {
                mDisableUrlOverride = false;
            }
        }

        boolean isEmpty() {
            return (mUrl == null || mUrl.length() == 0);
        }

        boolean isPreloaded() {
            return mPreloadedTab != null;
        }

        PreloadedTabControl getPreloadedTab() {
            return mPreloadedTab;
        }

        String getSearchBoxQueryToSubmit() {
            return mSearchBoxQueryToSubmit;
        }
    }

}
