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
import android.app.Dialog;
import android.app.DownloadManager;
import android.app.ProgressDialog;
import android.app.AlertDialog;
import android.content.ClipboardManager;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.content.BroadcastReceiver;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.net.Uri;
import android.net.http.SslError;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.preference.PreferenceActivity;
import android.provider.Browser;
import android.provider.BrowserContract;
import android.provider.BrowserContract.Images;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Intents.Insert;
import android.provider.Settings;
import android.speech.RecognizerIntent;
import android.text.TextUtils;
import android.util.Log;
import android.util.Patterns;
import android.view.ActionMode;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.webkit.CookieManager;
import android.webkit.CookieSyncManager;
import android.webkit.HttpAuthHandler;
import android.webkit.MimeTypeMap;
import android.webkit.SslErrorHandler;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebChromeClient.FileChooserParams;
import android.webkit.WebIconDatabase;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Toast;

import com.android.browser.IntentHandler.UrlData;
import com.android.browser.UI.ComboViews;
import com.android.browser.provider.BrowserProvider2.Thumbnails;
import com.android.browser.provider.SnapshotProvider.Snapshots;
import com.android.browser.util.Util;
import com.android.browser.util.URLUtil;

import com.android.browser.sitenavigation.SiteNavigation;
import com.android.browser.sitenavigation.SiteNavigationAddDialog;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URLEncoder;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import android.app.AlertDialog.Builder;
import android.app.AlertDialog;
import android.view.LayoutInflater;
import android.widget.Button;
import android.widget.EditText;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnDismissListener;
import android.view.View.OnClickListener;
import com.sprd.common.Common;
import android.content.ActivityNotFoundException;
import android.Manifest;
import android.webkit.PermissionRequest;
import com.android.browser.util.Util;

/**
 * Controller for browser
 */
