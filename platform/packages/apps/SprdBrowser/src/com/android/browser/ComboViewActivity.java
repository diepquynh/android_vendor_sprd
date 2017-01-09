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

import android.app.ActionBar;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v13.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.Menu;
import android.view.MenuItem;

import com.android.browser.UI.ComboViews;
import com.android.browser.util.Util;

import java.util.ArrayList;
import android.net.WebAddress;
import java.net.URISyntaxException;
import android.net.ParseException;
import android.util.Log;

public class ComboViewActivity extends Activity implements CombinedBookmarksCallbacks {

    private static final String STATE_SELECTED_TAB = "tab";
    public static final String EXTRA_COMBO_ARGS = "combo_args";
    public static final String EXTRA_INITIAL_VIEW = "initial_view";

    public static final String EXTRA_OPEN_SNAPSHOT = "snapshot_id";
    public static final String EXTRA_OPEN_ALL = "open_all";
    public static final String EXTRA_CURRENT_URL = "url";
    private ViewPager mViewPager;
    private TabsAdapter mTabsAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setResult(RESULT_CANCELED);
        Bundle extras = getIntent().getExtras();
        Bundle args = extras.getBundle(EXTRA_COMBO_ARGS);
        String svStr = extras.getString(EXTRA_INITIAL_VIEW, null);
        ComboViews startingView = svStr != null
                ? ComboViews.valueOf(svStr)
                : ComboViews.Bookmarks;
        mViewPager = new ViewPager(this);
        mViewPager.setId(R.id.tab_view);
        setContentView(mViewPager);

