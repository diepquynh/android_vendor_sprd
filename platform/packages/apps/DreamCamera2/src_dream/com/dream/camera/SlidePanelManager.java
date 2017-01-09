
package com.dream.camera;

import android.graphics.Color;
import android.view.View;
import android.view.ViewTreeObserver;

import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import com.android.camera2.R;

public class SlidePanelManager {

    private static SlidePanelManager mSlidePanelManager;
    private static CameraActivity mActivity;

    public final static int MODE = 0;
    public final static int CAPTURE = 1;
    public final static int FILTER = 2;

    private View slidePanelParent;
    private View slidePanel;

    private View modeText;
    private View captureText;
    private View filterText;

    private SlidePanelManager(CameraActivity activity) {
        mActivity = activity;
        init();
    }

    public static SlidePanelManager getInstance(CameraActivity activity) {
         /*
          * if we start camera via SecureCameraActivity, we should recreate SlidePanelManager to fresh
          * slidePanelParent
         */
        if (mSlidePanelManager == null || activity != mActivity) {
            mSlidePanelManager = new SlidePanelManager(activity);
        }
        return mSlidePanelManager;
    }

    public void init() {
        slidePanelParent = mActivity.findViewById(R.id.slide_panel_parent);
        slidePanel = mActivity.findViewById(R.id.slide_panel);

        modeText = mActivity.findViewById(R.id.sp_mode_tv);
        captureText = mActivity.findViewById(R.id.sp_capture_tv);
        filterText = mActivity.findViewById(R.id.sp_filter_tv);
    }

    public void focusItem(int id, boolean animation) {
        switch (id) {
            case MODE:
                focusItem(modeText, animation);
                break;
            case CAPTURE:
                focusItem(captureText, animation);
                break;
            case FILTER:
                focusItem(filterText, animation);
                break;
        }
    }

    public void focusItem(final View item, final boolean animation) {
        if (slidePanelParent == null || slidePanel == null || item == null ) {
            return;
        }

        int sppWidth = slidePanelParent.getWidth();

        if (sppWidth == 0) {
            if(firstItemView == null){
                final ViewTreeObserver vto = slidePanelParent.getViewTreeObserver();
                vto.addOnPreDrawListener(mOnPreDrawListener);
            }
            firstItemView = item;
            return;
        } else if (firstItemView != null) {
            final ViewTreeObserver vto = slidePanelParent.getViewTreeObserver();
            vto.removeOnPreDrawListener(mOnPreDrawListener);
            firstItemView = null;
        }

        int itemWidth = item.getWidth();
        int x = (int) item.getX();
        int diff = sppWidth / 2 - (x + itemWidth / 2);

        slidePanel.setPadding(diff, 0, 0, 0);
    }

    private View firstItemView;

    private ViewTreeObserver.OnPreDrawListener mOnPreDrawListener = new ViewTreeObserver.OnPreDrawListener() {
        public boolean onPreDraw() {
            focusItem(firstItemView, false);
            return true;
        }
    };

    public void udpateSlidePanelShow(int id, int visible) {
        switch (id) {
            case MODE:
                modeText.setVisibility(visible);
                break;
            case CAPTURE:
                captureText.setVisibility(visible);
                break;
            case FILTER:
                filterText.setVisibility(visible);
                break;
        }
    }

    public void onDestroy() {
        mSlidePanelManager = null;
        mActivity = null;
    }
}
