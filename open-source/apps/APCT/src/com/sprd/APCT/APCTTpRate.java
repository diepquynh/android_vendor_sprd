package com.sprd.APCT;

import com.sprd.APCT.R;
import java.util.ArrayList;
import java.util.List;
import android.content.Context;
//import android.content.res.Resources;
//import android.content.res.TypedArray;
import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
//import android.view.Window;
import android.widget.AbsListView;
import android.view.View.OnClickListener;

import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import java.util.HashMap;
import java.util.Map;

import android.provider.Settings;
import android.content.ContentResolver;
import android.util.Log;
import android.content.Intent;
import java.io.*;
import android.os.SystemClock;
import android.view.MotionEvent;

public class APCTTpRate extends Activity{

    static final String TAG = "APCT";
	boolean mOldShowTouches = false;
	boolean mShowTouches    = false;
	long    mTpCount        = 0;
	long    mTpRate         = 0;
	boolean mIsTpDown       = false;
	static long mNowTime    = 0;
	static long mLastTime   = 0;
	static long mRecTime    = 0;
	static long[] mDelTime  = new long[10];
	static int mDelCount    = 0;
	TextView mTpRateView    = null;
	TextView mTpCurRateView = null;
	TextView mPromptView    = null;
	TextView mTpDeltaTime   = null;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.tprate);

		mOldShowTouches = Settings.System.getInt(getContentResolver(), Settings.System.SHOW_TOUCHES, 0) != 0;

        mShowTouches = mOldShowTouches;

		if (!mOldShowTouches)
		{
		    mShowTouches = true;
		    Settings.System.putInt(getContentResolver(),Settings.System.SHOW_TOUCHES, 1);
		}

		mTpRateView    = (TextView)findViewById(R.id.TpRateTxt);
		mTpCurRateView = (TextView)findViewById(R.id.TpCurRateTxt);		
		mPromptView    = (TextView)findViewById(R.id.promptTxt);
		mTpDeltaTime   = (TextView)findViewById(R.id.TpDeltaTime);
		mPromptView.setText("Please Move the TP on the screen for enough long time");
	}

	@Override
	protected void onPause()
	{
	    super.onPause();

	    if (!mOldShowTouches)
	    {
   	        mShowTouches = false;
	        Settings.System.putInt(getContentResolver(),Settings.System.SHOW_TOUCHES, 0);
	    }
	}

	@Override
	protected void onResume()
	{
	    super.onResume();

	    if (!mShowTouches)
	    {
	        Settings.System.putInt(getContentResolver(),Settings.System.SHOW_TOUCHES, 1);
	    }
	}

	@Override
	public boolean onTouchEvent(MotionEvent event)
	{
	    switch(event.getAction() & MotionEvent.ACTION_MASK)
	    {
	    case MotionEvent.ACTION_DOWN:
	    mTpCount = 0;
	    mRecTime = SystemClock.elapsedRealtime();
	    break;

	    case MotionEvent.ACTION_MOVE:
	    mTpCount++;
	    mDelCount++;
	    mNowTime = SystemClock.elapsedRealtime();
	    mDelTime[mDelCount%10] = mNowTime - mRecTime;
	    mRecTime = SystemClock.elapsedRealtime();
	    mTpDeltaTime.setText("10 Times Delta Time:" + mDelTime[0] + "," + mDelTime[1] + "," + mDelTime[2] + ","
	                            + mDelTime[3] + "," + mDelTime[4] + "," + mDelTime[5] + "," + mDelTime[6] + "," 
	                            + mDelTime[7] + "," + mDelTime[8] + "," + mDelTime[9]);

	    if (mNowTime - mLastTime >= 1000)
	    {
	        if (mTpRate < mTpCount)
	        {
	            mTpRate = mTpCount;
          	    mTpRateView.setText("TP MAX Report Rate(1s):" + Long.toString(mTpRate));
	        }

	        mLastTime = mNowTime;
	        mTpCurRateView.setText("TP Current Report Rate(1s):" + Long.toString(mTpCount));
	        mTpCount = 0;
	    }
	    break;

	    case MotionEvent.ACTION_UP:
	    break;

	    default:
	    break;
	    }

	    return true;
	}
}
