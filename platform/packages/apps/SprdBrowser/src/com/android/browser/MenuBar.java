package com.android.browser;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.DecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.view.Gravity;

public class MenuBar extends LinearLayout {

    private static final String TAG = "MemuBar";
    private BaseUi mBaseUi;
    //protected LinearLayout mMenuBar;
    protected ImageView mMenuBarBack;
    protected ImageView mMenuBarBookmarks;
    protected ImageView mMenuBarForward;
    protected ImageView mMenuBarStop;
    protected TextView mMenuBarTabCount;
    protected ImageView mMenuBarTabs;
    //private FrameLayout mContentView;
    //private int mContentViewHeight;
    private Context mContext;
    //private boolean mShowing;
    private TabControl mTabControl;
    private UiController mUiController;

    public MenuBar(Context Context, UiController UiController,
            BaseUi BaseUi, TabControl TabControl,
            FrameLayout FrameLayout) {
        super(Context, null);
        mContext = Context;
        mUiController = UiController;
        mBaseUi = BaseUi;
        mTabControl = TabControl;
        //mContentView = FrameLayout;
        initLayout(Context);
        mBaseUi.addFixedMenuBar(this);
    }

    private void initLayout(Context Context) {
        LayoutInflater.from(Context).inflate(R.layout.menu_bar, this);
        //mMenuBar = (LinearLayout) findViewById(R.id.menubar);
        mMenuBarBack = (ImageView) findViewById(R.id.back);
        mMenuBarForward = (ImageView) findViewById(R.id.forward);
        mMenuBarTabs = (ImageView) findViewById(R.id.tabs);
        mMenuBarBookmarks = (ImageView) findViewById(R.id.bookmarks);
        mMenuBarStop = (ImageView) findViewById(R.id.stop_img);
        mMenuBarTabCount = (TextView) findViewById(R.id.tabcount);
        mMenuBarBack.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View paramAnonymousView) {
                Tab currentTab = mUiController.getTabControl().getCurrentTab();
                if (currentTab.canGoBack())
                    currentTab.goBack();
            }
        });
        mMenuBarBack.setOnLongClickListener(new OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View paramAnonymousView) {
                        Toast.makeText(
                                mUiController.getActivity(),
                                mUiController.getActivity().getResources().getString(R.string.back),0).show();
                        return false;
                    }
                });
        mMenuBarForward.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View paramAnonymousView) {
                if ((mUiController != null) && (mUiController.getCurrentTab() != null))
                    mUiController.getCurrentTab().goForward();
            }
        });
        mMenuBarForward.setOnLongClickListener(new OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View paramAnonymousView) {
                        Toast.makeText(
                                mUiController.getActivity(),
                                mUiController.getActivity().getResources().getString(R.string.forward),0).show();
                        return false;
                    }
                });

        mMenuBarStop.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View paramAnonymousView) {
                if ((mUiController != null) && (mUiController.getCurrentTab() != null))
                    mUiController.stopLoading();
                changeStoploadingState(false);
            }
        });

        mMenuBarTabs.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View paramAnonymousView) {
                ((PhoneUi)mBaseUi).toggleNavScreen();
            }
        });
        mMenuBarTabs.setOnLongClickListener(new OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View paramAnonymousView) {
                        Toast.makeText(
                                mUiController.getActivity(),
                                mUiController.getActivity().getResources().getString(R.string.tabs),0).show();
                        return false;
                    }
                });
        mMenuBarBookmarks.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View paramAnonymousView) {
                mUiController.bookmarksOrHistoryPicker(UI.ComboViews.Bookmarks);
            }
        });
        mMenuBarBookmarks.setOnLongClickListener(new OnLongClickListener() {
                    @Override
                    public boolean onLongClick(View paramAnonymousView) {
                        Toast.makeText(
                                mUiController.getActivity(),
                                mUiController.getActivity().getResources().getString(R.string.bookmarks),0).show();
                        return false;
                    }
                });
        mMenuBarTabCount.setText(Integer.toString(mUiController.getTabControl().getTabCount()));
        mTabControl.setOnTabCountChangedListener(new TabControl.OnTabCountChangedListener() {
                    @Override
                    public void onTabCountChanged() {
                        mMenuBarTabCount.setText(Integer.toString(mTabControl.getTabCount()));
                        /* SPRD: Modify for Bug:501784, should not show forward and back button when all tabs closed @{ */
                        if (mTabControl.getTabCount() <= 0) {
                            changeMenuBarState(false, false);
                        }
                        /* @} */
                    }
                });
    }

    public void changeMenuBarState(boolean canGoBack,
            boolean canGoForward) {
        mMenuBarBack.setEnabled(canGoBack);
        mMenuBarForward.setEnabled(canGoForward);
    }

    public void changeStoploadingState(boolean update) {
        if (update) {
            mMenuBarStop.setVisibility(View.VISIBLE);
            mMenuBarForward.setVisibility(View.GONE);
        } else {
            mMenuBarStop.setVisibility(View.GONE);
            mMenuBarForward.setVisibility(View.VISIBLE);
        }
    }
}
