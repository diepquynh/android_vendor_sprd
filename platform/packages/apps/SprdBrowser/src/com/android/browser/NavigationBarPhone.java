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

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewConfiguration;
import android.webkit.WebView;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.widget.PopupMenu.OnDismissListener;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.util.Log;
import android.content.res.Configuration;

import com.android.browser.UrlInputView.StateListener;
import com.android.browser.util.Util;
import com.android.browser.sitenavigation.SiteNavigation;

public class NavigationBarPhone extends NavigationBarBase implements
        StateListener, OnMenuItemClickListener, OnDismissListener {

    private static final String TAG = "NavigationBarPhone";
    private ImageView mStopButton;
    private ImageView mMagnify;
    private ImageView mClearButton;
    private ImageView mVoiceButton;
    private Drawable mStopDrawable;
    private Drawable mRefreshDrawable;
    private String mStopDescription;
    private String mRefreshDescription;
    private View mTabSwitcher;
    private View mComboIcon;
    private View mTitleContainer;
    private View mMore;
    private Drawable mTextfieldBgDrawable;
    private PopupMenu mPopupMenu;
    private boolean mOverflowMenuShowing;
    private boolean mNeedsMenu;
    private View mIncognitoIcon;
    private View mAnchor;

    public NavigationBarPhone(Context context) {
        super(context);
    }

    public NavigationBarPhone(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public NavigationBarPhone(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mStopButton = (ImageView) findViewById(R.id.stop);
        mStopButton.setOnClickListener(this);
        mClearButton = (ImageView) findViewById(R.id.clear);
        mClearButton.setOnClickListener(this);
        mVoiceButton = (ImageView) findViewById(R.id.voice);
        mVoiceButton.setOnClickListener(this);
        mMagnify = (ImageView) findViewById(R.id.magnify);
        mTabSwitcher = findViewById(R.id.tab_switcher);
        mTabSwitcher.setOnClickListener(this);
        mMore = findViewById(R.id.more);
        mMore.setOnClickListener(this);
        mComboIcon = findViewById(R.id.iconcombo);
        mComboIcon.setOnClickListener(this);
        mTitleContainer = findViewById(R.id.title_bg);
        setFocusState(false);
        Resources res = getContext().getResources();
        mStopDrawable = res.getDrawable(R.drawable.ic_stop_holo_dark);
        mRefreshDrawable = res.getDrawable(R.drawable.ic_refresh_holo_dark);
        mStopDescription = res.getString(R.string.accessibility_button_stop);
        mRefreshDescription = res.getString(R.string.accessibility_button_refresh);
        mTextfieldBgDrawable = res.getDrawable(R.drawable.textfield_active_holo_dark);
        mUrlInput.setContainer(this);
        mUrlInput.setStateListener(this);
        mNeedsMenu = !ViewConfiguration.get(getContext()).hasPermanentMenuKey();
        mIncognitoIcon = findViewById(R.id.incognito_icon);
    }

    @Override
    public void onProgressStarted() {
        super.onProgressStarted();
        if (mStopButton.getDrawable() != mStopDrawable) {
            mStopButton.setImageDrawable(mStopDrawable);
            mStopButton.setContentDescription(mStopDescription);
            if (mStopButton.getVisibility() != View.VISIBLE) {
                mComboIcon.setVisibility(View.GONE);
                mStopButton.setVisibility(View.VISIBLE);
            }
        }
    }

    @Override
    public void onProgressStopped() {
        super.onProgressStopped();
        mStopButton.setImageDrawable(mRefreshDrawable);
        mStopButton.setContentDescription(mRefreshDescription);
        if (!isEditingUrl()) {
            mComboIcon.setVisibility(View.VISIBLE);
        }
        onStateChanged(mUrlInput.getState());
    }

    /**
     * Update the text displayed in the title bar.
     * @param title String to display.  If null, the new tab string will be
     *      shown.
     */
    @Override
    void setDisplayTitle(String title) {
        /**
         * Add for navigation tab
         *@{
         */
        String  url = null;
        String urlTile = null;
        String  webViewUrl = null;
        Tab tab = mUiController.getTabControl().getCurrentTab();
        if (tab != null) {
            url = tab.getUrl();
            urlTile = tab.getTitle();
            WebView view = tab.getWebView();
            if (view != null) {
                webViewUrl = view.getUrl();
            }
        }
        if (title != null && (title.startsWith("http://") || title.startsWith("https://") || title.startsWith("rtsp://") || title.startsWith("file://"))) {
            if(urlTile != null)
            title = urlTile +"-"+title;
        }
        if (SiteNavigation.SITE_NAVIGATION_URI.toString().equals(webViewUrl) && urlTile != null) {
            title = urlTile;
        }
        /*@}*/
        mUrlInput.setTag(title);
        if (!isEditingUrl()) {
            if (title == null) {
                mUrlInput.setText(R.string.new_tab);
            } else {
                mUrlInput.setText(UrlUtils.stripUrl(title), false);
            }
            mUrlInput.setSelection(0);
        }
        /**
         * Add for navigation tab
         *@{
         */
        else {
            if (url == null) {
                mUrlInput.setText(R.string.new_tab);
            } else {
                mUrlInput.setText(url, false);
            }
            mUrlInput.selectAll();
        }
        /*@}*/
    }

    @Override
    public void onClick(View v) {
        if (v == mStopButton) {
            if (mTitleBar.isInLoad()) {
                mUiController.stopLoading();
            } else {
                WebView web = mBaseUi.getWebView();
                if (web != null) {
                    stopEditingUrl();
                    web.reload();
                }
            }
        } else if (v == mTabSwitcher) {
            ((PhoneUi) mBaseUi).toggleNavScreen();
        } else if (mMore == v) {
            showMenu(mMore);
        } else if (mClearButton == v) {
            mUrlInput.setText("");
        } else if (mComboIcon == v) {
            mUiController.showPageInfo();
        } else if (mVoiceButton == v) {
            mUiController.startVoiceRecognizer();
        } else {
            super.onClick(v);
        }
    }

    @Override
    public boolean isMenuShowing() {
        return super.isMenuShowing() || mOverflowMenuShowing;
    }

    /*SPRD:597617 menu is invalid while multi window mode changed@{ */
    @Override
    public void showMenu() {
        if (mMore != null) {
            showMenu(mMore);
        } else {
            Log.i(TAG, "showMenu,mMore is null!");
        }
    }

    @Override
    public void dismissMenu() {
        if (mPopupMenu != null && mOverflowMenuShowing) {
            mPopupMenu.dismiss();
            mPopupMenu = null;
        }
    }
    /*@}*/

    void showMenu(View anchor) {
        Activity activity = mUiController.getActivity();
        if (mPopupMenu == null) {
            mAnchor = anchor;
            mPopupMenu = new PopupMenu(mContext, anchor);
            mPopupMenu.setOnMenuItemClickListener(this);
            mPopupMenu.setOnDismissListener(this);
            if (!activity.onCreateOptionsMenu(mPopupMenu.getMenu())) {
                mPopupMenu = null;
                return;
            }
        }
        Menu menu = mPopupMenu.getMenu();
        if (activity.onPrepareOptionsMenu(menu)) {
            mOverflowMenuShowing = true;
            mPopupMenu.show();
        }
    }

    @Override
    public void onDismiss(PopupMenu menu) {
        if (menu == mPopupMenu) {
            onMenuHidden();
            mPopupMenu = null;
        }
    }

    private void onMenuHidden() {
        mOverflowMenuShowing = false;
        mBaseUi.showTitleBarForDuration();
    }

    @Override
    public void onFocusChange(View view, boolean hasFocus) {
        if (view == mUrlInput) {
            if (hasFocus && !mUrlInput.getText().toString().equals(mUrlInput.getTag())) {
                // only change text if different
                mUrlInput.setText((String) mUrlInput.getTag(), false);
                mUrlInput.selectAll();
            } else {
                /**
                 * Add for navigation tab
                 * Original Android code:
                 *    setDisplayTitle(mUrlInput.getText().toString());
                 *@{
                 */
                String  url = null;
                Tab tab = mUiController.getTabControl().getCurrentTab();
                if (tab != null){
                    url = tab.getUrl();
                }
                if (url == null) {
                    url = mUrlInput.getText().toString();
                }
                setDisplayTitle(url);
                /*@}*/
            }
        }
        super.onFocusChange(view, hasFocus);
    }

    @Override
    public void onStateChanged(int state) {
        mVoiceButton.setVisibility(View.GONE);
        switch(state) {
        case StateListener.STATE_NORMAL:
            mComboIcon.setVisibility(View.VISIBLE);
            mStopButton.setVisibility(View.GONE);
            mClearButton.setVisibility(View.GONE);
            mMagnify.setVisibility(View.GONE);
            /**
             *Add for bottom bar
             *Original Android code:
             *mTabSwitcher.setVisibility(View.VISIBLE);
             *@{
             */
            if (!Util.SUPPORT_BOTTOM_BAR)
                mTabSwitcher.setVisibility(View.VISIBLE);
            /*@}*/
            mTitleContainer.setBackgroundDrawable(null);
            mMore.setVisibility(mNeedsMenu ? View.VISIBLE : View.GONE);
            break;
        case StateListener.STATE_HIGHLIGHTED:
            mComboIcon.setVisibility(View.GONE);
            mStopButton.setVisibility(View.VISIBLE);
            /*mClearButton change to VISIBLE when highlighted*/
            mClearButton.setVisibility(View.VISIBLE);
            if ((mUiController != null) && mUiController.supportsVoice()) {
                mVoiceButton.setVisibility(View.VISIBLE);
            }
            mMagnify.setVisibility(View.GONE);
            mTabSwitcher.setVisibility(View.GONE);
            mMore.setVisibility(View.GONE);
            mTitleContainer.setBackgroundDrawable(mTextfieldBgDrawable);
            break;
        case StateListener.STATE_EDITED:
            mComboIcon.setVisibility(View.GONE);
            mStopButton.setVisibility(View.GONE);
            mClearButton.setVisibility(View.VISIBLE);
            mMagnify.setVisibility(View.VISIBLE);
            mTabSwitcher.setVisibility(View.GONE);
            mMore.setVisibility(View.GONE);
            mTitleContainer.setBackgroundDrawable(mTextfieldBgDrawable);
            break;
        }
    }

    @Override
    public void onTabDataChanged(Tab tab) {
        super.onTabDataChanged(tab);
        mIncognitoIcon.setVisibility(tab.isPrivateBrowsingEnabled()
                ? View.VISIBLE : View.GONE);
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        return mUiController.onOptionsItemSelected(item);
    }

    /* SPRD:522198 re-show PopupMenu while system config changed@{ */
    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        if (mPopupMenu != null && mOverflowMenuShowing) {
            Log.i(TAG, "reshow popupmenu");
            mPopupMenu.dismiss();
            mPopupMenu = null;
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    showMenu(mAnchor);
                }
            }, 100);
        }
        String hint = getResources().getString(R.string.search_hint);
        if (hint != null && !hint.equals(mUrlInput.getHint())) {
            mUrlInput.setHint(hint);
        }
    }
    /* }@ */

}
