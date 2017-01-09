package com.android.server.display;

import android.content.Context;
import android.hardware.display.DisplayManagerInternelEx;

public class DisplayManagerServiceEx extends DisplayManagerInternelEx{

    private static final String TAG = "DisplayManagerServiceEx";

    private static DisplayManagerServiceEx sInstance;

    private Context mContext;

    private DisplayPowerControllerEx mDisplayPowerControllerEx;

    private DisplayManagerServiceEx(Context context){
        mContext = context;
        mDisplayPowerControllerEx = DisplayPowerControllerEx.getInstance(context);
    }

    public static DisplayManagerServiceEx getInstance(Context context){

        synchronized(DisplayManagerServiceEx.class){
            if (sInstance == null ){
                sInstance = new DisplayManagerServiceEx(context);
            }
        }

        return sInstance;
    }

    @Override
    public void scheduleButtonTimeout(long now) {
        mDisplayPowerControllerEx.scheduleButtonTimeout(now);
    }

    @Override
    public void updateButtonTimeout(int timeout) {
        mDisplayPowerControllerEx.updateButtonTimeout(timeout);
    }
}
