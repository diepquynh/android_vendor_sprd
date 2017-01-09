package com.sprd.ucam.vgesutre;

import com.android.camera2.R;

import android.content.Context;
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;
import android.widget.RelativeLayout;

public class VgestureLayout extends RelativeLayout{
    private RelativeLayout mTitle;
    private ImageView modeview;
    private ImageView mGestureButton;

    public VgestureLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        // TODO Auto-generated constructor stub
    }

@Override
protected void onConfigurationChanged(Configuration newConfig) {
    // TODO Auto-generated method stub
    /* SPRD: fix bug: 604903 vgesture help&hint image is wrong position when back from filmstrip which is showed landscape @{
    super.onConfigurationChanged(newConfig);
    checkOrientation(newConfig.orientation);
    @} */
}
/**
 * Set the layout gravity of the child layout to be top or left
 * depending on orientation.
 */
public void checkOrientation(int orientation) {
    final boolean isPortrait = (Configuration.ORIENTATION_PORTRAIT == orientation);

    final int imagemarginLeft = (int) getResources()
            .getDimension(R.dimen.vgesture_image_margin_left);
    final int imagemarginTop = (int) getResources()
            .getDimension(R.dimen.vgesture_image_margin_top);

    RelativeLayout.LayoutParams titlerlParams
        = (RelativeLayout.LayoutParams) mTitle.getLayoutParams();
    RelativeLayout.LayoutParams modeviewParams
    = (RelativeLayout.LayoutParams) modeview.getLayoutParams();
    RelativeLayout.LayoutParams getureParams
    = (RelativeLayout.LayoutParams) mGestureButton.getLayoutParams();

    if (isPortrait) {
        titlerlParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        modeviewParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        getureParams.removeRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        getureParams.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        titlerlParams.height = RelativeLayout.LayoutParams.WRAP_CONTENT;
        titlerlParams.width = RelativeLayout.LayoutParams.MATCH_PARENT;
        modeviewParams.leftMargin = imagemarginLeft;
        modeviewParams.topMargin = 0;
        getureParams.rightMargin = imagemarginLeft;
        getureParams.bottomMargin = 0;
        titlerlParams.topMargin = imagemarginTop;
        titlerlParams.leftMargin = 0;
    } else {
        titlerlParams.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        modeviewParams.addRule(RelativeLayout.ALIGN_PARENT_TOP);
        getureParams.removeRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        getureParams.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        titlerlParams.height = RelativeLayout.LayoutParams.MATCH_PARENT;
        titlerlParams.width = RelativeLayout.LayoutParams.WRAP_CONTENT;
        modeviewParams.leftMargin = 0;
        modeviewParams.topMargin = imagemarginLeft;
        getureParams.rightMargin = 0;
        getureParams.bottomMargin = imagemarginLeft;
        titlerlParams.leftMargin = imagemarginTop;
        titlerlParams.topMargin = 0;
        Log.i("vglayout","ALIGN_PARENT_LEFT");
    }

    requestLayout();
}
@Override
protected void onFinishInflate() {
    // TODO Auto-generated method stub
    mTitle = (RelativeLayout) findViewById(R.id.title_rl);
    modeview = (ImageView) mTitle.findViewById(R.id.camera_mode_view);
    mGestureButton = (ImageView) mTitle.findViewById(R.id.camera_gesture_button);
}}
