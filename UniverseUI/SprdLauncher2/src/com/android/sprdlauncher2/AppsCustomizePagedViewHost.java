
package com.android.sprdlauncher2;

import android.content.Context;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TabWidget;
import com.android.sprdlauncher2.R;
import com.android.sprdlauncher2.PagedView.PageSwitchListener;

import android.widget.TabHost.TabContentFactory;
//Created by Spreadst
/* SPRD: Feature 254280,show widget list in edit mode@{ */
public class AppsCustomizePagedViewHost extends FrameLayout {

    private AppsCustomizePagedView mAppsCustomizePane;
    private ImageView head;
    private ImageView tail;
    private Context mContext;
    public static String TAG = "AppsCustomizedHost";

    public AppsCustomizePagedViewHost(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        mContext = context;
    }

    public AppsCustomizePagedViewHost(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    public AppsCustomizePagedViewHost(Context context) {
        super(context);
        mContext = context;
    }

    public void onResume() {
        if (getVisibility() == VISIBLE) {
            // mContent.setVisibility(VISIBLE);
            // We unload the widget previews when the UI is hidden, so need to
            // reload pages
            // Load the current page synchronously, and the neighboring pages
            // asynchronously
            mAppsCustomizePane.loadAssociatedPages(
                    mAppsCustomizePane.getCurrentPage(), true);
            mAppsCustomizePane.loadAssociatedPages(mAppsCustomizePane
                    .getCurrentPage());
        }
    }

    /* SPRD: fix bug291896 @{ */
    public void onTrimMemory() {
        Log.d(TAG, "onTrimMemory started");
        mAppsCustomizePane.clearAllWidgetPages();
        mAppsCustomizePane.onPackagesUpdated(
                LauncherModel.getSortedWidgetsAndShortcuts(mContext));;
    }
    /* @} */

    /**
     * Setup the tab host and create all necessary tabs.
     */
    @Override
    protected void onFinishInflate() {
        head = (ImageView) findViewById(R.id.head);
        tail = (ImageView) findViewById(R.id.tail);
        mAppsCustomizePane = (AppsCustomizePagedView) findViewById(R.id.apps_customize_pane_content);
        mAppsCustomizePane.setPageSwitchListener(new PageSwitchListener() {

            @Override
            public void onPageSwitch(View newPage, int newPageIndex) {
                if (newPageIndex == 0) {
                    head.setVisibility(View.INVISIBLE);
                } else if (newPageIndex == mAppsCustomizePane.getPageCount() - 1) {
                    tail.setVisibility(View.INVISIBLE);
                } else {
                    head.setVisibility(View.VISIBLE);
                    tail.setVisibility(View.VISIBLE);
                }
            }
        });
    }

    void reset() {
        // if (mInTransition) {
        // // Defer to after the transition to reset
        // mResetAfterTransition = true;
        // } else {
        // Reset immediately
        mAppsCustomizePane.reset();
        // }
    }
}
/* @} */
