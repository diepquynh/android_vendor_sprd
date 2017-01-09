
package com.sprd.validationtools.itemstest;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.sprd.validationtools.BaseActivity;

import android.app.ListActivity;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.widget.TextView;
import java.text.SimpleDateFormat;
import com.sprd.validationtools.R;

public class RTCTest extends BaseActivity
{
    private static final String TAG = "RTCTest";
    TextView mContent;
    public static final String timeFormat = "hh:mm:ss";
    public static final SimpleDateFormat TIME_FORMAT = new SimpleDateFormat(timeFormat);
    public long mTime;
    public Handler mHandler = new Handler();
    public Runnable mRunnable = new Runnable() {
        public void run() {
            showResultDialog(getString(R.string.rtc_test));
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContent = new TextView(this);
        mContent.setGravity(Gravity.CENTER);
        setContentView(mContent);
        setTitle(R.string.rtc_test);
        mTime = System.currentTimeMillis();
        setTimeText();
       // mHandler.postDelayed(mRunnable, 3000);
    }

    private void setTimeText() {
        mContent.postDelayed(new Runnable() {
            public void run() {
                mContent.setText(getResources().getText(R.string.rtc_tag) + getTime());
                mContent.setTextSize(35);
                if (System.currentTimeMillis() - mTime > 3000) {
                }
                else {
                    setTimeText();
                }
            }
        }, 100);
    }

    private String getTime() {
        return TIME_FORMAT.format(new Date());
    }

    @Override
    public void onDestroy() {
        mHandler.removeCallbacks(mRunnable);
        super.onDestroy();
    }

//    @Override
//    public void onBackPressed() {
//        storeRusult(true);
//        finish();
//    }
}
