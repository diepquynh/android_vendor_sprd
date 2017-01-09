
package com.dream.camera;

import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import com.android.camera2.R;
import android.content.Context;
import android.util.AttributeSet;
import com.android.camera.ui.PreviewOverlay;

public class ZoomPanel extends LinearLayout implements PreviewOverlay.OnDreamZoomUIChangedListener {
    private static final Log.Tag TAG = new Log.Tag("ZoomPanel");
    private SeekBar mSeekbar;
    private TextView mMinZoomValue;
    private TextView mMaxZoomValue;
    private TextView mCurZoomValue;
    private int SeekBar_MAX = 100;
    private CameraActivity mActivity;

    public ZoomPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        mActivity = (CameraActivity) context;
    }

    @Override
    public void setZoomVisible(int state) {
        this.setVisibility(state);
        if(View.VISIBLE == state){
            mActivity.getCameraAppUI().updateExtendPanelUI(View.GONE);
            mActivity.getCameraAppUI().updateSidePanelUI(View.GONE);
            // SPRD: nj dream camera test debug 120
            mActivity.getCameraAppUI().updateFilterPanelUI(View.GONE);
        } else {
            mActivity.getCameraAppUI().updateExtendPanelUI(View.VISIBLE);
            mActivity.getCameraAppUI().updateSidePanelUI(View.VISIBLE);
            // SPRD: nj dream camera test debug 120
            mActivity.getCameraAppUI().updateFilterPanelUI(View.VISIBLE);
        }
    }

    @Override
    public void onFinishInflate() {
        mSeekbar = (SeekBar) findViewById(R.id.zoom_seekbar);
        mMinZoomValue = (TextView) findViewById(R.id.min_zoom_value);
        mMaxZoomValue = (TextView) findViewById(R.id.max_zoom_value);
        mCurZoomValue = (TextView) findViewById(R.id.current_zoom_value);
        Log.d(TAG, "init " + mSeekbar + "," + mCurZoomValue);
    }

    @Override
    public void initZoomLevel(float minZoomValue, float maxZoomValue, float curZoomValue) {

        int progress = (int) ((curZoomValue - minZoomValue) * SeekBar_MAX / (maxZoomValue - minZoomValue));
        curZoomValue = (float) (Math.round(curZoomValue * 10)) / 10;
        maxZoomValue = (float) (Math.round(maxZoomValue * 10)) / 10;
        mSeekbar.setProgress(progress);
        mMinZoomValue.setText("" + minZoomValue + "x");
        mMaxZoomValue.setText("" + maxZoomValue + "x");
        mCurZoomValue.setText("" + curZoomValue + "x");
    }

    @Override
    public void updateZoomLevel(float minZoomValue, float maxZoomValue, float curZoomValue) {
        int progress = (int) ((curZoomValue - minZoomValue) * SeekBar_MAX / (maxZoomValue - minZoomValue));
        curZoomValue = (float) (Math.round(curZoomValue * 10)) / 10;
        mSeekbar.setProgress(progress);

        mCurZoomValue.setText("" + curZoomValue + "x");
    }

    @Override
    public void updateZoomValueText(float curZoomValue) {
        curZoomValue = (float) (Math.round(curZoomValue * 10)) / 10;
        mCurZoomValue.setText("" + curZoomValue + "x");
    }

    public void setOnProgressChangeListener(OnSeekBarChangeListener listener) {
        mSeekbar.setOnSeekBarChangeListener(listener);
    }

    @Override
    public void setVisibility(int visibility) {
        if (mActivity != null && mActivity.getCurrentModule() != null && mActivity.getCurrentModule().isAudioRecording()) {
            super.setVisibility(View.GONE);
        } else {
            super.setVisibility(visibility);
        }
    }
}