public class Controller
        implements WebViewController, UiController, ActivityController {

    private static final String LOGTAG = "Controller";
    private static final String SEND_APP_ID_EXTRA =
        "android.speech.extras.SEND_APPLICATION_ID_EXTRA";
    private static final String INCOGNITO_URI = "browser:incognito";
    private static final String BLANK_URL = "about:blank";
    /*
     * for download_storage_save_path
     *@{
     */
    public final static String INTERNAL = "INTERNAL";
    public final static String SD = "SD";
    public final static String OTG = "OTG";
    private final static int DOWNLOAD_SELECTED = 7;
    private static final String IMAGE_BASE_FORMAT = "yyyy-MM-dd-HH-mm-ss-";
    private static final String FALLBACK_EXTENSION = "dat";


    private static String mDownloadPathStr = null;
    private AlertDialog mDownloadDialog = null;
    private AlertDialog mSelectStorageDialog = null;
    private EditText mDownloadPath;
    public static Boolean isSavePathEditorActivityStart = false;
    private String mDownloadSize = null;
    /*@}*/

    /*
     * for save_webpage
     *@{
     */
    private boolean isSavePage = false;
    /*@}*/

    /*
     * for download_overlay_same_file
     *@{
     */
    private static final int OVERLAY_DIALOG = 203;
    private ProgressDialog mFileProgressDialog;
    private AlertDialog mOverlayDialog;
    private ProgressDialog mFileDeleteProgressDialog;
    /*@}*/
    // public message ids
    public final static int LOAD_URL = 1001;
    public final static int STOP_LOAD = 1002;
    /*
     * for download_storage_save_path
     *@{
     */
    public final static int SET_TITLE = 1003;
    /*@}*/

    // Message Ids
    private static final int FOCUS_NODE_HREF = 102;
    private static final int RELEASE_WAKELOCK = 107;

    static final int UPDATE_BOOKMARK_THUMBNAIL = 108;

    private static final int OPEN_BOOKMARKS = 201;

    private static final int SHOW_SHAREPAGE = 301;

    private static final int EMPTY_MENU = -1;

    // activity requestCode
    final static int COMBO_VIEW = 1;
    final static int PREFERENCES_PAGE = 3;
    final static int FILE_SELECTED = 4;
    final static int VOICE_RESULT = 6;

    private final static int WAKELOCK_TIMEOUT = 5 * 60 * 1000; // 5 minutes

    private static final int MAX_BYTES = 180;

    // As the ids are dynamically created, we can't guarantee that they will
    // be in sequence, so this static array maps ids to a window number.
    final static private int[] WINDOW_SHORTCUT_ID_ARRAY =
    { R.id.window_one_menu_id, R.id.window_two_menu_id,
      R.id.window_three_menu_id, R.id.window_four_menu_id,
      R.id.window_five_menu_id, R.id.window_six_menu_id,
      R.id.window_seven_menu_id, R.id.window_eight_menu_id };

    // "source" parameter for Google search through search key
    final static String GOOGLE_SEARCH_SOURCE_SEARCHKEY = "browser-key";
    // "source" parameter for Google search through simplily type
    final static String GOOGLE_SEARCH_SOURCE_TYPE = "browser-type";

    // "no-crash-recovery" parameter in intent to suppress crash recovery
    final static String NO_CRASH_RECOVERY = "no-crash-recovery";

    // Only view images using these schemes
    private static final String[] IMAGE_VIEWABLE_SCHEMES = {
        "http",
        "https",
        "file",
        /*add data scheme for google photo*/
        "data"
    };

    // A bitmap that is re-used in createScreenshot as scratch space
    private static Bitmap sThumbnailBitmap;

    private Activity mActivity;
    private UI mUi;
    private TabControl mTabControl;
    private BrowserSettings mSettings;
    private WebViewFactory mFactory;

    private WakeLock mWakeLock;

    private UrlHandler mUrlHandler;
    private UploadHandler mUploadHandler;
    private IntentHandler mIntentHandler;
    private PageDialogsHandler mPageDialogsHandler;
    private NetworkStateHandler mNetworkHandler;

    private Message mAutoFillSetupMessage;

    private boolean mShouldShowErrorConsole;

    private SystemAllowGeolocationOrigins mSystemAllowGeolocationOrigins;

    // FIXME, temp address onPrepareMenu performance problem.
    // When we move everything out of view, we should rewrite this.
    private int mCurrentMenuState = 0;
    private int mMenuState = R.id.MAIN_MENU;
    private int mOldMenuState = EMPTY_MENU;
    private Menu mCachedMenu;

    private boolean mMenuIsDown;

    // For select and find, we keep track of the ActionMode so that
    // finish() can be called as desired.
    private ActionMode mActionMode;

    /**
     * Only meaningful when mOptionsMenuOpen is true.  This variable keeps track
     * of whether the configuration has changed.  The first onMenuOpened call
     * after a configuration change is simply a reopening of the same menu
     * (i.e. mIconView did not change).
     */
    private boolean mConfigChanged;

    /**
     * Keeps track of whether the options menu is open. This is important in
     * determining whether to show or hide the title bar overlay
     */
    private boolean mOptionsMenuOpen;

    /**
     * Whether or not the options menu is in its bigger, popup menu form. When
     * true, we want the title bar overlay to be gone. When false, we do not.
     * Only meaningful if mOptionsMenuOpen is true.
     */
    private boolean mExtendedMenuOpen;

    private boolean mActivityPaused = true;
    private boolean mLoadStopped;

    private Handler mHandler;
    // Checks to see when the bookmarks database has changed, and updates the
    // Tabs' notion of whether they represent bookmarked sites.
    private ContentObserver mBookmarksObserver;
    private CrashRecoveryHandler mCrashRecoveryHandler;

    private boolean mBlockEvents;

    private String mVoiceResult;

    /**
     * Add for navigation tab
     *@{
     */
    private ContentObserver mSiteNavigationObserver;
    private static final int FOCUS_NODE_HREF_NAVIGATION = 401;
    private final static int MAX_TITLE_LENGTH = 128;
    private ContextMenu mContextMenu;
    /*@}*/

    /**
     * Add for oma download
     *@{
     */
    public static final String OMA_DD_DOWNLOAD_ACTION = "com.android.browser.OMA_DOWNLOAD";
    public static final String OMA_DD_DOWNLOAD_RESULT_ACTION = "com.android.browser.OMA_DOWNLOAD_RESULT";
    public static final String SPACE_INSUFFICIENT_ACTION = "com.android.browser.SPACE_INSUFFICIENT";//2012.3.21 -Thundersoft fix bug 798
    public static final String DOWNLOAD_FILE_SUCCESS = "com.android.browser.DOWNLOAD_SUCCESS";
    public static final String DOWNLOAD_FILE_TOOLONG = "com.android.browser.FILE_TOOLONG";
    private BroadcastReceiver mDownloadSucessReceiver;
    /*@}*/

    //modify for permissions
    public static Object mPermissionObj = null;
    public static final int PERMISSIONS_REQUEST_STORAGE_READ_WRITE = 1;
    public static final int PERMISSIONS_REQUEST_OPENFILE = 2;
    public static final int PERMISSIONS_REQUEST_STORAGE_READ_WRITE_SELECT_PATH = 3;
    public static final int PERMISSIONS_REQUEST_CALL_PHONE = 4;
    public static final int PERMISSIONS_REQUEST_MEDIA = 5;

    public Controller(Activity browser) {
        mActivity = browser;
        mSettings = BrowserSettings.getInstance();
        mTabControl = new TabControl(this);
        mSettings.setController(this);
        mCrashRecoveryHandler = CrashRecoveryHandler.initialize(this);
        mCrashRecoveryHandler.preloadCrashState();
        mFactory = new BrowserWebViewFactory(browser);

        mUrlHandler = new UrlHandler(this);
        mIntentHandler = new IntentHandler(mActivity, this);
        mPageDialogsHandler = new PageDialogsHandler(mActivity, this);

        startHandler();
        mBookmarksObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                int size = mTabControl.getTabCount();
                for (int i = 0; i < size; i++) {
                    mTabControl.getTab(i).updateBookmarkedStatus();
                }
            }

        };
        browser.getContentResolver().registerContentObserver(
                BrowserContract.Bookmarks.CONTENT_URI, true, mBookmarksObserver);

        /**
         * Add for navigation tab
         *@{
         */
        mSiteNavigationObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                Log.d(LOGTAG, "SiteNavigation.SITE_NAVIGATION_URI changed");
                if (getCurrentTopWebView() != null &&
                        getCurrentTopWebView().getUrl() != null &&
                        getCurrentTopWebView().getUrl().equals(SiteNavigation.SITE_NAVIGATION)) {
                    Log.d(LOGTAG, "start reload");
                    getCurrentTopWebView().reload();
                }
            }
        };
        browser.getContentResolver().registerContentObserver(
                SiteNavigation.SITE_NAVIGATION_URI, true, mSiteNavigationObserver);
        /*@}*/


        mNetworkHandler = new NetworkStateHandler(mActivity, this);
        // Start watching the default geolocation permissions
        mSystemAllowGeolocationOrigins =
                new SystemAllowGeolocationOrigins(mActivity.getApplicationContext());
        mSystemAllowGeolocationOrigins.start();

        openIconDatabase();

        /**
         * Add for oma download
         *@{
         */
        IntentFilter fFilter = new IntentFilter();
        fFilter.addAction(Controller.OMA_DD_DOWNLOAD_ACTION);
        fFilter.addAction(Controller.OMA_DD_DOWNLOAD_RESULT_ACTION);
        fFilter.addAction(Controller.SPACE_INSUFFICIENT_ACTION);
        fFilter.addAction(DOWNLOAD_FILE_SUCCESS);
        fFilter.addAction(DOWNLOAD_FILE_TOOLONG);
        mDownloadSucessReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                onDownloadSucessReceiver(context, intent);
            }
        };
        mActivity.registerReceiver(mDownloadSucessReceiver, fFilter);
        /*@}*/
    }

    @Override
    public void start(final Intent intent) {
        Log.i(LOGTAG, "controller.start");
        // mCrashRecoverHandler has any previously saved state.
        mCrashRecoveryHandler.startRecovery(intent);
    }

    void doStart(final Bundle icicle, final Intent intent) {
        Log.i(LOGTAG, "controller.doStart");
        // Unless the last browser usage was within 24 hours, destroy any
        // remaining incognito tabs.

        Calendar lastActiveDate = icicle != null ?
                (Calendar) icicle.getSerializable("lastActiveDate") : null;
        Calendar today = Calendar.getInstance();
        Calendar yesterday = Calendar.getInstance();
        yesterday.add(Calendar.DATE, -1);

        final boolean restoreIncognitoTabs = !(lastActiveDate == null
            || lastActiveDate.before(yesterday)
            || lastActiveDate.after(today));

        // Find out if we will restore any state and remember the tab.
        final long currentTabId =
                mTabControl.canRestoreState(icicle, restoreIncognitoTabs);

        Log.i(LOGTAG, "controller.doStart-----before onPreloginFinished");
        if (currentTabId == -1) {
            Log.i(LOGTAG, "controller.doStart-----currentTabId == -1");
            // Not able to restore so we go ahead and clear session cookies.  We
            // must do this before trying to login the user as we don't want to
            // clear any session cookies set during login.
            CookieManager.getInstance().removeSessionCookie();
            Log.i(LOGTAG, "controller.doStart-----after CookieManager.removeSessionCookie");
        }

        GoogleAccountLogin.startLoginIfNeeded(mActivity,
                new Runnable() {
                    @Override public void run() {
                        onPreloginFinished(icicle, intent, currentTabId,
                                restoreIncognitoTabs);
                    }
                });
    }

    private void onPreloginFinished(Bundle icicle, Intent intent, long currentTabId,
            boolean restoreIncognitoTabs) {
        Log.i(LOGTAG, "controller.onPreloginFinished");
        if (currentTabId == -1) {
            BackgroundHandler.execute(new PruneThumbnails(mActivity, null));
            if (intent == null) {
                // This won't happen under common scenarios. The icicle is
                // not null, but there aren't any tabs to restore.
                openTabToHomePage();
            } else {
                final Bundle extra = intent.getExtras();
                // Create an initial tab.
                // If the intent is ACTION_VIEW and data is not null, the Browser is
                // invoked to view the content by another application. In this case,
                // the tab will be close when exit.
                UrlData urlData = IntentHandler.getUrlDataFromIntent(intent);
                Tab t = null;
                if (urlData.isEmpty()) {
                    t = openTabToHomePage();
                } else {
                    t = openTab(urlData);
                }
                if (t != null) {
                    t.setAppId(intent.getStringExtra(Browser.EXTRA_APPLICATION_ID));
                }
                WebView webView = t.getWebView();
                if (extra != null) {
                    int scale = extra.getInt(Browser.INITIAL_ZOOM_LEVEL, 0);
                    if (scale > 0 && scale <= 1000) {
                        webView.setInitialScale(scale);
                    }
                }
            }
            mUi.updateTabs(mTabControl.getTabs());
        } else {
            mTabControl.restoreState(icicle, currentTabId, restoreIncognitoTabs,
                    mUi.needsRestoreAllTabs());
            List<Tab> tabs = mTabControl.getTabs();
            ArrayList<Long> restoredTabs = new ArrayList<Long>(tabs.size());
            for (Tab t : tabs) {
                restoredTabs.add(t.getId());
            }
            BackgroundHandler.execute(new PruneThumbnails(mActivity, restoredTabs));
            if (tabs.size() == 0) {
                openTabToHomePage();
            }
            mUi.updateTabs(tabs);
            // TabControl.restoreState() will create a new tab even if
            // restoring the state fails.
            setActiveTab(mTabControl.getCurrentTab());
            // Intent is non-null when framework thinks the browser should be
            // launching with a new intent (icicle is null).
            if (intent != null) {
                /**
                 *Add for wtai feature
                 *Original Android code:
                 * mIntentHandler.onNewIntent(intent);
                 *@{
                 */
                mIntentHandler.onNewIntent(intent,mUrlHandler);
                /*@}*/
            }
        }
        // Read JavaScript flags if it exists.
        String jsFlags = getSettings().getJsEngineFlags();
        if (intent != null
                && BrowserActivity.ACTION_SHOW_BOOKMARKS.equals(intent.getAction())) {
            bookmarksOrHistoryPicker(ComboViews.Bookmarks);
        }
    }

    private static class PruneThumbnails implements Runnable {
        private Context mContext;
        private List<Long> mIds;

        PruneThumbnails(Context context, List<Long> preserveIds) {
            mContext = context.getApplicationContext();
            mIds = preserveIds;
        }

        @Override
        public void run() {
             try {
                ContentResolver cr = mContext.getContentResolver();
                if (mIds == null || mIds.size() == 0) {
                    cr.delete(Thumbnails.CONTENT_URI, null, null);
                } else {
                    int length = mIds.size();
                    StringBuilder where = new StringBuilder();
                    where.append(Thumbnails._ID);
                    where.append(" not in (");
                    for (int i = 0; i < length; i++) {
                        where.append(mIds.get(i));
                        if (i < (length - 1)) {
                            where.append(",");
                        }
                    }
                    where.append(")");
                    cr.delete(Thumbnails.CONTENT_URI, where.toString(), null);
                }
            } catch (SQLiteException e) {
                Log.e(LOGTAG, "PruneThumbnails: " + e);
            }
        }

    }

    @Override
    public WebViewFactory getWebViewFactory() {
        return mFactory;
    }

    @Override
    public void onSetWebView(Tab tab, WebView view) {
        mUi.onSetWebView(tab, view);
    }

    @Override
    public void createSubWindow(Tab tab) {
        endActionMode();
        WebView mainView = tab.getWebView();
        WebView subView = mFactory.createWebView((mainView == null)
                ? false
                : mainView.isPrivateBrowsingEnabled());
        mUi.createSubWindow(tab, subView);
    }

    @Override
    public Context getContext() {
        return mActivity;
    }

    @Override
    public Activity getActivity() {
        return mActivity;
    }

    void setUi(UI ui) {
        mUi = ui;
    }

    @Override
    public BrowserSettings getSettings() {
        return mSettings;
    }

    IntentHandler getIntentHandler() {
        return mIntentHandler;
    }

    @Override
    public UI getUi() {
        return mUi;
    }

    int getMaxTabs() {
        return mActivity.getResources().getInteger(R.integer.max_tabs);
    }

    @Override
    public TabControl getTabControl() {
        return mTabControl;
    }

    @Override
    public List<Tab> getTabs() {
        return mTabControl.getTabs();
    }

    // Open the icon database.
    private void openIconDatabase() {
        Log.i(LOGTAG, "controller.openIconDatabase");
        // We have to call getInstance on the UI thread
        final WebIconDatabase instance = WebIconDatabase.getInstance();
        BackgroundHandler.execute(new Runnable() {

            @Override
            public void run() {
                instance.open(mActivity.getDir("icons", 0).getPath());
            }
        });
    }

    private void startHandler() {
        mHandler = new Handler() {

            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case OPEN_BOOKMARKS:
                        bookmarksOrHistoryPicker(ComboViews.Bookmarks);
                        break;
                    /**
                     * Add for navigation tab
                     *@{
                     */
                    case FOCUS_NODE_HREF_NAVIGATION:
                        String navigationUrl = (String) msg.getData().get("url");
                        String navigationTitle = (String) msg.getData().get("title");
                        if (navigationUrl == "") break;
                        createNavigationMenu(navigationUrl, navigationTitle.trim(), msg.arg1);
                        break;
                    /*@}*/
                    case FOCUS_NODE_HREF:
                    {
                        String url = (String) msg.getData().get("url");
                        String title = (String) msg.getData().get("title");
                        String src = (String) msg.getData().get("src");
                        if (url == "") url = src; // use image if no anchor
                        if (TextUtils.isEmpty(url)) {
                            break;
                        }
                        HashMap focusNodeMap = (HashMap) msg.obj;
                        WebView view = (WebView) focusNodeMap.get("webview");
                        // Only apply the action if the top window did not change.
                        if (getCurrentTopWebView() != view) {
                            break;
                        }
                        switch (msg.arg1) {
                            case R.id.open_context_menu_id:
                                loadUrlFromContext(url);
                                break;
                            case R.id.view_image_context_menu_id:
                                loadUrlFromContext(src);
                                break;
                            case R.id.open_newtab_context_menu_id:
                                final Tab parent = mTabControl.getCurrentTab();
                                openTab(url, parent,
                                        !mSettings.openInBackground(), true);
                                break;
                            case R.id.copy_link_context_menu_id:
                                copy(url);
                                break;
                            case R.id.save_link_context_menu_id:
                            case R.id.download_context_menu_id:
                                /*
                                 * for download_storage_save_path
                                 *@{
                                 */
                                if (Util.SUPPORT_SELECT_DOWNLOAD_PATH){
                                    if (url != null){
                                        Log.i(LOGTAG, "Controller -- url for DRM   "+url);
                                        if (url.endsWith(".dr") || url.endsWith(".dcf") || url.endsWith(".dm")){
                                            DownloadHandler.onDownloadStartNoStream(Controller.this, mActivity, url, null, null,
                                                    null, view.isPrivateBrowsingEnabled(), false);
                                            return;
                                        }
                                    }
                                    DownloadHandler.onDownloadStart(Controller.this, mActivity, url,
                                            null, null, null, view.isPrivateBrowsingEnabled(),false,true);
                                } else {
                                    DownloadHandler.onDownloadStartNoStream(
                                            mActivity, url, view.getSettings().getUserAgentString(),
                                            null, null, null, view.isPrivateBrowsingEnabled());
                                }
                                /*@}*/
                                break;
                            /*SPRD:feature 522253 of  bookmark link menu:*
                              *@}
                            */
                            case R.id.bookmark_context_menu_id:
                                Intent intent = new Intent(mActivity,
                                        AddBookmarkPage.class);
                                intent.putExtra("url", url);
                                intent.putExtra("title", title);
                                mActivity.startActivity(intent);
                                break;
                        }
                        break;
                    }

                    case LOAD_URL:
                        loadUrlFromContext((String) msg.obj);
                        break;

                    case STOP_LOAD:
                        stopLoading();
                        break;

                    case RELEASE_WAKELOCK:
                        if (mWakeLock != null && mWakeLock.isHeld()) {
                            mWakeLock.release();
                            // if we reach here, Browser should be still in the
                            // background loading after WAKELOCK_TIMEOUT (5-min).
                            // To avoid burning the battery, stop loading.
                            mTabControl.stopAllLoading();
                        }
                        break;

                    case UPDATE_BOOKMARK_THUMBNAIL:
                        Tab tab = (Tab) msg.obj;
                        if (tab != null) {
                            updateScreenshot(tab);
                        }
                        break;
                    /*
                     * for download_overlay_same_file
                     *@{
                     */
                    case OVERLAY_DIALOG:
                        Boolean fileExist = (Boolean)msg.obj;
                        if(fileExist){
                            showOverlayDialog();
                        }
                        break;
                    /*@}*/
                    /* SPRD:504976 display incorrect while share page@{ */
                    case SHOW_SHAREPAGE:
                        Tab stab = (Tab) msg.obj;
                        sharePage(mActivity, stab.getTitle(),
                                stab.getUrl(), stab.getFavicon(),
                                createScreenshot(stab.getWebView(),
                                        getDesiredThumbnailWidth(mActivity),
                                        getDesiredThumbnailHeight(mActivity)));
                        break;
                    /* }@ */
                    /*
                     * for download_storage_save_path
                     *@{
                     */
                    case SET_TITLE:
                        mDownloadSize = (String)msg.obj;
                        if (mDownloadDialog != null && mDownloadDialog.isShowing()) {
                            /*only set download dialog title when onresume*/
                            if (!isActivityPaused()) {
                                mDownloadDialog.setTitle(mDownloadSize);
                                mDownloadSize = null;
                            }
                        }
                        break;
                    /* }@ */
                }
            }
        };

    }

    @Override
    public Tab getCurrentTab() {
        return mTabControl.getCurrentTab();
    }

    @Override
    public void shareCurrentPage() {
        shareCurrentPage(mTabControl.getCurrentTab());
    }

    private void shareCurrentPage(Tab tab) {
        if (tab != null) {
            /* SPRD:504976 display incorrect while share page@{ */
            InputMethodManager inputManager = (InputMethodManager) mActivity
                            .getSystemService(Activity.INPUT_METHOD_SERVICE);
            if(inputManager !=null && inputManager.isActive() && mActivity.getCurrentFocus() != null) {
                inputManager.hideSoftInputFromWindow(mActivity.getCurrentFocus()
                                .getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
            }

            mHandler.sendMessageDelayed(mHandler.obtainMessage(
                                SHOW_SHAREPAGE, 0, 0, tab),
                                400);
            /* }@ */
        }
    }

    /**
     * Share a page, providing the title, url, favicon, and a screenshot.  Uses
     * an {@link Intent} to launch the Activity chooser.
     * @param c Context used to launch a new Activity.
     * @param title Title of the page.  Stored in the Intent with
     *          {@link Intent#EXTRA_SUBJECT}
     * @param url URL of the page.  Stored in the Intent with
     *          {@link Intent#EXTRA_TEXT}
     * @param favicon Bitmap of the favicon for the page.  Stored in the Intent
     *          with {@link Browser#EXTRA_SHARE_FAVICON}
     * @param screenshot Bitmap of a screenshot of the page.  Stored in the
     *          Intent with {@link Browser#EXTRA_SHARE_SCREENSHOT}
     */
    static final void sharePage(Context c, String title, String url,
            Bitmap favicon, Bitmap screenshot) {
        Intent send = new Intent(Intent.ACTION_SEND);
        send.setType("text/plain");
        send.putExtra(Intent.EXTRA_TEXT, url);
        send.putExtra(Intent.EXTRA_SUBJECT, title);
        /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 start */
        send.putExtra(Browser.EXTRA_SHARE_FAVICON, Util.compressImage(favicon));
        send.putExtra(Browser.EXTRA_SHARE_SCREENSHOT, Util.compressImage(screenshot));
        /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 end */
        if(url != null && url.length() > c.getResources().getInteger(R.integer.max_url_length)){
            Toast.makeText(c, R.string.can_not_share_link, Toast.LENGTH_SHORT).show();
        }else{
            try {
                c.startActivity(Intent.createChooser(send, c.getString(
                    R.string.choosertitle_sharevia)));
            } catch(android.content.ActivityNotFoundException ex) {
            // if no app handles it, do nothing
            }
        }
    }

    private void copy(CharSequence text) {
        ClipboardManager cm = (ClipboardManager) mActivity
                .getSystemService(Context.CLIPBOARD_SERVICE);
        cm.setText(text);
    }

    // lifecycle

    @Override
    public void onConfgurationChanged(Configuration config) {
        mConfigChanged = true;
        // update the menu in case of a locale change
        mActivity.invalidateOptionsMenu();
        if (mPageDialogsHandler != null) {
            mPageDialogsHandler.onConfigurationChanged(config);
        }
        mUi.onConfigurationChanged(config);
    }

    @Override
    public void handleNewIntent(Intent intent) {
        if (!mUi.isWebShowing()) {
            mUi.showWeb(false);
        }
        /**
         *Add for wtai feature
         *Original Android code:
         * mIntentHandler.onNewIntent(intent);
         *@{
         */
        mIntentHandler.onNewIntent(intent,mUrlHandler);
        /*@}*/

    }

    @Override
    public void onPause() {
        if (mUi.isCustomViewShowing()) {
            hideCustomView();
        }
        if (mActivityPaused) {
            Log.e(LOGTAG, "BrowserActivity is already paused.");
            return;
        }
        mActivityPaused = true;
        Tab tab = mTabControl.getCurrentTab();
        if (tab != null) {
            tab.pause();
            if (!pauseWebViewTimers(tab)) {
                if (mWakeLock == null) {
                    PowerManager pm = (PowerManager) mActivity
                            .getSystemService(Context.POWER_SERVICE);
                    mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Browser");
                }
                mWakeLock.acquire();
                mHandler.sendMessageDelayed(mHandler
                        .obtainMessage(RELEASE_WAKELOCK), WAKELOCK_TIMEOUT);
            }
        }
        mUi.onPause();
        mNetworkHandler.onPause();

        WebView.disablePlatformNotifications();
        NfcHandler.unregister(mActivity);
        if (sThumbnailBitmap != null) {
            sThumbnailBitmap.recycle();
            sThumbnailBitmap = null;
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        /*SPRD:597617 menu is invalid while multi window mode changed@{ */
        outState.putBoolean("isNavigationBarMenuShowing", mUi.isMenuShowing());
        /*@}*/
        // Save all the tabs
        Bundle saveState = createSaveState();

        // crash recovery manages all save & restore state
        mCrashRecoveryHandler.writeState(saveState);
        mSettings.setLastRunPaused(true);
    }

    /**
     * Save the current state to outState. Does not write the state to
     * disk.
     * @return Bundle containing the current state of all tabs.
     */
    /* package */ Bundle createSaveState() {
        Bundle saveState = new Bundle();
        mTabControl.saveState(saveState);
        if (!saveState.isEmpty()) {
            // Save time so that we know how old incognito tabs (if any) are.
            saveState.putSerializable("lastActiveDate", Calendar.getInstance());
        }
        return saveState;
    }

    /*SPRD:597617 menu is invalid while multi window mode changed@{ */
    public void showNavigationBarMenu() {
        mUi.showMenu();
    }
    /*@}*/

    /*SPRD:604208 menubar display incorrect while multi window mode @{ */
    @Override
    public void setFullscreen(boolean enabled) {
        mUi.setFullscreen(enabled);
    }
    /* }@ */

    @Override
    public void onResume() {
        if (!mActivityPaused) {
            Log.e(LOGTAG, "BrowserActivity is already resumed.");
            return;
        }
        mSettings.setLastRunPaused(false);

        mActivityPaused = false;
        Tab current = mTabControl.getCurrentTab();
        if (current != null) {
            current.resume();
            resumeWebViewTimers(current);
        }
        releaseWakeLock();
        /**
         * Add for horizontal screen
         *@{
         */
        boolean horizontal = isHorizontal();
        if(horizontal){
            mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }
        /*@}*/
        mUi.onResume();
        mNetworkHandler.onResume();
        WebView.enablePlatformNotifications();
        NfcHandler.register(mActivity, this);
        if (mVoiceResult != null) {
            mUi.onVoiceResult(mVoiceResult);
            mVoiceResult = null;
        }
        /*
         * for download_storage_save_path
         *@{
         */
        /*set download dialog title when onresume*/
        if (mDownloadSize != null && mDownloadDialog != null && mDownloadDialog.isShowing()) {
            mDownloadDialog.setTitle(mDownloadSize);
            mDownloadSize = null;
        }
        /*@}*/
    }

    private void releaseWakeLock() {
        if (mWakeLock != null && mWakeLock.isHeld()) {
            mHandler.removeMessages(RELEASE_WAKELOCK);
            mWakeLock.release();
        }
    }

    /**
     * resume all WebView timers using the WebView instance of the given tab
     * @param tab guaranteed non-null
     */
    private void resumeWebViewTimers(Tab tab) {
        boolean inLoad = tab.inPageLoad();
        if ((!mActivityPaused && !inLoad) || (mActivityPaused && inLoad)) {
            CookieSyncManager.getInstance().startSync();
            WebView w = tab.getWebView();
            WebViewTimersControl.getInstance().onBrowserActivityResume(w);
        }
    }

    /**
     * Pause all WebView timers using the WebView of the given tab
     * @param tab
     * @return true if the timers are paused or tab is null
     */
    private boolean pauseWebViewTimers(Tab tab) {
        if (tab == null) {
            return true;
        } else if (!tab.inPageLoad()) {
            CookieSyncManager.getInstance().stopSync();
            WebViewTimersControl.getInstance().onBrowserActivityPause(getCurrentWebView());
            return true;
        }
        return false;
    }

    @Override
    public void onDestroy() {
        closeDialogMessage();
        if (mUploadHandler != null && !mUploadHandler.handled()) {
            mUploadHandler.onResult(Activity.RESULT_CANCELED, null);
            mUploadHandler = null;
        }
        if (mTabControl == null) return;
        mUi.onDestroy();
        // Remove the current tab and sub window
        Tab t = mTabControl.getCurrentTab();
        if (t != null) {
            dismissSubWindow(t);
            removeTab(t);
        }

        /**
         * Add for oma download
         *@{
         */
        if (mDownloadSucessReceiver != null) {
            mActivity.unregisterReceiver(mDownloadSucessReceiver);
            mDownloadSucessReceiver = null;
        }
        /*@}*/
        mActivity.getContentResolver().unregisterContentObserver(mBookmarksObserver);
        /**
         * Add for navigation tab
         *@{
         */
        mActivity.getContentResolver().unregisterContentObserver(mSiteNavigationObserver);
        /*@}*/
        // Destroy all the tabs
        mTabControl.destroy();
        WebIconDatabase.getInstance().close();
        // Stop watching the default geolocation permissions
        mSystemAllowGeolocationOrigins.stop();
        mSystemAllowGeolocationOrigins = null;
    }

    protected boolean isActivityPaused() {
        return mActivityPaused;
    }

    @Override
    public void onLowMemory() {
        mTabControl.freeMemory();
    }

    @Override
    public boolean shouldShowErrorConsole() {
        return mShouldShowErrorConsole;
    }

    protected void setShouldShowErrorConsole(boolean show) {
        if (show == mShouldShowErrorConsole) {
            // Nothing to do.
            return;
        }
        mShouldShowErrorConsole = show;
        Tab t = mTabControl.getCurrentTab();
        if (t == null) {
            // There is no current tab so we cannot toggle the error console
            return;
        }
        mUi.setShouldShowErrorConsole(t, show);
    }

    @Override
    public void stopLoading() {
        mLoadStopped = true;
        Tab tab = mTabControl.getCurrentTab();
        WebView w = getCurrentTopWebView();
        if (w != null) {
            w.stopLoading();
            mUi.onPageStopped(tab);
        }
    }

    boolean didUserStopLoading() {
        return mLoadStopped;
    }

    // WebViewController

    @Override
    public void onPageStarted(Tab tab, WebView view, Bitmap favicon) {

        // We've started to load a new page. If there was a pending message
        // to save a screenshot then we will now take the new page and save
        // an incorrect screenshot. Therefore, remove any pending thumbnail
        // messages from the queue.
        mHandler.removeMessages(Controller.UPDATE_BOOKMARK_THUMBNAIL,
                tab);

        // reset sync timer to avoid sync starts during loading a page
        CookieSyncManager.getInstance().resetSync();

        if (!mNetworkHandler.isNetworkUp()) {
            view.setNetworkAvailable(false);
        }

        // when BrowserActivity just starts, onPageStarted may be called before
        // onResume as it is triggered from onCreate. Call resumeWebViewTimers
        // to start the timer. As we won't switch tabs while an activity is in
        // pause state, we can ensure calling resume and pause in pair.
        if (mActivityPaused) {
            resumeWebViewTimers(tab);
        }
        mLoadStopped = false;
        endActionMode();

        mUi.onTabDataChanged(tab);

        String url = tab.getUrl();
        /*modify for favicon not update when nav to another site*/
        // update the bookmark database for favicon
        //maybeUpdateFavicon(tab, null, url, favicon);

        Performance.tracePageStart(url);

        // Performance probe
        if (false) {
            Performance.onPageStarted();
        }

        /**
         *Add for bottom bar
         *@{
         */
        if (tab != null && tab.inForeground()) {
            updateMenuBarState();
        }
        /*@}*/
    }

    @Override
    public void onPageFinished(Tab tab) {
        mCrashRecoveryHandler.backupState();
        mUi.onTabDataChanged(tab);

        // Performance probe
        if (false) {
            Performance.onPageFinished(tab.getUrl());
         }

        /**
         *Add for bottom bar
         *@{
         */
        if (tab != null && tab.inForeground()) {
            updateMenuBarState();
        }
        /*@}*/

        Performance.tracePageFinished();
    }

    @Override
    public void onProgressChanged(Tab tab) {
        int newProgress = tab.getLoadProgress();

        if (newProgress == 100) {
            CookieSyncManager.getInstance().sync();
            // onProgressChanged() may continue to be called after the main
            // frame has finished loading, as any remaining sub frames continue
            // to load. We'll only get called once though with newProgress as
            // 100 when everything is loaded. (onPageFinished is called once
            // when the main frame completes loading regardless of the state of
            // any sub frames so calls to onProgressChanges may continue after
            // onPageFinished has executed)
            /* seprate reload and stop menu
            if (tab.inPageLoad()) {
                updateInLoadMenuItems(mCachedMenu, tab);
            } else
            */
            if (mActivityPaused && pauseWebViewTimers(tab)) {
                // pause the WebView timer and release the wake lock if it is
                // finished while BrowserActivity is in pause state.
                releaseWakeLock();
            }
            if (!tab.isPrivateBrowsingEnabled()
                    && !TextUtils.isEmpty(tab.getUrl())
                    && !tab.isSnapshot()) {
                // Only update the bookmark screenshot if the user did not
                // cancel the load early and there is not already
                // a pending update for the tab.
                if (tab.shouldUpdateThumbnail() &&
                        (tab.inForeground() && !didUserStopLoading()
                        || !tab.inForeground())) {
                    if (!mHandler.hasMessages(UPDATE_BOOKMARK_THUMBNAIL, tab)) {
                        mHandler.sendMessageDelayed(mHandler.obtainMessage(
                                UPDATE_BOOKMARK_THUMBNAIL, 0, 0, tab),
                                1000);
                    }
                }
            }
        }
        /* seprate reload and stop menu
        else {
            if (!tab.inPageLoad()) {
                // onPageFinished may have already been called but a subframe is
                // still loading
                // updating the progress and
                // update the menu items.
                updateInLoadMenuItems(mCachedMenu, tab);
            }
        }
        */
        mUi.onProgressChanged(tab);
    }

    @Override
    public void onUpdatedSecurityState(Tab tab) {
        mUi.onTabDataChanged(tab);
    }

    @Override
    public void onReceivedTitle(Tab tab, final String title) {
        mUi.onTabDataChanged(tab);
        /*SPRD: add for bug 568641 @{*/
        if (tab != null && tab.inForeground()) {
            updateMenuBarState();
        }
        /*@}*/
        final String pageUrl = tab.getOriginalUrl();
        if (TextUtils.isEmpty(pageUrl) || pageUrl.length()
                >= SQLiteDatabase.SQLITE_MAX_LIKE_PATTERN_LENGTH) {
            return;
        }
        // Update the title in the history database if not in private browsing mode
        if (!tab.isPrivateBrowsingEnabled()) {
            DataController.getInstance(mActivity).updateHistoryTitle(pageUrl, title);
        }
    }

    @Override
    public void onFavicon(Tab tab, WebView view, Bitmap icon) {
        mUi.onTabDataChanged(tab);
        maybeUpdateFavicon(tab, view.getOriginalUrl(), view.getUrl(), icon);
    }

    @Override
    public boolean shouldOverrideUrlLoading(Tab tab, WebView view, String url) {
        /**
         *Add for bottom bar
         *@{
         */
        if (tab != null && tab.inForeground()) {
            updateMenuBarState();
        }
        /*@}*/
        return mUrlHandler.shouldOverrideUrlLoading(tab, view, url);
    }

    @Override
    public boolean shouldOverrideKeyEvent(KeyEvent event) {
        if (mMenuIsDown) {
            // only check shortcut key when MENU is held
            return mActivity.getWindow().isShortcutKey(event.getKeyCode(),
                    event);
        } else {
            return false;
        }
    }

    @Override
    public boolean onUnhandledKeyEvent(KeyEvent event) {
        if (!isActivityPaused()) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                return mActivity.onKeyDown(event.getKeyCode(), event);
            } else {
                return mActivity.onKeyUp(event.getKeyCode(), event);
            }
        }
        return false;
    }

    @Override
    public void doUpdateVisitedHistory(Tab tab, boolean isReload) {
        // Don't save anything in private browsing mode
        if (tab.isPrivateBrowsingEnabled()) return;
        String url = tab.getOriginalUrl();

        if (TextUtils.isEmpty(url)
                || url.regionMatches(true, 0, "about:", 0, 6)) {
            return;
        }
        /*
         * Add for save_webpage
         *@{
         */
        if ((tab != null && tab.isSnapshot()) || (url != null && url.startsWith("file:"))) {
            return;
        }
        /*@}*/
        DataController.getInstance(mActivity).updateVisitedHistory(url);
        mCrashRecoveryHandler.backupState();
    }

    @Override
    public void getVisitedHistory(final ValueCallback<String[]> callback) {
        AsyncTask<Void, Void, String[]> task =
                new AsyncTask<Void, Void, String[]>() {
            @Override
            public String[] doInBackground(Void... unused) {
                return Util.getVisitedHistory(mActivity.getContentResolver());
            }
            @Override
            public void onPostExecute(String[] result) {
                callback.onReceiveValue(result);
            }
        };
        task.execute();
    }

    @Override
    public void onReceivedHttpAuthRequest(Tab tab, WebView view,
            final HttpAuthHandler handler, final String host,
            final String realm) {
        String username = null;
        String password = null;

        boolean reuseHttpAuthUsernamePassword
                = handler.useHttpAuthUsernamePassword();

        if (reuseHttpAuthUsernamePassword && view != null) {
            String[] credentials = view.getHttpAuthUsernamePassword(host, realm);
            if (credentials != null && credentials.length == 2) {
                username = credentials[0];
                password = credentials[1];
            }
        }

        if (username != null && password != null) {
            handler.proceed(username, password);
        } else {
            if (tab.inForeground() && !handler.suppressDialog()) {
                mPageDialogsHandler.showHttpAuthentication(tab, handler, host, realm);
            } else {
                handler.cancel();
            }
        }
    }

    @Override
    public void onDownloadStart(Tab tab, String url, String userAgent,
            String contentDisposition, String mimetype, String referer,
            long contentLength) {
        WebView w = tab.getWebView();
        /*
         * for download_storage_save_path
         * Orignal android code:
        DownloadHandler.onDownloadStart(mActivity, url, userAgent,
                contentDisposition, mimetype, referer, w.isPrivateBrowsingEnabled());
         *@{
         */
        if(Util.SUPPORT_SELECT_DOWNLOAD_PATH){
            if ((mimetype!=null)&&(!mimetype.isEmpty())) {
                mimetype = mimetype.replaceAll("\"","");
            }
            MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
            if (mimeTypeMap != null && (mimetype == null || "".equals(mimetype))) {
                mimetype = mimeTypeMap.getMimeTypeFromExtension(MimeTypeMap.getFileExtensionFromUrl(url));
            }
            Log.i(LOGTAG, "onDownloadStart mimeType "+mimetype);
            boolean normalMimeType = Common.getBrowserDrmPlugIn().getMimeType(mimetype);
            if ((mimetype != null && !mimetype.equals("application/vnd.oma.dd+xml")
                && !mimetype.equals("application/vnd.oma.drm.message")
                && normalMimeType
                && !mimetype.equals("application/vnd.oma.drm.content"))
                || mimetype==null) {
                Log.i(LOGTAG, "DownloadHandler.onDownloadStart");
                DownloadHandler.onDownloadStart(this, mActivity, url, userAgent, contentDisposition, mimetype, false,false,false);
            }else{
                Log.i(LOGTAG, "DownloadHandler.onDownloadStartNoStream");
                DownloadHandler.onDownloadStartNoStream(this, mActivity, url, userAgent, contentDisposition, mimetype, false,false);
            }

        }else{
            DownloadHandler.onDownloadStart(mActivity, url, userAgent,
                contentDisposition, mimetype, referer, w.isPrivateBrowsingEnabled());
        }

        /*@}*/

        if ((w != null) && (w.copyBackForwardList().getSize() == 0)) {
            // This Tab was opened for the sole purpose of downloading a
            // file. Remove it.
            if (tab == mTabControl.getCurrentTab()) {
                // After download, don't close the current tab
                // In this case, the Tab is still on top.
                //goBackOnePageOrQuit();
            } else {
                // In this case, it is not.
                closeTab(tab);
            }
        }
    }

    @Override
    public Bitmap getDefaultVideoPoster() {
        return mUi.getDefaultVideoPoster();
    }

    @Override
    public View getVideoLoadingProgressView() {
        return mUi.getVideoLoadingProgressView();
    }

    @Override
    public void showSslCertificateOnError(WebView view, SslErrorHandler handler,
            SslError error) {
        mPageDialogsHandler.showSSLCertificateOnError(view, handler, error);
    }

    @Override
    public void showAutoLogin(Tab tab) {
        assert tab.inForeground();
        // Update the title bar to show the auto-login request.
        mUi.showAutoLogin(tab);
    }

    @Override
    public void hideAutoLogin(Tab tab) {
        assert tab.inForeground();
        mUi.hideAutoLogin(tab);
    }

    // helper method

    /*
     * Update the favorites icon if the private browsing isn't enabled and the
     * icon is valid.
     */
    private void maybeUpdateFavicon(Tab tab, final String originalUrl,
            final String url, Bitmap favicon) {
        if (favicon == null) {
            return;
        }
        if (!tab.isPrivateBrowsingEnabled()) {
            Bookmarks.updateFavicon(mActivity
                    .getContentResolver(), originalUrl, url, favicon);
        }
    }

    @Override
    public void bookmarkedStatusHasChanged(Tab tab) {
        // TODO: Switch to using onTabDataChanged after b/3262950 is fixed
        mUi.bookmarkedStatusHasChanged(tab);
    }

    // end WebViewController

    protected void pageUp() {
        getCurrentTopWebView().pageUp(false);
    }

    protected void pageDown() {
        getCurrentTopWebView().pageDown(false);
    }

    // callback from phone title bar
    @Override
    public void editUrl() {
        if (mOptionsMenuOpen) mActivity.closeOptionsMenu();
        mUi.editUrl(false, true);
    }

    @Override
    public void showCustomView(Tab tab, View view, int requestedOrientation,
            WebChromeClient.CustomViewCallback callback) {
        if (tab.inForeground()) {
            if (mUi.isCustomViewShowing()) {
                callback.onCustomViewHidden();
                return;
            }
            mUi.showCustomView(view, requestedOrientation, callback);
            // Save the menu state and set it to empty while the custom
            // view is showing.
            mOldMenuState = mMenuState;
            mMenuState = EMPTY_MENU;
            mActivity.invalidateOptionsMenu();
        }
    }

    @Override
    public void hideCustomView() {
        if (mUi.isCustomViewShowing()) {
            mUi.onHideCustomView();
            // Reset the old menu state.
            mMenuState = mOldMenuState;
            mOldMenuState = EMPTY_MENU;
            mActivity.invalidateOptionsMenu();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode,
            Intent intent) {
        if (getCurrentTopWebView() == null) return;
        switch (requestCode) {
            case PREFERENCES_PAGE:
                if (resultCode == Activity.RESULT_OK && intent != null) {
                    String action = intent.getStringExtra(Intent.EXTRA_TEXT);
                    if (PreferenceKeys.PREF_PRIVACY_CLEAR_HISTORY.equals(action)) {
                        mTabControl.removeParentChildRelationShips();
                    }
                }
                break;
            case FILE_SELECTED:
                // Chose a file from the file picker.
                if (null == mUploadHandler) break;
                mUploadHandler.onResult(resultCode, intent);
                break;
            case COMBO_VIEW:
                if (intent == null || resultCode != Activity.RESULT_OK) {
                    break;
                }
                mUi.showWeb(false);
                if (Intent.ACTION_VIEW.equals(intent.getAction())) {
                    Tab t = getCurrentTab();
                    Uri uri = intent.getData();
                    loadUrl(t, uri.toString());
                } else if (intent.hasExtra(ComboViewActivity.EXTRA_OPEN_ALL)) {
                    String[] urls = intent.getStringArrayExtra(
                            ComboViewActivity.EXTRA_OPEN_ALL);
                    Tab parent = getCurrentTab();
                    for (String url : urls) {
                        parent = openTab(url, parent,
                                !mSettings.openInBackground(), true);
                    }
                } else if (intent.hasExtra(ComboViewActivity.EXTRA_OPEN_SNAPSHOT)) {
                    long id = intent.getLongExtra(
                            ComboViewActivity.EXTRA_OPEN_SNAPSHOT, -1);
                    if (id >= 0) {
                        /*
                         * for save_webpage
                         * original code
                        Toast.makeText(mActivity, "Snapshot Tab no longer supported",
                            Toast.LENGTH_LONG).show();
                         *@{
                         */
                        createNewSnapshotTab(id, true);
                        /*@}*/
                    }
                }
                break;
            case VOICE_RESULT:
                if (resultCode == Activity.RESULT_OK && intent != null) {
                    ArrayList<String> results = intent.getStringArrayListExtra(
                            RecognizerIntent.EXTRA_RESULTS);
                    if (results.size() >= 1) {
                        mVoiceResult = results.get(0);
                    }
                }
                break;
            /*
             *  for download_storage_save_path
             *@{
             */
            case DOWNLOAD_SELECTED:
                if(null != intent){
                    Bundle bundle = intent.getExtras();
                    mDownloadPathStr = bundle.getString("absolutepath",getDownloadPath());
                    Log.i(LOGTAG, "Select download path is: " + mDownloadPathStr);
                    dismissSelectStorageDialog();
                    if (null != mDownloadPath) {
                        mDownloadPath.setText(mDownloadPathStr);
                    }
                }
                break;
            /*@}*/

            default:
                break;
        }
        getCurrentTopWebView().requestFocus();
    }

    /**
     * Open the Go page.
     * @param startWithHistory If true, open starting on the history tab.
     *                         Otherwise, start with the bookmarks tab.
     */
    @Override
    public void bookmarksOrHistoryPicker(ComboViews startView) {
        if (mTabControl.getCurrentWebView() == null) {
            return;
        }
        // clear action mode
        if (isInCustomActionMode()) {
            endActionMode();
        }
        Bundle extras = new Bundle();
        // Disable opening in a new window if we have maxed out the windows
        extras.putBoolean(BrowserBookmarksPage.EXTRA_DISABLE_WINDOW,
                !mTabControl.canCreateNewTab());
        mUi.showComboView(startView, extras);
    }

    // combo view callbacks

    // key handling
    protected void onBackKey() {
        if (!mUi.onBackKey()) {
            WebView subwindow = mTabControl.getCurrentSubWindow();
            if (subwindow != null) {
                if (subwindow.canGoBack()) {
                    subwindow.goBack();
                } else {
                    dismissSubWindow(mTabControl.getCurrentTab());
                }
            } else {
                goBackOnePageOrQuit();
            }
        }
    }

    protected boolean onMenuKey() {
        return mUi.onMenuKey();
    }

    // menu handling and state
    // TODO: maybe put into separate handler

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (mMenuState == EMPTY_MENU) {
            return false;
        }
        MenuInflater inflater = mActivity.getMenuInflater();
        inflater.inflate(R.menu.browser, menu);
        return true;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
            ContextMenuInfo menuInfo) {
        /**
         * Add for navigation tab
         *@{
         */
        mContextMenu = menu;
        /*@}*/

        if (v instanceof TitleBar) {
            return;
        }
        if (!(v instanceof WebView)) {
            return;
        }
        final WebView webview = (WebView) v;
        WebView.HitTestResult result = webview.getHitTestResult();
        if (result == null) {
            return;
        }

        int type = result.getType();
        if (type == WebView.HitTestResult.UNKNOWN_TYPE) {
            Log.w(LOGTAG,
                    "We should not show context menu when nothing is touched");
            return;
        }
        if (type == WebView.HitTestResult.EDIT_TEXT_TYPE) {
            // let TextView handles context menu
            return;
        }

        // Note, http://b/issue?id=1106666 is requesting that
        // an inflated menu can be used again. This is not available
        // yet, so inflate each time (yuk!)
        MenuInflater inflater = mActivity.getMenuInflater();
        inflater.inflate(R.menu.browsercontext, menu);

        // Show the correct menu group
        final String extra = result.getExtra();
        if (extra == null) return;

        /**
         * Add for navigation tab
         *@{
         */
        if (Util.BROWSER_SITE_NAVIGATION_SUPPORT) {
            String url = webview.getOriginalUrl();
            if (url != null && url.equalsIgnoreCase(SiteNavigation.SITE_NAVIGATION)) {
                HashMap<String, WebView> hrefMap =
                        new HashMap<String, WebView>();
                hrefMap.put("webview", webview);
                Message msg = mHandler.obtainMessage(
                            FOCUS_NODE_HREF_NAVIGATION,
                            type, 0, hrefMap);
                webview.requestFocusNodeHref(msg);

                return;
            }
        }

        // If site navigation is not enabled, then hide the two menu groups
        menu.setGroupVisible(R.id.SITE_NAVIGATION_EDIT, false);
        menu.setGroupVisible(R.id.SITE_NAVIGATION_ADD, false);
        /*@}*/

        // Other group should be normal
        menu.setGroupVisible(R.id.PHONE_MENU,
                type == WebView.HitTestResult.PHONE_TYPE);
        menu.setGroupVisible(R.id.EMAIL_MENU,
                type == WebView.HitTestResult.EMAIL_TYPE);
        menu.setGroupVisible(R.id.GEO_MENU,
                type == WebView.HitTestResult.GEO_TYPE);
        menu.setGroupVisible(R.id.IMAGE_MENU,
                type == WebView.HitTestResult.IMAGE_TYPE
                || type == WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE);
        menu.setGroupVisible(R.id.ANCHOR_MENU,
                type == WebView.HitTestResult.SRC_ANCHOR_TYPE
                || type == WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE);

        // Setup custom handling depending on the type
        switch (type) {
            case WebView.HitTestResult.PHONE_TYPE:
                menu.setHeaderTitle(Uri.decode(extra));
                menu.findItem(R.id.dial_context_menu_id).setIntent(
                        new Intent(Intent.ACTION_VIEW, Uri
                                .parse(WebView.SCHEME_TEL + extra)));
                Intent addIntent = new Intent(Intent.ACTION_INSERT_OR_EDIT);
                addIntent.putExtra(Insert.PHONE, Uri.decode(extra));
                addIntent.setType(ContactsContract.Contacts.CONTENT_ITEM_TYPE);
                menu.findItem(R.id.add_contact_context_menu_id).setIntent(
                        addIntent);
                menu.findItem(R.id.copy_phone_context_menu_id)
                        .setOnMenuItemClickListener(
                        new Copy(extra));
                break;

            case WebView.HitTestResult.EMAIL_TYPE:
                menu.setHeaderTitle(extra);
                menu.findItem(R.id.email_context_menu_id).setIntent(
                        new Intent(Intent.ACTION_VIEW, Uri
                                .parse(WebView.SCHEME_MAILTO + extra)));
                menu.findItem(R.id.copy_mail_context_menu_id)
                        .setOnMenuItemClickListener(
                        new Copy(extra));
                break;

            case WebView.HitTestResult.GEO_TYPE:
                menu.setHeaderTitle(extra);
                menu.findItem(R.id.map_context_menu_id).setIntent(
                        new Intent(Intent.ACTION_VIEW, Uri
                                .parse(WebView.SCHEME_GEO
                                        + URLEncoder.encode(extra))));
                menu.findItem(R.id.copy_geo_context_menu_id)
                        .setOnMenuItemClickListener(
                        new Copy(extra));
                break;

            case WebView.HitTestResult.SRC_ANCHOR_TYPE:
            case WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE:
                /*
                 * Add for save_webpage
                 *@{
                 */
                Tab anchorTab = getCurrentTab();
                if ((anchorTab != null && anchorTab.isSnapshot()) || extra.startsWith("file:")) {
                    menu.findItem(R.id.save_link_context_menu_id).setVisible(false);
                }

                if (extra.startsWith("file:")) {
                    menu.findItem(R.id.bookmark_context_menu_id).setVisible(false);
                }
                /*@}*/

                menu.setHeaderTitle(extra);
                // decide whether to show the open link in new tab option
                boolean showNewTab = mTabControl.canCreateNewTab();
                MenuItem newTabItem
                        = menu.findItem(R.id.open_newtab_context_menu_id);
                newTabItem.setTitle(getSettings().openInBackground()
                        ? R.string.contextmenu_openlink_newwindow_background
                        : R.string.contextmenu_openlink_newwindow);
                newTabItem.setVisible(showNewTab);
                if (showNewTab) {
                    if (WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE == type) {
                        newTabItem.setOnMenuItemClickListener(
                                new MenuItem.OnMenuItemClickListener() {
                                    @Override
                                    public boolean onMenuItemClick(MenuItem item) {
                                        final HashMap<String, WebView> hrefMap =
                                                new HashMap<String, WebView>();
                                        hrefMap.put("webview", webview);
                                        final Message msg = mHandler.obtainMessage(
                                                FOCUS_NODE_HREF,
                                                R.id.open_newtab_context_menu_id,
                                                0, hrefMap);
                                        webview.requestFocusNodeHref(msg);
                                        return true;
                                    }
                                });
                    } else {
                        newTabItem.setOnMenuItemClickListener(
                                new MenuItem.OnMenuItemClickListener() {
                                    @Override
                                    public boolean onMenuItemClick(MenuItem item) {
                                        final Tab parent = mTabControl.getCurrentTab();
                                        openTab(extra, parent,
                                                !mSettings.openInBackground(),
                                                true);
                                        return true;
                                    }
                                });
                    }
                }
                if (type == WebView.HitTestResult.SRC_ANCHOR_TYPE) {
                    break;
                }
                // otherwise fall through to handle image part
            case WebView.HitTestResult.IMAGE_TYPE:
                MenuItem shareItem = menu.findItem(R.id.share_link_context_menu_id);
                shareItem.setVisible(type == WebView.HitTestResult.IMAGE_TYPE);
                if (type == WebView.HitTestResult.IMAGE_TYPE) {
                    /* SPRD: Delete for bug 557526 {@ */
                    /*menu.setHeaderTitle(extra);*/
                    /* @} */
                    shareItem.setOnMenuItemClickListener(
                            new MenuItem.OnMenuItemClickListener() {
                                @Override
                                public boolean onMenuItemClick(MenuItem item) {
                                    sharePage(mActivity, null, extra, null,
                                    null);
                                    return true;
                                }
                            }
                        );
                }
                menu.findItem(R.id.view_image_context_menu_id)
                        .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                    @Override
                    public boolean onMenuItemClick(MenuItem item) {
                        /* SPRD: ADD for bug 563287 {@ */
                        byte[] bytes = extra.getBytes();
                        if ((bytes.length/1024) > MAX_BYTES) {
                            Log.e(LOGTAG,"The image is too lage to view");
                            /*menu.findItem(R.id.view_image_context_menu_id).setVisible(false);*/
                            Toast.makeText(mActivity, R.string.can_not_view_image, Toast.LENGTH_SHORT).show();
                            return false;
                        }
                        /* @} */
                        if (isImageViewableUri(Uri.parse(extra))) {
                            openTab(extra, mTabControl.getCurrentTab(), true, true);
                        } else {
                            Log.e(LOGTAG, "Refusing to view image with invalid URI, \"" +
                                    extra + "\"");
                        }
                        return false;
                    }
                });

                /*
                 * Add for save_webpage
                 *@{
                 */
                Tab imageTab = getCurrentTab();
                if ((imageTab != null && imageTab.isSnapshot()) || extra.startsWith("file:")) {
                    menu.findItem(R.id.download_context_menu_id).setVisible(false);
                }
                /*@}*/

                /*
                 * for download_storage_save_path
                 * Orignal android code:
                 *@{
                 */
                if(Util.SUPPORT_SELECT_DOWNLOAD_PATH){
                    menu.findItem(R.id.download_context_menu_id).setOnMenuItemClickListener(
                            new Download(this, mActivity, extra, webview.isPrivateBrowsingEnabled(),
                                    webview.getSettings().getUserAgentString()));
                }else{
                    menu.findItem(R.id.download_context_menu_id).setOnMenuItemClickListener(
                            new Download(mActivity, extra, webview.isPrivateBrowsingEnabled(),
                                    webview.getSettings().getUserAgentString()));
                }
                /*@}*/
                menu.findItem(R.id.set_wallpaper_context_menu_id).
                        setOnMenuItemClickListener(new WallpaperHandler(mActivity,
                                extra));
                break;

            default:
                Log.w(LOGTAG, "We should not get here.");
                break;
        }
        //update the ui
        mUi.onContextMenuCreated(menu);
    }

    /**
     * Add for navigation tab
     *@{
     */
    private void createNavigationMenu(String url, String name, int type) {
        ContextMenu menu = mContextMenu;
        if (menu == null) return;

        final String imageAnchorUrlExtra = url;
        String itemUrl = null;
        itemUrl = Uri.decode(imageAnchorUrlExtra);
        // If item's url != null, then should show edit menu; otherwise if it is null, then should show "add" menu
        if (itemUrl != null) {
            if (isSiteNavigationAdd(itemUrl)) {
                menu.setGroupVisible(R.id.SITE_NAVIGATION_ADD, type == WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE);
                menu.setGroupVisible(R.id.SITE_NAVIGATION_EDIT, false);
            } else {
                menu.setGroupVisible(R.id.SITE_NAVIGATION_ADD, false);
                menu.setGroupVisible(R.id.SITE_NAVIGATION_EDIT, type == WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE);
            }

            // Other group should be invisible
            menu.setGroupVisible(R.id.PHONE_MENU, false);
            menu.setGroupVisible(R.id.EMAIL_MENU, false);
            menu.setGroupVisible(R.id.GEO_MENU, false);
            menu.setGroupVisible(R.id.IMAGE_MENU, false);
            menu.setGroupVisible(R.id.ANCHOR_MENU, false);

        } else {
            Log.d(LOGTAG, "sitenavigation onCreateContextMenu itemUrl is null! Anchor text selected.");
            menu.setGroupVisible(R.id.SITE_NAVIGATION_ADD, false);
            menu.setGroupVisible(R.id.SITE_NAVIGATION_EDIT, false);
            menu.setGroupVisible(R.id.PHONE_MENU, false);
            menu.setGroupVisible(R.id.EMAIL_MENU, false);
            menu.setGroupVisible(R.id.GEO_MENU, false);
            menu.setGroupVisible(R.id.IMAGE_MENU, false);
            menu.setGroupVisible(R.id.ANCHOR_MENU, false);
        }

        // Setup custom handling depending on the type
        switch (type) {
            case WebView.HitTestResult.SRC_IMAGE_ANCHOR_TYPE:

                        // Set the menu title
                        if (isSiteNavigationAdd(imageAnchorUrlExtra)) {
                            menu.setHeaderTitle(SiteNavigation.BROWSER_NAVIGATION);
                        } else if (imageAnchorUrlExtra != null) {
                            if (imageAnchorUrlExtra.length() <= MAX_TITLE_LENGTH) {
                                menu.setHeaderTitle(imageAnchorUrlExtra);
                            } else {
                                menu.setHeaderTitle(imageAnchorUrlExtra.substring(0, MAX_TITLE_LENGTH));
                            }
                        }

                        // Set the openInNewTab invisible
                        menu.findItem(R.id.open_newtab_context_menu_id).setVisible(false);

                        if (itemUrl != null) {
                            if (isSiteNavigationAdd(itemUrl)) {
                                // Add menu
                                menu.findItem(R.id.add_sn_context_menu_id)
                                        .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                                            @Override
                                            public boolean onMenuItemClick(MenuItem item) {
                                                final Intent intent = new Intent(Controller.this
                                                        .getContext(),
                                                        SiteNavigationAddDialog.class);
                                                Bundle bundle = new Bundle();
                                                String url = imageAnchorUrlExtra;
                                                bundle.putBoolean("isAdding", true);
                                                bundle.putString("url", url);
                                                bundle.putString("name", name);
                                                intent.putExtra("websites", bundle);
                                                mActivity.startActivityForResult(intent,
                                                        SITE_NAVIGATION_ADD_DIALOG);
                                                return false;
                                            }
                                        });
                            } else {
                                // Edit menu
                                menu.findItem(R.id.edit_sn_context_menu_id)
                                        .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                                            @Override
                                            public boolean onMenuItemClick(MenuItem item) {
                                                final Intent intent = new Intent(Controller.this
                                                        .getContext(),
                                                        SiteNavigationAddDialog.class);
                                                Bundle bundle = new Bundle();
                                                String url = getUrlFromName(name);
                                                bundle.putBoolean("isAdding", false);
                                                bundle.putString("url", url);
                                                bundle.putString("name", name);
                                                intent.putExtra("websites", bundle);
                                                mActivity.startActivityForResult(intent,
                                                        SITE_NAVIGATION_ADD_DIALOG);
                                                return false;
                                            }
                                        });
                                // Delete menu
                                menu.findItem(R.id.delete_sn_context_menu_id)
                                        .setOnMenuItemClickListener(new OnMenuItemClickListener() {
                                            @Override
                                            public boolean onMenuItemClick(MenuItem item) {
                                                showSiteNavigationDeleteDialog(name);
                                                return false;
                                            }
                                        });
                            }
                }
                break;

            default:
                Log.w(LOGTAG, "We should not get here.");
                break;
        }
    }

    private void showSiteNavigationDeleteDialog(final String name) {
        int title = R.string.delete;
        int msg = R.string.delete_site_navigation_msg;
        new AlertDialog.Builder(this.getContext())
            .setTitle(title)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setMessage(msg)
            .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int whichButton) {
                    deleteSiteNavigationItem(name);
                }
            })
            .setNegativeButton(R.string.cancel, null)
            .show();
    }


    private void deleteSiteNavigationItem(final String name) {
        ContentResolver cr = this.getContext().getContentResolver();
        cr.delete(SiteNavigation.SITE_NAVIGATION_URI,
                "title = ?", new String[] {name});
        Log.d(LOGTAG, "Controller deleteSiteNavigationItem name is : " + name);

        //After delete, reload the page
        if (getCurrentTopWebView() != null) {
            getCurrentTopWebView().reload();
        }
    }

    public static final int SITE_NAVIGATION_ADD_DIALOG = 7;

    private String getUrlFromName(String name) {
        ContentResolver cr = this.getContext().getContentResolver();
        Cursor cursor = null;
        String url = null;
        try {
            cursor = cr.query(SiteNavigation.SITE_NAVIGATION_URI,
                    new String[] {SiteNavigation.URL}, "title = ?", new String[] {name}, null);
            if (null != cursor && cursor.moveToFirst()) {
                url = cursor.getString(0);
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
        Log.d(LOGTAG, "sitenavigation getNameFromUrl name is : " + name + ", will return url : " + url);
        return url;
    }

    private void updateSiteNavigationThumbnail(final String itemUrl, WebView webView) {
        int width = mActivity.getResources().getDimensionPixelOffset(R.dimen.siteNavigationThumbnailWidth);
        int height = mActivity.getResources().getDimensionPixelOffset(R.dimen.siteNavigationThumbnailHeight);

        final Bitmap bm = createScreenshot(webView, width, height);

        if (bm == null) {
            Log.e(LOGTAG, "updateSiteNavigationThumbnail bm is null!");
            return;
        }

        final ContentResolver cr = mActivity.getContentResolver();
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... unused) {
                ContentResolver cr = mActivity.getContentResolver();
                Cursor cursor = null;
                try {
                    cursor = cr.query(SiteNavigation.SITE_NAVIGATION_URI,
                            new String[] {SiteNavigation.ID}, "url = ?", new String[] {itemUrl}, null);
                    if (null != cursor && cursor.moveToFirst()) {
                        final ByteArrayOutputStream os = new ByteArrayOutputStream();
                        bm.compress(Bitmap.CompressFormat.PNG, 100, os);

                        ContentValues values = new ContentValues();
                        values.put(SiteNavigation.THUMBNAIL, os.toByteArray());
                        Uri uri = ContentUris.withAppendedId(SiteNavigation.SITE_NAVIGATION_URI, cursor.getLong(0));
                        Log.d(LOGTAG, "updateSiteNavigationThumbnail uri is : " + uri);
                        cr.update(uri, values, null, null);
                        os.close();
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "updateSiteNavigationThumbnail", e);
                } finally {
                    if (null != cursor) {
                        cursor.close();
                    }
                }
                return null;
            }
        } .execute();
    }

    private boolean isSiteNavigationAdd(String url) {
        if (url.equals(SiteNavigation.BROWSER_NAVIGATION)) {
            return true;
        }
        return false;
    }
    /*@}*/

    private static boolean isImageViewableUri(Uri uri) {
        String scheme = uri.getScheme();
        for (String allowed : IMAGE_VIEWABLE_SCHEMES) {
            if (allowed.equals(scheme)) {
                return true;
            }
        }
        return false;
    }

    /**
     * As the menu can be open when loading state changes
     * we must manually update the state of the stop/reload menu
     * item
     */
    /* seprate reload and stop menu
    private void updateInLoadMenuItems(Menu menu, Tab tab) {
        if (menu == null) {
            return;
        }
        MenuItem dest = menu.findItem(R.id.stop_reload_menu_id);
        MenuItem src = ((tab != null) && tab.inPageLoad()) ?
                menu.findItem(R.id.stop_menu_id):
                menu.findItem(R.id.reload_menu_id);
        if (src != null) {
            dest.setIcon(src.getIcon());
            dest.setTitle(src.getTitle());
        }
    }
    */

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        /* seprate reload and stop menu
        updateInLoadMenuItems(menu, getCurrentTab());
        */
        // hold on to the menu reference here; it is used by the page callbacks
        // to update the menu based on loading state
        mCachedMenu = menu;
        // Note: setVisible will decide whether an item is visible; while
        // setEnabled() will decide whether an item is enabled, which also means
        // whether the matching shortcut key will function.
        switch (mMenuState) {
            case EMPTY_MENU:
                if (mCurrentMenuState != mMenuState) {
                    menu.setGroupVisible(R.id.MAIN_MENU, false);
                    menu.setGroupEnabled(R.id.MAIN_MENU, false);
                    menu.setGroupEnabled(R.id.MAIN_SHORTCUT_MENU, false);
                }
                break;
            default:
                if (mCurrentMenuState != mMenuState) {
                    menu.setGroupVisible(R.id.MAIN_MENU, true);
                    menu.setGroupEnabled(R.id.MAIN_MENU, true);
                    menu.setGroupEnabled(R.id.MAIN_SHORTCUT_MENU, true);
                }
                updateMenuState(getCurrentTab(), menu);
                break;
        }
        mCurrentMenuState = mMenuState;
        return mUi.onPrepareOptionsMenu(menu);
    }

    @Override
    public void updateMenuState(Tab tab, Menu menu) {
        boolean canGoBack = false;
        boolean canGoForward = false;
        boolean isHome = false;
        boolean isDesktopUa = false;
        boolean isLive = false;
        if (tab != null) {
            canGoBack = tab.canGoBack();
            canGoForward = tab.canGoForward();
            isHome = mSettings.getHomePage().equals(tab.getUrl());
            isDesktopUa = mSettings.hasDesktopUseragent(tab.getWebView());
            /*
             *Add for navigation tab
             *Orignal android code:
             *isLive = !tab.isSnapshot();
             *@{
             */
            boolean isSiteNavigation = false;
            if(tab != null &&  tab.getWebView() != null) {
                isSiteNavigation = SiteNavigation.SITE_NAVIGATION.equals(tab.getWebView().getUrl());
            }
            isLive = !tab.isSnapshot() && !isSiteNavigation;
            /*@}*/
        }
        final MenuItem back = menu.findItem(R.id.back_menu_id);
        back.setEnabled(canGoBack);

        final MenuItem home = menu.findItem(R.id.homepage_menu_id);
        home.setEnabled(!isHome);

        final MenuItem forward = menu.findItem(R.id.forward_menu_id);
        forward.setEnabled(canGoForward);
        /* seprate reload and stop menu
        final MenuItem source = menu.findItem(isInLoad() ? R.id.stop_menu_id
                : R.id.reload_menu_id);
        final MenuItem dest = menu.findItem(R.id.stop_reload_menu_id);
        if (source != null && dest != null) {
            dest.setTitle(source.getTitle());
            dest.setIcon(source.getIcon());
        }
        */
        final MenuItem stop_load = menu.findItem(R.id.stop_menu_id);
        stop_load.setEnabled(isInLoad());
        menu.setGroupVisible(R.id.NAV_MENU, isLive);

        // decide whether to show the share link option
        PackageManager pm = mActivity.getPackageManager();
        Intent send = new Intent(Intent.ACTION_SEND);
        send.setType("text/plain");
        ResolveInfo ri = pm.resolveActivity(send,
                PackageManager.MATCH_DEFAULT_ONLY);
        menu.findItem(R.id.share_page_menu_id).setVisible(ri != null);

        boolean isNavDump = mSettings.enableNavDump();
        final MenuItem nav = menu.findItem(R.id.dump_nav_menu_id);
        nav.setVisible(isNavDump);
        nav.setEnabled(isNavDump);

        boolean showDebugSettings = mSettings.isDebugEnabled();
        final MenuItem uaSwitcher = menu.findItem(R.id.ua_desktop_menu_id);
        uaSwitcher.setChecked(isDesktopUa);

        menu.setGroupVisible(R.id.LIVE_MENU, isLive);
        /**
         * Add save_webpage
         *@{
         */
        final MenuItem saveSnapshot = menu.findItem(R.id.save_snapshot_menu_id);
        saveSnapshot.setEnabled(!isSavePage);
        if (tab != null) {
            saveSnapshot.setVisible(Util.SUPPORT_SAVE_SNAPSHOT && !DataUri.isDataUri(tab.getUrl()) && isLive);
        }
        /*@}*/

        /*
         *Add for navigation tab
         *Orignal android code:
         *menu.setGroupVisible(R.id.SNAPSHOT_MENU, !isLive);
         *@{
         */
        if (tab != null) {
            menu.setGroupVisible(R.id.SNAPSHOT_MENU, tab.isSnapshot());
        }
        /*@}*/
        menu.setGroupVisible(R.id.COMBO_MENU, false);

        mUi.updateMenuState(tab, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (null == getCurrentTopWebView()) {
            return false;
        }
        if (mMenuIsDown) {
            // The shortcut action consumes the MENU. Even if it is still down,
            // it won't trigger the next shortcut action. In the case of the
            // shortcut action triggering a new activity, like Bookmarks, we
            // won't get onKeyUp for MENU. So it is important to reset it here.
            mMenuIsDown = false;
        }
        if (mUi.onOptionsItemSelected(item)) {
            // ui callback handled it
            return true;
        }
        switch (item.getItemId()) {
            // -- Main menu
            case R.id.new_tab_menu_id:
                openTabToHomePage();
                break;

            case R.id.close_other_tabs_id:
                closeOtherTabs();
                break;

            case R.id.goto_menu_id:
                editUrl();
                break;

            case R.id.bookmarks_menu_id:
                bookmarksOrHistoryPicker(ComboViews.Bookmarks);
                break;

            case R.id.history_menu_id:
                bookmarksOrHistoryPicker(ComboViews.History);
                break;

            case R.id.snapshots_menu_id:
                bookmarksOrHistoryPicker(ComboViews.Snapshots);
                break;

            case R.id.add_bookmark_menu_id:
                bookmarkCurrentPage();
                break;
            /* seprate reload and stop menu
            case R.id.stop_reload_menu_id:
                if (isInLoad()) {
                    stopLoading();
                } else {
                    getCurrentTopWebView().reload();
                }
                break;
            */
            case R.id.stop_menu_id:
                stopLoading();
                break;

            case R.id.reload_menu_id:
                if (isInLoad()) {
                    stopLoading();
                }
                getCurrentTopWebView().reload();
                break;

            case R.id.back_menu_id:
                getCurrentTab().goBack();
                break;

            case R.id.forward_menu_id:
                getCurrentTab().goForward();
                break;

            case R.id.close_menu_id:
                // Close the subwindow if it exists.
                if (mTabControl.getCurrentSubWindow() != null) {
                    dismissSubWindow(mTabControl.getCurrentTab());
                    break;
                }
                closeCurrentTab();
                break;

            case R.id.homepage_menu_id:
            case R.id.home_page_id: //SPRD:feature 522253 of menu add homepage menu
                Tab current = mTabControl.getCurrentTab();
                loadUrl(current, mSettings.getHomePage());
                break;

            case R.id.preferences_menu_id:
                openPreferences();
                break;

            case R.id.find_menu_id:
                findOnPage();
                break;

            case R.id.page_info_menu_id:
                showPageInfo();
                break;

            case R.id.snapshot_go_live:
                goLive();
                return true;

            case R.id.share_page_menu_id:
                Tab currentTab = mTabControl.getCurrentTab();
                if (null == currentTab) {
                    return false;
                }
                shareCurrentPage(currentTab);
                break;

            case R.id.dump_nav_menu_id:
                getCurrentTopWebView().debugDump();
                break;

            case R.id.zoom_in_menu_id:
                getCurrentTopWebView().zoomIn();
                break;

            case R.id.zoom_out_menu_id:
                getCurrentTopWebView().zoomOut();
                break;

            case R.id.view_downloads_menu_id:
                viewDownloads();
                break;

            case R.id.ua_desktop_menu_id:
                toggleUserAgent();
                break;
            /*
             * for save_webpage
             *@{
             */
            case R.id.save_snapshot_menu_id:
                saveWebArchive();
                break;
            /*@}*/

            case R.id.window_one_menu_id:
            case R.id.window_two_menu_id:
            case R.id.window_three_menu_id:
            case R.id.window_four_menu_id:
            case R.id.window_five_menu_id:
            case R.id.window_six_menu_id:
            case R.id.window_seven_menu_id:
            case R.id.window_eight_menu_id:
                {
                    int menuid = item.getItemId();
                    for (int id = 0; id < WINDOW_SHORTCUT_ID_ARRAY.length; id++) {
                        if (WINDOW_SHORTCUT_ID_ARRAY[id] == menuid) {
                            Tab desiredTab = mTabControl.getTab(id);
                            if (desiredTab != null &&
                                    desiredTab != mTabControl.getCurrentTab()) {
                                switchToTab(desiredTab);
                            }
                            break;
                        }
                    }
                }
                break;
            //SPRD:feature 522253 of exitmenu menu
            case R.id.exit_menu_id:
                mActivity.finish();
                break;
            //SPRD:feature 522253 of exitmenu menu
            default:
                return false;
        }
        return true;
    }

    @Override
    public void toggleUserAgent() {
        WebView web = getCurrentWebView();
        mSettings.toggleDesktopUseragent(web);
        web.loadUrl(web.getOriginalUrl());
    }

    @Override
    public void findOnPage() {
        getCurrentTopWebView().showFindDialog(null, true);
    }

    @Override
    public void openPreferences() {
        Intent intent = new Intent(mActivity, BrowserPreferencesPage.class);
        intent.putExtra(BrowserPreferencesPage.CURRENT_PAGE,
                getCurrentTopWebView().getUrl());
        mActivity.startActivityForResult(intent, PREFERENCES_PAGE);
    }

    @Override
    public void bookmarkCurrentPage() {
        Intent bookmarkIntent = createBookmarkCurrentPageIntent(false);
        if (bookmarkIntent != null) {
            mActivity.startActivity(bookmarkIntent);
        }
    }

    private void goLive() {
        Tab t = getCurrentTab();
        t.loadUrl(t.getUrl(), null);
    }

    @Override
    public void showPageInfo() {
        mPageDialogsHandler.showPageInfo(mTabControl.getCurrentTab(), false, null);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        // Let the History and Bookmark fragments handle menus they created.
        if (item.getGroupId() == R.id.CONTEXT_MENU) {
            return false;
        }

        int id = item.getItemId();
        boolean result = true;
        switch (id) {
            // -- Browser context menu
            case R.id.open_context_menu_id:
            case R.id.save_link_context_menu_id:
            case R.id.copy_link_context_menu_id:
            /**
              *SPRD:feature 522253 of  bookmark link menu:
              *@{
              */
            case R.id.bookmark_context_menu_id:
              /**
              *@}
            */
                final WebView webView = getCurrentTopWebView();
                if (null == webView) {
                    result = false;
                    break;
                }
                final HashMap<String, WebView> hrefMap =
                        new HashMap<String, WebView>();
                hrefMap.put("webview", webView);
                final Message msg = mHandler.obtainMessage(
                        FOCUS_NODE_HREF, id, 0, hrefMap);
                webView.requestFocusNodeHref(msg);
                break;
            default:
                // For other context menus
                result = onOptionsItemSelected(item);
        }
        return result;
    }

    /**
     * support programmatically opening the context menu
     */
    public void openContextMenu(View view) {
        mActivity.openContextMenu(view);
    }

    /**
     * programmatically open the options menu
     */
    public void openOptionsMenu() {
        mActivity.openOptionsMenu();
    }

    @Override
    public boolean onMenuOpened(int featureId, Menu menu) {
        if (mOptionsMenuOpen) {
            if (mConfigChanged) {
                // We do not need to make any changes to the state of the
                // title bar, since the only thing that happened was a
                // change in orientation
                mConfigChanged = false;
            } else {
                if (!mExtendedMenuOpen) {
                    mExtendedMenuOpen = true;
                    mUi.onExtendedMenuOpened();
                } else {
                    // Switching the menu back to icon view, so show the
                    // title bar once again.
                    mExtendedMenuOpen = false;
                    mUi.onExtendedMenuClosed(isInLoad());
                }
            }
        } else {
            // The options menu is closed, so open it, and show the title
            mOptionsMenuOpen = true;
            mConfigChanged = false;
            mExtendedMenuOpen = false;
            mUi.onOptionsMenuOpened();
        }
        return true;
    }

    @Override
    public void onOptionsMenuClosed(Menu menu) {
        mOptionsMenuOpen = false;
        mUi.onOptionsMenuClosed(isInLoad());
    }

    @Override
    public void onContextMenuClosed(Menu menu) {
        mUi.onContextMenuClosed(menu, isInLoad());
    }

    // Helper method for getting the top window.
    @Override
    public WebView getCurrentTopWebView() {
        return mTabControl.getCurrentTopWebView();
    }

    @Override
    public WebView getCurrentWebView() {
        return mTabControl.getCurrentWebView();
    }

    /*
     * This method is called as a result of the user selecting the options
     * menu to see the download window. It shows the download window on top of
     * the current window.
     */
    void viewDownloads() {
        Intent intent = new Intent(DownloadManager.ACTION_VIEW_DOWNLOADS);
        mActivity.startActivity(intent);
    }

    int getActionModeHeight() {
        TypedArray actionBarSizeTypedArray = mActivity.obtainStyledAttributes(
                    new int[] { android.R.attr.actionBarSize });
        int size = (int) actionBarSizeTypedArray.getDimension(0, 0f);
        actionBarSizeTypedArray.recycle();
        return size;
    }

    // action mode

    @Override
    public void onActionModeStarted(ActionMode mode) {
        mUi.onActionModeStarted(mode);
        mActionMode = mode;
    }

    /*
     * True if a custom ActionMode (i.e. find or select) is in use.
     */
    @Override
    public boolean isInCustomActionMode() {
        return mActionMode != null;
    }

    /*
     * End the current ActionMode.
     */
    @Override
    public void endActionMode() {
        if (mActionMode != null) {
            mActionMode.finish();
        }
    }

    /*
     * Called by find and select when they are finished.  Replace title bars
     * as necessary.
     */
    @Override
    public void onActionModeFinished(ActionMode mode) {
        if (!isInCustomActionMode()) return;
        mUi.onActionModeFinished(isInLoad());
        mActionMode = null;
    }

    boolean isInLoad() {
        final Tab tab = getCurrentTab();
        return (tab != null) && tab.inPageLoad();
    }

    // bookmark handling

    /**
     * add the current page as a bookmark to the given folder id
     * @param folderId use -1 for the default folder
     * @param editExisting If true, check to see whether the site is already
     *          bookmarked, and if it is, edit that bookmark.  If false, and
     *          the site is already bookmarked, do not attempt to edit the
     *          existing bookmark.
     */
    @Override
    public Intent createBookmarkCurrentPageIntent(boolean editExisting) {
        WebView w = getCurrentTopWebView();
        if (w == null) {
            return null;
        }
        Intent i = new Intent(mActivity,
                AddBookmarkPage.class);
        i.putExtra(BrowserContract.Bookmarks.URL, w.getUrl());
        i.putExtra(BrowserContract.Bookmarks.TITLE, w.getTitle());
        String touchIconUrl = w.getTouchIconUrl();
        if (touchIconUrl != null) {
            i.putExtra(AddBookmarkPage.TOUCH_ICON_URL, touchIconUrl);
            WebSettings settings = w.getSettings();
            if (settings != null) {
                i.putExtra(AddBookmarkPage.USER_AGENT,
                        settings.getUserAgentString());
            }
        }
        /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 start */
        i.putExtra(BrowserContract.Bookmarks.THUMBNAIL,
                Util.compressImage(createScreenshot(w, getDesiredThumbnailWidth(mActivity),
                getDesiredThumbnailHeight(mActivity))));
        i.putExtra(BrowserContract.Bookmarks.FAVICON, Util.compressImage(w.getFavicon()));
        /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 end */
        if (editExisting) {
            i.putExtra(AddBookmarkPage.CHECK_FOR_DUPE, true);
        }
        // Put the dialog at the upper right of the screen, covering the
        // star on the title bar.
        i.putExtra("gravity", Gravity.RIGHT | Gravity.TOP);
        return i;
    }

    // file chooser
    @Override
    public void showFileChooser(ValueCallback<Uri[]> callback, FileChooserParams params) {
        mUploadHandler = new UploadHandler(this);
        mUploadHandler.openFileChooser(callback, params);
    }

    // thumbnails

    /**
     * Return the desired width for thumbnail screenshots, which are stored in
     * the database, and used on the bookmarks screen.
     * @param context Context for finding out the density of the screen.
     * @return desired width for thumbnail screenshot.
     */
    static int getDesiredThumbnailWidth(Context context) {
        return context.getResources().getDimensionPixelOffset(
                R.dimen.bookmarkThumbnailWidth);
    }

    /**
     * Return the desired height for thumbnail screenshots, which are stored in
     * the database, and used on the bookmarks screen.
     * @param context Context for finding out the density of the screen.
     * @return desired height for thumbnail screenshot.
     */
    static int getDesiredThumbnailHeight(Context context) {
        return context.getResources().getDimensionPixelOffset(
                R.dimen.bookmarkThumbnailHeight);
    }

    static Bitmap createScreenshot(WebView view, int width, int height) {
        if (view == null || view.getContentHeight() == 0
                || view.getContentWidth() == 0) {
            return null;
        }
        // We render to a bitmap 2x the desired size so that we can then
        // re-scale it with filtering since canvas.scale doesn't filter
        // This helps reduce aliasing at the cost of being slightly blurry
        final int filter_scale = 2;
        int scaledWidth = width * filter_scale;
        int scaledHeight = height * filter_scale;
        if (sThumbnailBitmap == null || sThumbnailBitmap.getWidth() != scaledWidth
                || sThumbnailBitmap.getHeight() != scaledHeight) {
            if (sThumbnailBitmap != null) {
                sThumbnailBitmap.recycle();
                sThumbnailBitmap = null;
            }
            sThumbnailBitmap =
                    Bitmap.createBitmap(scaledWidth, scaledHeight, Bitmap.Config.RGB_565);
        }
        Canvas canvas = new Canvas(sThumbnailBitmap);
        int contentWidth = view.getContentWidth();
        float overviewScale = scaledWidth / (view.getScale() * contentWidth);
        if (view instanceof BrowserWebView) {
            //modify for thumbnail blank issue
            int dx = -((BrowserWebView)view).getScrollX();
            int dy = -((BrowserWebView)view).getTitleHeight() - ((BrowserWebView)view).getScrollY();
            canvas.translate(dx * overviewScale, dy * overviewScale);
        }

        canvas.scale(overviewScale, overviewScale);

        if (view instanceof BrowserWebView) {
            ((BrowserWebView)view).drawContent(canvas);
        } else {
            view.draw(canvas);
        }
        Bitmap ret = Bitmap.createScaledBitmap(sThumbnailBitmap,
                width, height, true);
        canvas.setBitmap(null);
        return ret;
    }

    private void updateScreenshot(Tab tab) {
        // If this is a bookmarked site, add a screenshot to the database.
        // FIXME: Would like to make sure there is actually something to
        // draw, but the API for that (WebViewCore.pictureReady()) is not
        // currently accessible here.

        WebView view = tab.getWebView();
        if (view == null) {
            // Tab was destroyed
            return;
        }
        final String url = tab.getUrl();
        final String originalUrl = view.getOriginalUrl();
        if (TextUtils.isEmpty(url)) {
            return;
        }

        /**
         * Add for navigation tab
         *@{
         */
        if (Util.BROWSER_SITE_NAVIGATION_SUPPORT) {
            String tempUrl = originalUrl;
            if (tempUrl == null) {
                tempUrl = url;
            }
            if (Patterns.WEB_URL.matcher(tempUrl).matches()) {
                // Check if it a url existed in site navigation
                boolean isSNUrl = SiteNavigationAddDialog.isSiteNavigationUrl(mActivity, tempUrl);
                if (isSNUrl) {
                    updateSiteNavigationThumbnail(tempUrl, view);
                }
            }
        }
        /*@}*/

        // Only update thumbnails for web urls (http(s)://), not for
        // about:, javascript:, data:, etc...
        // Unless it is a bookmarked site, then always update
        if (!Util.isMatches(Patterns.WEB_URL.matcher(url)) && !tab.isBookmarkedSite()) {
            return;
        }

        final Bitmap bm = createScreenshot(view, getDesiredThumbnailWidth(mActivity),
                getDesiredThumbnailHeight(mActivity));
        if (bm == null) {
            return;
        }

        final ContentResolver cr = mActivity.getContentResolver();
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... unused) {
                Cursor cursor = null;
                try {
                    // TODO: Clean this up
                    cursor = Bookmarks.queryCombinedForUrl(cr, originalUrl, url);
                    if (cursor != null && cursor.moveToFirst()) {
                        final ByteArrayOutputStream os =
                                new ByteArrayOutputStream();
                        /*SPRD:compress the icon in case it is too large begin*/
                        Util.compressImage(bm).compress(Bitmap.CompressFormat.PNG, 100, os);
                        /*SPRD:compress the icon in case it is too large end*/

                        ContentValues values = new ContentValues();
                        values.put(Images.THUMBNAIL, os.toByteArray());

                        do {
                            values.put(Images.URL, cursor.getString(0));
                            cr.update(Images.CONTENT_URI, values, null, null);
                        } while (cursor.moveToNext());
                    }
                } catch (Exception s) {
                    // Added for possible error when user tries to remove the same bookmark
                    // that is being updated with a screen shot
                    Log.w(LOGTAG, "Error when running updateScreenshot ", s);
                } finally {
                    if (cursor != null) cursor.close();
                }
                return null;
            }
        }.execute();
    }

    private class Copy implements OnMenuItemClickListener {
        private CharSequence mText;

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            copy(mText);
            return true;
        }

        public Copy(CharSequence toCopy) {
            mText = toCopy;
        }
    }

    private static class Download implements OnMenuItemClickListener {
        /*
         * for download_storage_save_path
         *@{
         */
        private Controller mController;
        /*@}*/
        private Activity mActivity;
        private String mText;
        private boolean mPrivateBrowsing;
        private String mUserAgent;
        private static final String FALLBACK_EXTENSION = "dat";
        private static final String IMAGE_BASE_FORMAT = "yyyy-MM-dd-HH-mm-ss-";

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            /*
             * for download_storage_save_path
             *@{
             */
            if(Util.SUPPORT_SELECT_DOWNLOAD_PATH){
                DownloadHandler.onDownloadStart(mController, mActivity, mText, null, null, null, mPrivateBrowsing,false,true);
            }else{
                if (DataUri.isDataUri(mText)) {
                    saveDataUri();
                } else {
                    DownloadHandler.onDownloadStartNoStream(mActivity, mText, mUserAgent,
                            null, null, null, mPrivateBrowsing);
                }
            }
            /*@}*/
            return true;
        }

        public Download(Activity activity, String toDownload, boolean privateBrowsing,
                String userAgent) {
            mActivity = activity;
            mText = toDownload;
            mPrivateBrowsing = privateBrowsing;
            mUserAgent = userAgent;
        }

        /*
         * for download_storage_save_path
         *@{
         */
        public Download(Controller controller,Activity activity, String toDownload, boolean privateBrowsing, String userAgent) {
            mController = controller;
            mActivity = activity;
            mText = toDownload;
            mPrivateBrowsing = privateBrowsing;
            mUserAgent = userAgent;
        }
        /*@}*/

        /**
         * Treats mText as a data URI and writes its contents to a file
         * based on the current time.
         */
        private void saveDataUri() {
            FileOutputStream outputStream = null;
            try {
                DataUri uri = new DataUri(mText);
                File target = getTarget(uri);
                outputStream = new FileOutputStream(target);
                outputStream.write(uri.getData());
                final DownloadManager manager =
                        (DownloadManager) mActivity.getSystemService(Context.DOWNLOAD_SERVICE);
                 manager.addCompletedDownload(target.getName(),
                        mActivity.getTitle().toString(), true,
                        uri.getMimeType(), target.getAbsolutePath(),
                        uri.getData().length, true);
            } catch (IOException e) {
                Log.e(LOGTAG, "Could not save data URL");
            } finally {
                if (outputStream != null) {
                    try {
                        outputStream.close();
                    } catch (IOException e) {
                        // ignore close errors
                    }
                }
            }
        }

        /**
         * Creates a File based on the current time stamp and uses
         * the mime type of the DataUri to get the extension.
         */
        private File getTarget(DataUri uri) throws IOException {
            File dir = mActivity.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
            DateFormat format = new SimpleDateFormat(IMAGE_BASE_FORMAT, Locale.US);
            String nameBase = format.format(new Date());
            String mimeType = uri.getMimeType();
            MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
            String extension = mimeTypeMap.getExtensionFromMimeType(mimeType);
            if (extension == null) {
                Log.w(LOGTAG, "Unknown mime type in data URI" + mimeType);
                extension = FALLBACK_EXTENSION;
            }
            extension = "." + extension; // createTempFile needs the '.'
            File targetFile = File.createTempFile(nameBase, extension, dir);
            return targetFile;
        }
    }

    /********************** TODO: UI stuff *****************************/

    // these methods have been copied, they still need to be cleaned up

    /****************** tabs ***************************************************/

    // basic tab interactions:

    // it is assumed that tabcontrol already knows about the tab
    protected void addTab(Tab tab) {
        mUi.addTab(tab);
    }

    protected void removeTab(Tab tab) {
        mUi.removeTab(tab);
        mTabControl.removeTab(tab);
        mCrashRecoveryHandler.backupState();
    }

    @Override
    public void setActiveTab(Tab tab) {
        // monkey protection against delayed start
        if (tab != null) {
            mTabControl.setCurrentTab(tab);
            // the tab is guaranteed to have a webview after setCurrentTab
            mUi.setActiveTab(tab);

            /**
             *Add for bottom bar
             *@{
             */
            if (tab != null && tab.inForeground()) {
                updateMenuBarState();
            }
            /*@}*/
        }
    }

    protected void closeEmptyTab() {
        Tab current = mTabControl.getCurrentTab();
        if (current != null
                && current.getWebView().copyBackForwardList().getSize() == 0) {
            closeCurrentTab();
        }
    }

    protected void reuseTab(Tab appTab, UrlData urlData) {
        // Dismiss the subwindow if applicable.
        dismissSubWindow(appTab);
        // Since we might kill the WebView, remove it from the
        // content view first.
        mUi.detachTab(appTab);
        // Recreate the main WebView after destroying the old one.
        mTabControl.recreateWebView(appTab);
        // TODO: analyze why the remove and add are necessary
        mUi.attachTab(appTab);
        if (mTabControl.getCurrentTab() != appTab) {
            switchToTab(appTab);
            loadUrlDataIn(appTab, urlData);
        } else {
            // If the tab was the current tab, we have to attach
            // it to the view system again.
            setActiveTab(appTab);
            loadUrlDataIn(appTab, urlData);
        }
    }

    // Remove the sub window if it exists. Also called by TabControl when the
    // user clicks the 'X' to dismiss a sub window.
    @Override
    public void dismissSubWindow(Tab tab) {
        removeSubWindow(tab);
        // dismiss the subwindow. This will destroy the WebView.
        tab.dismissSubWindow();
        WebView wv = getCurrentTopWebView();
        if (wv != null) {
            wv.requestFocus();
        }
    }

    @Override
    public void removeSubWindow(Tab t) {
        if (t.getSubWebView() != null) {
            mUi.removeSubWindow(t.getSubViewContainer());
        }
    }

    @Override
    public void attachSubWindow(Tab tab) {
        if (tab.getSubWebView() != null) {
            mUi.attachSubWindow(tab.getSubViewContainer());
            getCurrentTopWebView().requestFocus();
        }
    }

    private Tab showPreloadedTab(final UrlData urlData) {
        if (!urlData.isPreloaded()) {
            return null;
        }
        final PreloadedTabControl tabControl = urlData.getPreloadedTab();
        final String sbQuery = urlData.getSearchBoxQueryToSubmit();
        if (sbQuery != null) {
            if (!tabControl.searchBoxSubmit(sbQuery, urlData.mUrl, urlData.mHeaders)) {
                // Could not submit query. Fallback to regular tab creation
                tabControl.destroy();
                return null;
            }
        }
        // check tab count and make room for new tab
        if (!mTabControl.canCreateNewTab()) {
            Tab leastUsed = mTabControl.getLeastUsedTab(getCurrentTab());
            if (leastUsed != null) {
                closeTab(leastUsed);
            }
        }
        Tab t = tabControl.getTab();
        t.refreshIdAfterPreload();
        mTabControl.addPreloadedTab(t);
        addTab(t);
        setActiveTab(t);
        return t;
    }

    // open a non inconito tab with the given url data
    // and set as active tab
    public Tab openTab(UrlData urlData) {
        Tab tab = showPreloadedTab(urlData);
        if (tab == null) {
            tab = createNewTab(false, true, true);
            if ((tab != null) && !urlData.isEmpty()) {
                loadUrlDataIn(tab, urlData);
            }
        }
        return tab;
    }

    @Override
    public Tab openTabToHomePage() {
        return openTab(mSettings.getHomePage(), false, true, false);
    }

    @Override
    public Tab openIncognitoTab() {
        return openTab(INCOGNITO_URI, true, true, false);
    }

    @Override
    public Tab openTab(String url, boolean incognito, boolean setActive,
            boolean useCurrent) {
        return openTab(url, incognito, setActive, useCurrent, null);
    }

    @Override
    public Tab openTab(String url, Tab parent, boolean setActive,
            boolean useCurrent) {
        return openTab(url, (parent != null) && parent.isPrivateBrowsingEnabled(),
                setActive, useCurrent, parent);
    }

    public Tab openTab(String url, boolean incognito, boolean setActive,
            boolean useCurrent, Tab parent) {
        Tab tab = createNewTab(incognito, setActive, useCurrent);
        if (tab != null) {
            /**
              *Add for save_webpage
              *Orignal android code:
              *if (parent != null && parent != tab) {
              *@{
              */
            if (parent != null && parent != tab && !parent.isSnapshot()) {
            /*@}*/
                parent.addChildTab(tab);
            }
            if (url != null) {
                loadUrl(tab, url);
            }
        }
        return tab;
    }

    // this method will attempt to create a new tab
    // incognito: private browsing tab
    // setActive: ste tab as current tab
    // useCurrent: if no new tab can be created, return current tab
    private Tab createNewTab(boolean incognito, boolean setActive,
            boolean useCurrent) {
        Tab tab = null;
        if (mTabControl.canCreateNewTab()) {
            tab = mTabControl.createNewTab(incognito);
            addTab(tab);
            if (setActive) {
                setActiveTab(tab);
            }
        } else {
            if (useCurrent) {
                tab = mTabControl.getCurrentTab();
                reuseTab(tab, null);
            } else {
                mUi.showMaxTabsWarning();
            }
        }
        return tab;
    }

    /**
     * @param tab the tab to switch to
     * @return boolean True if we successfully switched to a different tab.  If
     *                 the indexth tab is null, or if that tab is the same as
     *                 the current one, return false.
     */
    @Override
    public boolean switchToTab(Tab tab) {
        Tab currentTab = mTabControl.getCurrentTab();
        if (tab == null || tab == currentTab) {
            return false;
        }
        setActiveTab(tab);
        return true;
    }

    @Override
    public void closeCurrentTab() {
        closeCurrentTab(false);
    }

    protected void closeCurrentTab(boolean andQuit) {
        if (mTabControl.getTabCount() == 1) {
            mCrashRecoveryHandler.clearState();
            mTabControl.removeTab(getCurrentTab());
            mActivity.finish();
            return;
        }
        final Tab current = mTabControl.getCurrentTab();
        final int pos = mTabControl.getCurrentPosition();
        Tab newTab = current.getParent();
        if (newTab == null) {
            newTab = mTabControl.getTab(pos + 1);
            if (newTab == null) {
                newTab = mTabControl.getTab(pos - 1);
            }
        }
        if (andQuit) {
            mTabControl.setCurrentTab(newTab);
            closeTab(current);
        } else if (switchToTab(newTab)) {
            // Close window
            closeTab(current);
        }
    }

    /**
     * Close the tab, remove its associated title bar, and adjust mTabControl's
     * current tab to a valid value.
     */
    @Override
    public void closeTab(Tab tab) {
        if (tab == mTabControl.getCurrentTab()) {
            closeCurrentTab();
        } else {
            removeTab(tab);
        }
    }

    /**
     * Close all tabs except the current one
     */
    @Override
    public void closeOtherTabs() {
        int inactiveTabs = mTabControl.getTabCount() - 1;
        for (int i = inactiveTabs; i >= 0; i--) {
            Tab tab = mTabControl.getTab(i);
            if (tab != mTabControl.getCurrentTab()) {
                removeTab(tab);
            }
        }
    }

    // Called when loading from context menu or LOAD_URL message
    protected void loadUrlFromContext(String url) {
        Tab tab = getCurrentTab();
        WebView view = tab != null ? tab.getWebView() : null;
        // In case the user enters nothing.
        if (url != null && url.length() != 0 && tab != null && view != null) {
            url = UrlUtils.smartUrlFilter(url);
            if (!((BrowserWebView) view).getWebViewClient().
                    shouldOverrideUrlLoading(view, url)) {
                loadUrl(tab, url);
            }
        }
    }

    /**
     * Load the URL into the given WebView and update the title bar
     * to reflect the new load.  Call this instead of WebView.loadUrl
     * directly.
     * @param view The WebView used to load url.
     * @param url The URL to load.
     */
    @Override
    public void loadUrl(Tab tab, String url) {
        loadUrl(tab, url, null);
    }

    protected void loadUrl(Tab tab, String url, Map<String, String> headers) {
        if (tab != null) {
            dismissSubWindow(tab);
            tab.loadUrl(url, headers);
            mUi.onProgressChanged(tab);
        }
    }

    /**
     * Load UrlData into a Tab and update the title bar to reflect the new
     * load.  Call this instead of UrlData.loadIn directly.
     * @param t The Tab used to load.
     * @param data The UrlData being loaded.
     */
    protected void loadUrlDataIn(Tab t, UrlData data) {
        if (data != null) {
            if (data.isPreloaded()) {
                // this isn't called for preloaded tabs
            } else {
                if (t != null && data.mDisableUrlOverride) {
                    t.disableUrlOverridingForLoad();
                }
                loadUrl(t, data.mUrl, data.mHeaders);
            }
        }
    }

    @Override
    public void onUserCanceledSsl(Tab tab) {
        // TODO: Figure out the "right" behavior
        if (tab.canGoBack()) {
            tab.goBack();
        } else {
            tab.loadUrl(mSettings.getHomePage(), null);
        }
    }

    void goBackOnePageOrQuit() {
        Tab current = mTabControl.getCurrentTab();
        if (current == null) {
            /*
             * Instead of finishing the activity, simply push this to the back
             * of the stack and let ActivityManager to choose the foreground
             * activity. As BrowserActivity is singleTask, it will be always the
             * root of the task. So we can use either true or false for
             * moveTaskToBack().
             */
            mActivity.moveTaskToBack(true);
            return;
        }
        if (current.canGoBack()) {
            current.goBack();
        } else {
            // Check to see if we are closing a window that was created by
            // another window. If so, we switch back to that window.
            Tab parent = current.getParent();
            if (parent != null) {
                switchToTab(parent);
                // Now we close the other tab
                closeTab(current);
            } else {
                if ((current.getAppId() != null) || current.closeOnBack()) {
                    closeCurrentTab(true);
                /* Add for Bug:496801 Close current tab 2015.11.10 start */
                } else {
                    if (mTabControl.getTabCount() > 1) {
                        closeCurrentTab(false);
                        return;
                    }
                /* Add for Bug:496801 Close current tab 2015.11.10 end */
                }
                /*
                 * Instead of finishing the activity, simply push this to the back
                 * of the stack and let ActivityManager to choose the foreground
                 * activity. As BrowserActivity is singleTask, it will be always the
                 * root of the task. So we can use either true or false for
                 * moveTaskToBack().
                 */
                if (!mActivity.moveTaskToBack(true) && mTabControl.getTabCount() <= 0) {
                    Log.d(LOGTAG,"goBackOnePageOrQuit,not go back and tab count is 0");
                    openTab(BLANK_URL, false, true, false);
                }
            }
        }
    }

    /**
     * helper method for key handler
     * returns the current tab if it can't advance
     */
    private Tab getNextTab() {
        int pos = mTabControl.getCurrentPosition() + 1;
        if (pos >= mTabControl.getTabCount()) {
            pos = 0;
        }
        return mTabControl.getTab(pos);
    }

    /**
     * helper method for key handler
     * returns the current tab if it can't advance
     */
    private Tab getPrevTab() {
        int pos  = mTabControl.getCurrentPosition() - 1;
        if ( pos < 0) {
            pos = mTabControl.getTabCount() - 1;
        }
        return  mTabControl.getTab(pos);
    }

    boolean isMenuOrCtrlKey(int keyCode) {
        return (KeyEvent.KEYCODE_MENU == keyCode)
                || (KeyEvent.KEYCODE_CTRL_LEFT == keyCode)
                || (KeyEvent.KEYCODE_CTRL_RIGHT == keyCode);
    }

    /**
     * handle key events in browser
     *
     * @param keyCode
     * @param event
     * @return true if handled, false to pass to super
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean noModifiers = event.hasNoModifiers();
        // Even if MENU is already held down, we need to call to super to open
        // the IME on long press.
        if (!noModifiers && isMenuOrCtrlKey(keyCode)) {
            mMenuIsDown = true;
            return false;
        }

        WebView webView = getCurrentTopWebView();
        Tab tab = getCurrentTab();
        if (webView == null || tab == null) return false;

        boolean ctrl = event.hasModifiers(KeyEvent.META_CTRL_ON);
        boolean shift = event.hasModifiers(KeyEvent.META_SHIFT_ON);

        switch(keyCode) {
            case KeyEvent.KEYCODE_TAB:
                if (event.isCtrlPressed()) {
                    if (event.isShiftPressed()) {
                        // prev tab
                        switchToTab(getPrevTab());
                    } else {
                        // next tab
                        switchToTab(getNextTab());
                    }
                    return true;
                }
                break;
            case KeyEvent.KEYCODE_SPACE:
                // WebView/WebTextView handle the keys in the KeyDown. As
                // the Activity's shortcut keys are only handled when WebView
                // doesn't, have to do it in onKeyDown instead of onKeyUp.
                if (shift) {
                    pageUp();
                } else if (noModifiers) {
                    pageDown();
                }
                return true;
            case KeyEvent.KEYCODE_BACK:
                if (!noModifiers) break;
                event.startTracking();
                return true;
            case KeyEvent.KEYCODE_FORWARD:
                if (!noModifiers) break;
                tab.goForward();
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                if (ctrl) {
                    tab.goBack();
                    return true;
                }
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                if (ctrl) {
                    tab.goForward();
                    return true;
                }
                break;
//          case KeyEvent.KEYCODE_B:    // menu
//          case KeyEvent.KEYCODE_D:    // menu
//          case KeyEvent.KEYCODE_E:    // in Chrome: puts '?' in URL bar
//          case KeyEvent.KEYCODE_F:    // menu
//          case KeyEvent.KEYCODE_G:    // in Chrome: finds next match
//          case KeyEvent.KEYCODE_H:    // menu
//          case KeyEvent.KEYCODE_I:    // unused
//          case KeyEvent.KEYCODE_J:    // menu
//          case KeyEvent.KEYCODE_K:    // in Chrome: puts '?' in URL bar
//          case KeyEvent.KEYCODE_L:    // menu
//          case KeyEvent.KEYCODE_M:    // unused
//          case KeyEvent.KEYCODE_N:    // in Chrome: new window
//          case KeyEvent.KEYCODE_O:    // in Chrome: open file
//          case KeyEvent.KEYCODE_P:    // in Chrome: print page
//          case KeyEvent.KEYCODE_Q:    // unused
//          case KeyEvent.KEYCODE_R:
//          case KeyEvent.KEYCODE_S:    // in Chrome: saves page
            case KeyEvent.KEYCODE_T:
                // we can't use the ctrl/shift flags, they check for
                // exclusive use of a modifier
                if (event.isCtrlPressed()) {
                    if (event.isShiftPressed()) {
                        openIncognitoTab();
                    } else {
                        openTabToHomePage();
                    }
                    return true;
                }
                break;
//          case KeyEvent.KEYCODE_U:    // in Chrome: opens source of page
//          case KeyEvent.KEYCODE_V:    // text view intercepts to paste
//          case KeyEvent.KEYCODE_W:    // menu
//          case KeyEvent.KEYCODE_X:    // text view intercepts to cut
//          case KeyEvent.KEYCODE_Y:    // unused
//          case KeyEvent.KEYCODE_Z:    // unused
        }
        // it is a regular key and webview is not null
         return mUi.dispatchKey(keyCode, event);
    }

    /* SPRD:bug622704 modify begin @{ */
    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        /*switch(keyCode) {
        case KeyEvent.KEYCODE_BACK:
            if (mUi.isWebShowing()) {
                bookmarksOrHistoryPicker(ComboViews.History);
                return true;
            }
            break;
        }*/
        return false;
    }
    /* @} */

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (isMenuOrCtrlKey(keyCode)) {
            mMenuIsDown = false;
            if (KeyEvent.KEYCODE_MENU == keyCode
                    && event.isTracking() && !event.isCanceled()) {
                return onMenuKey();
            }
        }
        if (!event.hasNoModifiers()) return false;
        switch(keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (event.isTracking() && !event.isCanceled()) {
                    onBackKey();
                    return true;
                }
                break;
        }
        return false;
    }

    public boolean isMenuDown() {
        return mMenuIsDown;
    }

    @Override
    public boolean onSearchRequested() {
        mUi.editUrl(false, true);
        return true;
    }

    @Override
    public boolean shouldCaptureThumbnails() {
        return mUi.shouldCaptureThumbnails();
    }

    @Override
    public boolean supportsVoice() {
        PackageManager pm = mActivity.getPackageManager();
        List activities = pm.queryIntentActivities(new Intent(
                RecognizerIntent.ACTION_RECOGNIZE_SPEECH), 0);
        return activities.size() != 0;
    }

    @Override
    public void startVoiceRecognizer() {
        Intent voice = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        voice.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        voice.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1);
        mActivity.startActivityForResult(voice, VOICE_RESULT);
    }

    @Override
    public void setBlockEvents(boolean block) {
        mBlockEvents = block;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mBlockEvents;
    }

    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event) {
        return mBlockEvents;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        return mBlockEvents;
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent ev) {
        return mBlockEvents;
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        return mBlockEvents;
    }

    /**
     * Add for horizontal screen
     *@{
     */
    public boolean isHorizontal(){
        return mSettings.isHorizontal();
    }

    public void updateHorizontalToUnspecified() {
        if (!isHorizontal()) {
            mActivity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        }
    }
    /*@}*/
    /*
     * for download_storage_save_path
     *@{
     */
    public String getDownloadPath(){
        if(mDownloadPathStr != null){
            return mDownloadPathStr;
        }
        String[] SAVEPATH_PROJECTION = new String[] {"_id", "savepath"};
        Cursor savePathInfo = null;
        String tmpDbSavepath = "";
        try {
            savePathInfo = mActivity.getContentResolver().query(Uri.parse("content://browser/filesavepath"),SAVEPATH_PROJECTION, "", new String[]{},null);
            if(null != savePathInfo){
                if (savePathInfo.moveToNext()) {
                    tmpDbSavepath = savePathInfo.getString(1);
                }
            }
        } catch (SQLiteException e) {
            Log.e(LOGTAG, "getDownloadPath: " + e);
        } finally {
            if(null != savePathInfo){
                savePathInfo.close();
            }
        }

        if(!StorageUtils.checkStoragePathAvailable(tmpDbSavepath)){
            String path = StorageUtils.getDefaultStoragePath();
            /*
            ContentValues argsSavepath = new ContentValues();
            argsSavepath.put("savepath", path);
            mActivity.getContentResolver().update(Uri.parse("content://browser/filesavepath"), argsSavepath, "_id = 1", null);
            */
            tmpDbSavepath = path;
        }
        if(tmpDbSavepath!= null && !tmpDbSavepath.endsWith("/")){
            tmpDbSavepath = tmpDbSavepath+"/";
        }
        Log.i(LOGTAG, "getDownloadPath tmpDbSavepath="+tmpDbSavepath);
        return tmpDbSavepath;
    }

    public void onDownloadStartSavePath(String url, String userAgent, String contentDisposition, String mimetype, boolean privateBrowsing,final boolean closeOnExit){
        Log.d(LOGTAG,"mDownloadDialog== " + mDownloadDialog);
        closeDialogMessage();
        mDownloadPathStr = null;
        final String mUrl = url;
        final String mUserAgent = userAgent;
        final String mContentDisposition = contentDisposition;
        final String mMimeType = mimetype;
        LayoutInflater factory = LayoutInflater.from(mActivity);
        final View certificateView = factory.inflate(R.layout.select_download_path, null);
        final boolean mPrivateBrowsing = privateBrowsing;
        mDownloadPath = (EditText) certificateView.findViewById(R.id.download_path_edit);
        Button mSelectPath = (Button) certificateView.findViewById(R.id.select_path);
        Button mDownload = (Button) certificateView.findViewById(R.id.select_path_ok);
        Button mCancel = (Button) certificateView.findViewById(R.id.select_path_cancel);
        mDownloadPath.setText(getDownloadPath());
        mSelectPath.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                selectDownloadStorage();
            }
        });
        mDownload.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.d(LOGTAG,"mUrl== "+mUrl);
                if (DataUri.isDataUri(mUrl)) {
                    String path = saveDataUri(mUrl, mDownloadPath.getText().toString());
                    if(path != null){
                        path = mActivity.getString(R.string.download_complete) +path;
                        Toast.makeText(mActivity, path, Toast.LENGTH_SHORT).show();
                    }else{
                        Toast.makeText(mActivity, mActivity.getString(R.string.download_failure), Toast.LENGTH_SHORT).show();
                    }
                }else{
                    if(mDownloadPath != null && mDownloadPath.getText().length() == 0){
                        Toast.makeText(mActivity, R.string.download_path_null, Toast.LENGTH_SHORT).show();
                    }
                    if(Util.SUPPORT_OVERLAY_DOWNLOAD_FILE){
                        checkDownloadFileIsExists(Controller.this,mActivity, mUrl, mUserAgent, mContentDisposition, mMimeType, mPrivateBrowsing, closeOnExit);
                    }else{
                        DownloadHandler.onDownloadStartNoStream(Controller.this, mActivity, mUrl, mUserAgent, mContentDisposition, mMimeType, mPrivateBrowsing, closeOnExit);
                    }
                }
                closeDialogMessage();
            }

            private String saveDataUri(String url,String path) {
                FileOutputStream outputStream = null;
                File target = null;
                try {
                    DataUri uri = new DataUri(url);
                    target = getTarget(uri, path);
                    outputStream = new FileOutputStream(target);
                    outputStream.write(uri.getData());
                    final DownloadManager manager =
                            (DownloadManager) mActivity.getSystemService(Context.DOWNLOAD_SERVICE);
                    manager.addCompletedDownload(target.getName(),
                        mActivity.getTitle().toString(), true,
                        uri.getMimeType(), target.getAbsolutePath(),
                        uri.getData().length, true);
                } catch (IOException e) {
                    Log.e(LOGTAG, "Could not save data URL");
                } finally {
                    if (outputStream != null) {
                        try {
                            outputStream.close();
                        } catch (IOException e) {
                            // ignore close errors
                        }
                    }
                }
                if( target != null){
                    return target.getAbsolutePath();
                }
                return null;
            }
        });
        mCancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                closeDialogMessage();
                if(closeOnExit){
                    goBackOnePageOrQuit();
                }
            }
        });
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setTitle(R.string.fetching_content_length);
        builder.setIcon(android.R.drawable.ic_dialog_alert);
        builder.setView(certificateView);
        if (mDownloadDialog == null){
            mDownloadDialog = builder.create();
            if(!mActivity.isFinishing()){
                mDownloadDialog.show();
                new FetchContentLength(mActivity, mUrl, mUserAgent, mPrivateBrowsing, mDownloadDialog, mHandler).start();
            }
        }
    }

    private void selectDownloadStorage(){
        showStorage(INTERNAL,SD,OTG);
    }

    private void showStorage(final String internal,final String sdCard,final String otg){
        if(!StorageUtils.isExternalStorageMounted() && !StorageUtils.isInternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()){
            Log.i(LOGTAG, "showStorage, no available storage");
            Toast.makeText(mActivity, R.string.no_storage, Toast.LENGTH_SHORT).show();
            return;
        }

        if (!StorageUtils.isExternalStorageMounted() && !StorageUtils.isUsbdiskStorageMounted()) {
            selectPath(internal);
            return;
        }

        LayoutInflater factory = LayoutInflater.from(mActivity);
        final View selectPathView = factory.inflate(R.layout.select_download_path_dialog, null);
        Button mBtnCancel = (Button)selectPathView.findViewById(R.id.select_path_cancel);
        Button mBtnInternal = (Button)selectPathView.findViewById(R.id.select_path_internal);
        Button mBtnSDCard = (Button)selectPathView.findViewById(R.id.select_path_sdcard);
        Button mBtnOTG = (Button)selectPathView.findViewById(R.id.select_path_otg);

        if (!StorageUtils.isExternalStorageMounted()) {
            mBtnSDCard.setVisibility(View.GONE);
        }
        if (!StorageUtils.isUsbdiskStorageMounted()) {
            mBtnOTG.setVisibility(View.GONE);
        }

        mBtnCancel.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                dismissSelectStorageDialog();
            }
        });
        mBtnInternal.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(internal);
            }
        });
        mBtnSDCard.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(sdCard);
            }
        });
        mBtnOTG.setOnClickListener(new OnClickListener(){
            @Override
            public void onClick(View v) {
                selectPath(otg);
            }
        });
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setTitle(R.string.select_storage_title);
        builder.setView(selectPathView);
        if (mSelectStorageDialog == null){
            mSelectStorageDialog = builder.create();
            mSelectStorageDialog.show();
        }
        mSelectStorageDialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                mSelectStorageDialog = null;
            }
        });
    }

    private void dismissSelectStorageDialog(){
        if (mSelectStorageDialog != null){
            mSelectStorageDialog.dismiss();
        }
    }

    private void selectPath(String storage){
        if ((mDownloadPath != null) && (null != mDownloadPath.getText())){
            if(isSavePathEditorActivityStart){
                Log.e(LOGTAG, storage + " has been ingoner");
                return;
            }else {
                Log.e(LOGTAG, storage + " has been send");
                isSavePathEditorActivityStart = true;
                Intent editPath = new Intent();
                editPath.setClassName( "com.android.browser", "com.android.browser.SavePathEditor");
                Bundle mbundle = new Bundle();
                mbundle.putString("storage", storage);
                mbundle.putString("mDownloadTag",mDownloadPath.getText().toString());
                editPath.putExtras(mbundle);
                //modify for download permission
                if (mActivity.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                        != PackageManager.PERMISSION_GRANTED) {
                    Log.i(LOGTAG, "no storage permission, Controller.mPermissionObj = " + Controller.mPermissionObj);
                    if (Controller.mPermissionObj == null) {
                        Controller.mPermissionObj = editPath;
                        Log.i(LOGTAG, "requestPermissions storage permission");
                        mActivity.requestPermissions(
                                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE},
                                Controller.PERMISSIONS_REQUEST_STORAGE_READ_WRITE_SELECT_PATH);
                    }
                } else {
                    mActivity.startActivityForResult(editPath, Controller.DOWNLOAD_SELECTED);
               }
            }
        }else{
            Log.e(LOGTAG, "mDownloadPath.getText() return null, Exception in SaveWebPage,please check");
        }
    }

    public void closeDialogMessage() {
        dismissSelectStorageDialog();
        if(mDownloadDialog!=null){
            mDownloadDialog.dismiss();
            mDownloadDialog = null;
            mDownloadSize = null;
        }
    }

    private File getTarget(DataUri uri, String path) throws IOException {
        File dir = new File(path);
        DateFormat format = new SimpleDateFormat(IMAGE_BASE_FORMAT);
        String nameBase = format.format(new Date());
        String mimeType = uri.getMimeType();
        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        String extension = mimeTypeMap.getExtensionFromMimeType(mimeType);
        if (extension == null) {
            Log.w(LOGTAG, "Unknown mime type in data URI" + mimeType);
            extension = FALLBACK_EXTENSION;
        }
        extension = "." + extension;
        File targetFile = File.createTempFile(nameBase, extension, dir);
        return targetFile;
    }
    /*@}*/

    /*
     * for download_overlay_same_file
     *@{
     */
    public void checkDownloadFileIsExists(Controller controller,Activity activity,
            String url, String userAgent, String contentDisposition,
            String mimetype, boolean privateBrowsing, boolean closeOnExit){
        String extension = null;
        final String myType = mimetype;
        final String myUrl = url;
        final String myUserAgent = userAgent;
        final String myContentDisposition = contentDisposition;
        final boolean myPrivateBrowsing = privateBrowsing;
        final boolean myCloseOnExit = closeOnExit;
        String filename = URLUtil.guessFileName(url, contentDisposition, mimetype);
        int dotIndex = filename.indexOf('.');
        if(dotIndex<0){
            extension = chooseExtensionFromMimeType(mimetype,true);
        }else{
            extension = chooseExtensionFromFilename(mimetype,filename,dotIndex);
        }
        final String download_path_filename = getDownloadPath() + filename.substring(0, dotIndex) + extension;
        final File file = new File(download_path_filename);

        mFileProgressDialog = ProgressDialog.show(activity, mActivity.getText(R.string.file_check), mActivity.getText(R.string.processing_please_wait));
        mFileProgressDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            public void onDismiss(DialogInterface dialog) {
                mFileProgressDialog = null;
            }
        });
        mFileProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                Log.i(LOGTAG, "mFileProgressDialog cancel");
                DownloadHandler.onDownloadStartNoStream(Controller.this,mActivity, myUrl, myUserAgent, myContentDisposition,
                    myType, myPrivateBrowsing, myCloseOnExit);
            }
        });

        mOverlayDialog = null;
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setTitle(R.string.file_exists);
        builder.setIcon(android.R.drawable.ic_dialog_alert);
        builder.setMessage(R.string.file_exists_msg);
        builder.setPositiveButton(R.string.file_coverage,
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mFileDeleteProgressDialog = ProgressDialog.show(mActivity, mActivity.getText(R.string.file_delete), mActivity.getText(R.string.processing_please_wait));
                    mFileDeleteProgressDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                            Log.i(LOGTAG, "mFileDeleteProgressDialog onDismiss");
                            mFileDeleteProgressDialog = null;
                            DownloadHandler.onDownloadStartNoStream(Controller.this,mActivity, myUrl, myUserAgent, myContentDisposition,
                                myType, myPrivateBrowsing, myCloseOnExit);
                        }
                    });
                    new FileDeleteThread(download_path_filename).start();
                }
            });
        builder.setNegativeButton(R.string.file_exists_ignore,
            new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                        DownloadHandler.onDownloadStartNoStream(Controller.this,mActivity, myUrl, myUserAgent, myContentDisposition,
                            myType, myPrivateBrowsing, myCloseOnExit);
                }
            });
        mOverlayDialog = builder.create();

        new FileCheckThread(download_path_filename).start();
    }
    public void showOverlayDialog(){
        if(mOverlayDialog != null){
            Log.i(LOGTAG, "showOverlayDialog");
            mOverlayDialog.show();
        }
    }
    public class FileCheckThread extends Thread{
       String fileName = null;
       public FileCheckThread(String str){
           fileName = str;
       }
       public void run(){
            if(fileName != null){
                try{
                    Thread.sleep(1000);
                }catch (Exception e){
                }
                File testFile = new File(fileName);
                if(testFile.exists()){
                    mHandler.sendMessage(mHandler.obtainMessage(OVERLAY_DIALOG, true));
                    mFileProgressDialog.dismiss();
                    Log.i(LOGTAG, "FileCheckThread  testFile exists");
                }else{
                    mFileProgressDialog.cancel();
                    Log.i(LOGTAG, "FileCheckThread  testFile not exists");
                }
            }
       }
    }

    public class FileDeleteThread extends Thread{
        String fileName = null;
        public FileDeleteThread(String str){
            fileName = str;
        }
        public void run(){
            if(fileName != null){
                new File(fileName).delete();
                deleteDatabase(fileName);
            }
            mFileDeleteProgressDialog.dismiss();
            Log.i(LOGTAG, "FileDeleteThread run end");
        }
    }
    private static String chooseExtensionFromMimeType(String mimeType, boolean useDefaults) {
        String extension = null;
        if (mimeType != null) {
            extension = MimeTypeMap.getSingleton().getExtensionFromMimeType(mimeType);
            if (extension != null) {
                extension = "." + extension;
            }
        }
        if (extension == null) {
            if (mimeType != null && mimeType.toLowerCase().startsWith("text/")) {
                if (mimeType.equalsIgnoreCase("text/html")) {
                    extension = ".html";
                } else if (useDefaults) {
                    extension = ".txt";
                }
            } else if (useDefaults) {
                extension = ".bin";
            }
        }
        return extension;
    }

    private static String chooseExtensionFromFilename(String mimeType, String filename, int dotIndex) {
        String extension = null;
        if (mimeType != null) {
            int lastDotIndex = filename.lastIndexOf('.');
            String typeFromExt = MimeTypeMap.getSingleton().getMimeTypeFromExtension(
                    filename.substring(lastDotIndex + 1).toLowerCase());
            if (typeFromExt == null || !typeFromExt.equalsIgnoreCase(mimeType)) {
                extension = chooseExtensionFromMimeType(mimeType, false);
            }
        }
        if (extension == null) {
            extension = filename.substring(dotIndex);
        }
        return extension;
    }
    private void deleteDatabase(String fileName){
        DownloadManager manager = (DownloadManager) mActivity.getSystemService(Context.DOWNLOAD_SERVICE);
        DownloadManager.Query mQuery = new DownloadManager.Query();
        manager.setAccessFilename(true);
        Cursor cursor = manager.query(mQuery);
        if(cursor != null){
            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                String downloadFileName = cursor.getString(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_FILENAME));
                if(fileName.equals(downloadFileName)){
                   long downloadId = cursor.getLong(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_ID));
                   manager.remove(downloadId);
                }
            }
        }
        if (cursor != null){
            cursor.close();
        }
    }
    /*@}*/

    /**
     * Add for oma download
     *@{
     */
    public void onDownloadSucessReceiver(Context context , Intent intent){
        Log.i(LOGTAG, "onDownloadSucessReceiver, action = " + intent.getAction());
        if(OMA_DD_DOWNLOAD_ACTION.equals(intent.getAction())){
            final String name = intent.getExtras().getString("name");
            String dd = intent.getExtras().getString("OMADD");
            final   String URI = intent.getExtras().getString("uri");
            final String mim =  intent.getExtras().getString("meta_type");
            //final String notify_uri = intent.getExtras().getString("notify_uri");

            new AlertDialog.Builder(context)
                .setTitle(R.string.oma_download_descriptor)
                .setMessage(dd)
                .setPositiveButton(R.string.ok,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            DownloadHandler.onDownloadStartNoStream(mActivity, URI, null, "content-disposition" ,
                                                                    mim, null, false);
                        }
                    })
                .setNegativeButton(R.string.cancel,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                        }
                    })
                .show();
        } else if(OMA_DD_DOWNLOAD_RESULT_ACTION.equals(intent.getAction())){
            String err = intent.getExtras().getString("ERRORMAG");

            new AlertDialog.Builder(context)
                .setTitle(R.string.oma_download_error)
                .setMessage(err)
                .setPositiveButton(R.string.ok,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                        }
                    })
                .show();
        } else if(SPACE_INSUFFICIENT_ACTION.equals(intent.getAction())){
            final String id = intent.getExtras().getString("ID");

            new AlertDialog.Builder(context)
                .setTitle(R.string.download)
                .setMessage(R.string.free_space)
                .setPositiveButton(R.string.ok,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                        }
                    })
                .show();
        } else if (DOWNLOAD_FILE_TOOLONG.equals(intent.getAction())) {
            final String id = intent.getExtras().getString("ID");

            new AlertDialog.Builder(context)
                .setTitle(R.string.download_failure).setMessage(R.string.file_toolong)
                .setPositiveButton(R.string.ok,
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                    }
                    })
                .show();
        }else if(DOWNLOAD_FILE_SUCCESS.equals(intent.getAction())){
            String filename = intent.getExtras().getString("filename");
            Log.i(LOGTAG,"download file success:"+filename);
            String downloadPath = null;
            String str = context.getString(R.string.download_complete);
            CharSequence path = str +filename;
            path = Common.getBrowserDrmPlugIn().getFilePath(path,str,filename);
            Toast.makeText(context, path, Toast.LENGTH_SHORT).show();
        }
    }
    /*@}*/

    //modify for permissions
    @Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        boolean resultsAllGranted = true;
        if (grantResults.length > 0) {
            for (int result : grantResults) {
                if (PackageManager.PERMISSION_GRANTED != result) {
                    resultsAllGranted = false;
                    break;
                }
            }
        } else {
            resultsAllGranted = false;
        }

        if(requestCode == PERMISSIONS_REQUEST_STORAGE_READ_WRITE) {
            final DownloadManager.Request request = (DownloadManager.Request)Controller.mPermissionObj;
            if (resultsAllGranted) {
                Log.i(LOGTAG, "onRequestPermissionsResult, storage permission granted");
                if (request != null) {
                    final DownloadManager manager
                            = (DownloadManager) mActivity.getSystemService(Context.DOWNLOAD_SERVICE);
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
                Toast.makeText(mActivity, R.string.download_pending, Toast.LENGTH_SHORT).show();
            } else {
                Log.i(LOGTAG, "onRequestPermissionsResult, storage permission not granted");
            }
        } else if (requestCode == PERMISSIONS_REQUEST_OPENFILE) {
            Intent intent = (Intent)Controller.mPermissionObj;
            if (resultsAllGranted) {
                Log.i(LOGTAG, "onRequestPermissionsResult, open file permission granted");
                if (null != mUploadHandler && intent != null) {
                    mUploadHandler.startActivity(intent);
                }
            } else {
                Log.i(LOGTAG, "onRequestPermissionsResult, open file permission not granted");
                if (null != mUploadHandler) {
                    mUploadHandler.onResult(Activity.RESULT_CANCELED, intent);
                }
            }
        } else if (requestCode == PERMISSIONS_REQUEST_STORAGE_READ_WRITE_SELECT_PATH) {
            Intent intent = (Intent)Controller.mPermissionObj;
            if (resultsAllGranted) {
                Log.i(LOGTAG, "onRequestPermissionsResult, storage permission for path granted");
                if (intent != null) {
                    mActivity.startActivityForResult(intent, Controller.DOWNLOAD_SELECTED);
                }
            } else {
                Log.i(LOGTAG, "onRequestPermissionsResult, storage permission for path not granted");
                isSavePathEditorActivityStart = false;
            }
        } else if (requestCode == PERMISSIONS_REQUEST_CALL_PHONE) {
            Intent intent = (Intent)Controller.mPermissionObj;
            if (resultsAllGranted) {
                Log.i(LOGTAG, "onRequestPermissionsResult, call phone permission granted");
                if (intent != null) {
                    mActivity.startActivity(intent);
                    closeEmptyTab();
                }
            } else {
                Log.i(LOGTAG, "onRequestPermissionsResult, call phone permission not granted");
                closeEmptyTab();
            }
        } else if (requestCode == PERMISSIONS_REQUEST_MEDIA) {
            PermissionRequest request = (PermissionRequest)Controller.mPermissionObj;
            if (resultsAllGranted) {
                Log.i(LOGTAG, "onRequestPermissionsResult, media permission granted");
                if (request != null) {
                    request.grant(request.getResources());
                }
            } else {
                Log.i(LOGTAG, "onRequestPermissionsResult, media permission not granted");
                request.deny();
            }
        }
        Controller.mPermissionObj = null;
    }

    /*
     * for save_webpage
     *@{
     */
    @Override
    public SnapshotTab createNewSnapshotTab(long snapshotId, boolean setActive) {
        SnapshotTab tab = null;
        if (mTabControl.canCreateNewTab()) {
            tab = mTabControl.createSnapshotTab(snapshotId);
            addTab(tab);
            if (setActive) {
                setActiveTab(tab);
            }
        } else {
            mUi.showMaxTabsWarning();
        }
        return tab;
    }

    private void saveWebArchive() {
        Log.i(LOGTAG, "sprd saveWebArchive()");
        String status = StorageUtils.getExternalStorageState();
        if((null == status) || (!status.equals(Environment.MEDIA_MOUNTED))){
            Toast.makeText(mActivity, R.string.save_webpage_no_storage,Toast.LENGTH_SHORT).show();
            return;
        }
        String timeIString = Long.toString(System.currentTimeMillis());
        File externStoragePath = StorageUtils.getExternalStorageDirectory();
        final String browserWebArchiveDir = externStoragePath.toString()
                + "/Android/data/com.android.browser/snapshotpage/" + timeIString + "/";
        Log.i(LOGTAG, "browserWebArchive = " + browserWebArchiveDir);
        File path = new File(browserWebArchiveDir);
        if (!path.exists()) {
            if (!path.mkdirs()) {
                Toast.makeText(mActivity, mActivity.getText(R.string.mkdirs_error),Toast.LENGTH_SHORT).show();
                return;
            }
        }
        final String basename = browserWebArchiveDir + timeIString + ".mht";
        if ((mTabControl != null) && (mTabControl.getCurrentWebView() != null)) {
            isSavePage = true;
            mTabControl.getCurrentWebView().saveWebArchive(basename, false,
                new ValueCallback<String>() {
                    @Override
                    public void onReceiveValue(String value) {
                        isSavePage = false;
                        if (value == null) {
                            Log.i(LOGTAG, "savepage failed");
                        } else {
                            Log.i(LOGTAG, "savepage success and value="+value);
                            final Tab source = getTabControl().getCurrentTab();
                            if (source == null) return;
                            new SaveSnapshotTask(source, basename,browserWebArchiveDir).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                        }
                    }
                });
        }
    }
    private class SaveSnapshotTask extends AsyncTask<Void, Void, Long> implements OnCancelListener {
        private Tab mTab;
        private Dialog mProgressDialog;
        private ContentValues mValues;
        private String mBasename;
        private String mFileDir;

        private SaveSnapshotTask(Tab tab, String basename, String fileDir) {
            mTab = tab;
            mBasename = basename;
            mFileDir = fileDir;
        }
        @Override
        protected void onPreExecute() {
            CharSequence message = mActivity.getText(R.string.saving_snapshot);
            mProgressDialog = ProgressDialog.show(mActivity, null, message, true, true, this);
            mValues = mTab.createSnapshotValues();
        }
        @Override
        protected Long doInBackground(Void... params) {
            if(null == mValues){
                Log.e(LOGTAG, "SaveSnapshotTask  doInBackground error,mValues is null ");
                return null;
            }
            mValues.put(Snapshots.WEBARCHIVE_PATH, mBasename);
            if (isCancelled()) {
                Log.e(LOGTAG, "SaveSnapshotTask  doInBackground cancelled ");
                String webArchive = mValues.getAsString(Snapshots.WEBARCHIVE_PATH);
                File webArchivefile = mActivity.getFileStreamPath(webArchive);
                if (!webArchivefile.delete()) {
                    webArchivefile.deleteOnExit();
                }
                return null;
            }
            Uri result = null;
            final ContentResolver cr = mActivity.getContentResolver();
            try {
                result = cr.insert(Snapshots.CONTENT_URI, mValues);
            } catch (SQLiteException e) {
                Log.e(LOGTAG, "SaveSnapshotTask:", e);
            }
            if (result == null) {
                return null;
            }
            long id = ContentUris.parseId(result);
            return id;
        }
        @Override
        protected void onPostExecute(Long id) {
            if (isCancelled() || mActivity.isFinishing()) {
                return;
            }
            //File file = new File(mFileDir);
            //mActivity.sendBroadcast(new Intent("android.intent.action.MEDIA_SCANNER_SCAN_DIR", Uri.fromFile(file)));
            mProgressDialog.dismiss();
            if (id == null) {
                Toast.makeText(mActivity, R.string.snapshot_failed,
                        Toast.LENGTH_SHORT).show();
                return;
            }
            Bundle b = new Bundle();
            b.putBoolean(BrowserBookmarksPage.EXTRA_DISABLE_WINDOW,
                !mTabControl.canCreateNewTab());
            mUi.showComboView(ComboViews.Snapshots, b);
        }
        @Override
        public void onCancel(DialogInterface dialog) {
            cancel(true);
        }
    }
    /*@}*/

    /**
     *Add for bottom bar
     *@{
     */
    public void updateMenuBarState() {
        final Tab ct = mTabControl.getCurrentTab();
        if (ct != null) {
            mUi.updateMenuBarState(ct.canGoBack(), ct.canGoForward());
        }
    }
    /*@}*/
}
