
package com.sprd.validationtools.itemstest;

import java.util.Timer;
import java.util.TimerTask;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Binder;
import android.os.Bundle;
import android.os.PowerManager;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.os.Handler;
import android.util.Log;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.TextView;

public class BackLightTest extends BaseActivity
{
    private static final String TAG = "BackLightTest";
    PowerManager mPowerManager = null;
    TextView mContent;
    private static final int[] COLOR_ARRAY = new int[] {
            Color.WHITE, Color.BLACK
    };
    private static final boolean[] KEY_BG_LIGHT_ARRAY = new boolean[] {
            true, false
    };
    private int mIndex = 0, mCount = 0;
    Timer mTimer;
    private static final int TIMES = 3;
    private Handler mUiHandler = new Handler();;
    private Runnable mRunnable;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContent = new TextView(this);
        setContentView(mContent);
        setTitle(R.string.backlight_test);
        mPowerManager = (PowerManager) getSystemService(POWER_SERVICE);
        mRunnable = new Runnable() {
            public void run() {
                mContent.setBackgroundColor(COLOR_ARRAY[mIndex]);
                //mPowerManager.engMode_ButtonOn(KEY_BG_LIGHT_ARRAY[mIndex]);
                mIndex = 1 - mIndex;
                mCount++;
                setBackground();
            }
        };

        setBackground();
    }

    private void setBackground() {
        if (mCount > TIMES) {
           // showResultDialog(getString(R.string.backlight_des));
            return;
        }
        mUiHandler.postDelayed(mRunnable, 600);
    }

    @Override
    public void onDestroy() {
        mUiHandler.removeCallbacks(mRunnable);
        super.onDestroy();
    }
}
