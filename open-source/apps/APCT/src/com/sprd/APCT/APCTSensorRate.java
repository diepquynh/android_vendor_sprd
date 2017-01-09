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
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

public class APCTSensorRate extends Activity implements SensorEventListener {

    static final String TAG      = "APCT";
    long        mSensorCount     = 0;
	long        mSensorRate      = 0;
	static long mNowTime         = 0;
	static long mLastTime        = 0;
	TextView    mMaxRateView     = null;
	TextView    mCurRateView     = null;
	TextView    mPromptView      = null;
    private SensorManager mSensorMgr;
    private Sensor        mSensor;
    private float         mX     = 0;
    private float         mY     = 0;
    private float         mZ     = 0;    
    private float         mLastX = 0;
    private float         mLastY = 0;
    private float         mLastZ = 0;    

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.sensorrate);

        mSensorMgr = (SensorManager) getSystemService(SENSOR_SERVICE);

        mSensor = mSensorMgr.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		mMaxRateView = (TextView)findViewById(R.id.SensorRateTxt);
		mCurRateView = (TextView)findViewById(R.id.SensorCurRateTxt);
		mPromptView = (TextView)findViewById(R.id.promptTxt);
		mPromptView.setText("Please shake the phone violently...");
	}

    public void onSensorChanged(SensorEvent e)
    {
        mNowTime = SystemClock.elapsedRealtime();

        mX = e.values[SensorManager.DATA_X];
        mY = e.values[SensorManager.DATA_Y];
        mZ = e.values[SensorManager.DATA_Z];

        if (mX != mLastX && mY != mLastY && mZ != mLastZ)
        {
            mSensorCount++;
            mLastX = mX;
            mLastY = mY;
            mLastZ = mZ;
        }

        if (mNowTime - mLastTime >= 2000)
        {
            if (mSensorRate < mSensorCount)
            {
                mSensorRate = mSensorCount;
                mMaxRateView.setText("Sensor MAX Report Rate(2s):" + Long.toString(mSensorRate));
            }

            mLastTime = mNowTime;
	        mCurRateView.setText("Sensor Current Report Rate(2s):" + Long.toString(mSensorCount));
            mSensorCount = 0;
        }
    }

    public void onAccuracyChanged(Sensor s, int accuracy)
    {
    }

	@Override
	protected void onPause()
	{
	    super.onPause();
        mSensorMgr.unregisterListener(this);
	}

	@Override
	protected void onResume()
	{
	    super.onResume();
        mSensorMgr.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
	}
}