        final ActionBar bar = getActionBar();
        bar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
        if (BrowserActivity.isTablet(this)) {
            bar.setDisplayOptions(ActionBar.DISPLAY_SHOW_HOME
                    | ActionBar.DISPLAY_USE_LOGO);
            bar.setHomeButtonEnabled(true);
        } else {
            bar.setDisplayOptions(0);
        }
        bar.setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE);

        mTabsAdapter = new TabsAdapter(this, mViewPager);
        mTabsAdapter.addTab(bar.newTab().setText(R.string.tab_bookmarks),
                BrowserBookmarksPage.class, args);
        /*add histroy tab*/
        mTabsAdapter.addTab(bar.newTab().setText(R.string.tab_history),
                BrowserHistoryPage.class, args);

        /*
         * for save_webpage
         *@{
         */
        if (Util.SUPPORT_SAVE_SNAPSHOT) {
            mTabsAdapter.addTab(bar.newTab().setText(R.string.tab_snapshots),
                    BrowserSnapshotPage.class, args);
        }
        /*@}*/

        if (savedInstanceState != null) {
            bar.setSelectedNavigationItem(
                    savedInstanceState.getInt(STATE_SELECTED_TAB, 0));
        } else {
            switch (startingView) {
            case Bookmarks:
                mViewPager.setCurrentItem(0);
                break;
            case History:
                mViewPager.setCurrentItem(1);
                break;
            case Snapshots:
                mViewPager.setCurrentItem(2);
                break;
            }
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(STATE_SELECTED_TAB,
                getActionBar().getSelectedNavigationIndex());
    }

    @Override
    public void openUrl(String url) {
        /*
         * for rtsp
         * original code
        Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        setResult(RESULT_OK, i);
        finish();
         *@{
         */
        try {
            WebAddress address;
            String httpUrl = null;
            try {
               if (url.startsWith("content://")){
                   httpUrl = url;
                } else if (url.startsWith("rtsp://")) {//add for rtsp feature
                    Intent rtspIntent = new Intent(Intent.ACTION_VIEW,Uri.parse(url));
                    rtspIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    startActivity(rtspIntent);
                    finish();
                    return;
                } else{
                    if (!Bookmarks.urlHasAcceptableScheme(url)){
                        address = new WebAddress(url);
                        httpUrl = address.toString();
                        if (address.getHost().length() == 0) {
                            if (!url.startsWith("file://")){
                                throw new URISyntaxException("", "");
                            }
                        }
                    } else {
                        httpUrl = url;
                    }
               }
            } catch (ParseException e) {
                throw new URISyntaxException("", "");
            }
            Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(httpUrl));
            setResult(RESULT_OK, i);
            finish();
        } catch (URISyntaxException e) {
            Log.e("ComboViewActivity", "Exception URISyntaxException url "+e);
        }
        /*@}*/

    }

    @Override
    public void openInNewTab(String... urls) {
        Intent i = new Intent();
        i.putExtra(EXTRA_OPEN_ALL, urls);
        setResult(RESULT_OK, i);
        finish();
    }

    @Override
    public void close() {
        finish();
    }

    @Override
    public void openSnapshot(long id) {
        Intent i = new Intent();
        i.putExtra(EXTRA_OPEN_SNAPSHOT, id);
        setResult(RESULT_OK, i);
        finish();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.combined, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
            return true;
        } else if (item.getItemId() == R.id.preferences_menu_id) {
            String url = getIntent().getStringExtra(EXTRA_CURRENT_URL);
            Intent intent = new Intent(this, BrowserPreferencesPage.class);
            intent.putExtra(BrowserPreferencesPage.CURRENT_PAGE, url);
            startActivityForResult(intent, Controller.PREFERENCES_PAGE);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * This is a helper class that implements the management of tabs and all
     * details of connecting a ViewPager with associated TabHost.  It relies on a
     * trick.  Normally a tab host has a simple API for supplying a View or
     * Intent that each tab will show.  This is not sufficient for switching
     * between pages.  So instead we make the content part of the tab host
     * 0dp high (it is not shown) and the TabsAdapter supplies its own dummy
     * view to show as the tab content.  It listens to changes in tabs, and takes
     * care of switch to the correct page in the ViewPager whenever the selected
     * tab changes.
     */
    public static class TabsAdapter extends FragmentPagerAdapter
            implements ActionBar.TabListener, ViewPager.OnPageChangeListener {
        private final Context mContext;
        private final ActionBar mActionBar;
        private final ViewPager mViewPager;
        private final ArrayList<TabInfo> mTabs = new ArrayList<TabInfo>();

        static final class TabInfo {
            private final Class<?> clss;
            private final Bundle args;

            TabInfo(Class<?> _class, Bundle _args) {
                clss = _class;
                args = _args;
            }
        }

        public TabsAdapter(Activity activity, ViewPager pager) {
            super(activity.getFragmentManager());
            mContext = activity;
            mActionBar = activity.getActionBar();
            mViewPager = pager;
            mViewPager.setAdapter(this);
            mViewPager.setOnPageChangeListener(this);
        }

        public void addTab(ActionBar.Tab tab, Class<?> clss, Bundle args) {
            TabInfo info = new TabInfo(clss, args);
            tab.setTag(info);
            tab.setTabListener(this);
            mTabs.add(info);
            mActionBar.addTab(tab);
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mTabs.size();
        }

        @Override
        public Fragment getItem(int position) {
            TabInfo info = mTabs.get(position);
            return Fragment.instantiate(mContext, info.clss.getName(), info.args);
        }

        @Override
        public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
        }

        @Override
        public void onPageSelected(int position) {
            mActionBar.setSelectedNavigationItem(position);
        }

        @Override
        public void onPageScrollStateChanged(int state) {
        }

        @Override
        public void onTabSelected(android.app.ActionBar.Tab tab,
                FragmentTransaction ft) {
            Object tag = tab.getTag();
            for (int i=0; i<mTabs.size(); i++) {
                if (mTabs.get(i) == tag) {
                    /*
                     * for save_webpage
                     * original code
                    mViewPager.setCurrentItem(i);
                     *@{
                     */
                    switch (i) {
                    case 0:
                        mViewPager.setCurrentItem(0);
                        break;
                    case 1:
                        mViewPager.setCurrentItem(1);
                        break;
                    case 2:
                        mViewPager.setCurrentItem(2);
                        break;
                    default:
                        mViewPager.setCurrentItem(0);
                        break;
                    }
                    /*@}*/
                }
            }
        }

        @Override
        public void onTabUnselected(android.app.ActionBar.Tab tab,
                FragmentTransaction ft) {
        }

        @Override
        public void onTabReselected(android.app.ActionBar.Tab tab,
                FragmentTransaction ft) {
        }
    }

    private static String makeFragmentName(int viewId, int index) {
        return "android:switcher:" + viewId + ":" + index;
    }

}
