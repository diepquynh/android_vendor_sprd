package com.android.server.display;

import android.content.Context;
import android.util.Log;

public class DisplayPowerControllerEx extends  AbsDisplayPowerController{

    private static final String TAG = "DisplayPowerControllerEx";

    private int mButtonTimeout;

    private Context mContext;

    private DisplayPowerStateEx mPowerState;

    private static DisplayPowerControllerEx sInstance;

    private DisplayPowerControllerEx(Context context){

        mContext = context;
        mPowerState = DisplayPowerStateEx.getInstance(context);

    }

    public static DisplayPowerControllerEx getInstance(Context context){

        synchronized(DisplayPowerControllerEx.class){
            if (sInstance == null ){
                sInstance = new DisplayPowerControllerEx(context);
            }
        }

        return sInstance;
    }

    @Override
    public void scheduleButtonTimeout(long now) {
        if(mPowerState != null){
            Log.d(TAG, "scheduleButtonTimeout");
            mPowerState.scheduleButtonTimeout(now);
        }
    }

    @Override
    public void updateButtonTimeout(int timeout) {
        Log.d(TAG, "timeout = " + timeout);
        mButtonTimeout = timeout;
        if(mPowerState != null){
            mPowerState.updateButtonTimeout(mButtonTimeout);
        }
    }

    @Override
    public int getButtonTimeout() {
        return mButtonTimeout;
    }

    @Override
    public void initButtonLight() {
        updateButtonTimeout(mButtonTimeout);
        if(mButtonTimeout != -2){
            Log.d(TAG,"BUTTON LIGHT TURN ON");
            mPowerState.setButtonOn(true);
        } else {
            Log.d(TAG,"BUTTON LIGHT TURN OFF");
            mPowerState.setButtonOn(false);
        }
    }
}
