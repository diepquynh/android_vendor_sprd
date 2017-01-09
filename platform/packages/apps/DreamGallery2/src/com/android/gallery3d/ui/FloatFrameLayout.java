
package com.android.gallery3d.ui;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.FrameLayout;

public class FloatFrameLayout extends FrameLayout {

    private OnConfigurationChangedListener mConfigurationChangedListener;

    public FloatFrameLayout(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public FloatFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public FloatFrameLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        Log.d("xbin", "onConfigurationChanged--------------");
        if (mConfigurationChangedListener != null) {
            mConfigurationChangedListener.onConfigurationChanged(newConfig);
        }
        super.onConfigurationChanged(newConfig);
    }

    public void setOnConfigurationChangedListener(OnConfigurationChangedListener listener) {
        mConfigurationChangedListener = listener;
    }

    public interface OnConfigurationChangedListener {
        boolean onConfigurationChanged(Configuration newConfig);
    }

    public int getStateHeight(){
        Rect frame = new Rect();
        getWindowVisibleDisplayFrame(frame);
        return frame.top;
    }

}
