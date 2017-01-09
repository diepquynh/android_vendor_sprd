
package com.sprd.validationtools.itemstest;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

public class ScreenColorTest extends BaseActivity
{
    private String TAG = "ScreenColorTest";
    TextView mContent;
    int mIndex = 0, mCount = 0;
    private Handler mUiHandler = new Handler();;
    private Runnable mRunnable;

    private static final int[] COLOR_ARRAY = new int[] {
            Color.WHITE, Color.RED, Color.GREEN, Color.BLUE, Color.YELLOW
    };
    private static final int TIMES = 4;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContent = new TextView(this);
        mRunnable = new Runnable() {
            public void run() {
                mContent.setBackgroundColor(COLOR_ARRAY[mIndex]);
                mIndex++;
                mCount++;
                setBackground();
            }
        };
        setBackground();
        setTitle(R.string.lcd_test);
        setContentView(mContent);
        mContent.setGravity(Gravity.CENTER);
        mContent.setTextSize(35);
        super.removeButton();
    }

    private void setBackground() {
        if (mIndex >= COLOR_ARRAY.length) {
            //showResultDialog(getString(R.string.lcd_max));
            super.createButton(true);
            super.createButton(false);
            return;
        }
        mContent.setBackgroundColor(COLOR_ARRAY[mIndex]);
        mUiHandler.postDelayed(mRunnable, 600);
    }

    @Override
    public void onDestroy() {
        mUiHandler.removeCallbacks(mRunnable);
        super.onDestroy();
    }
}
