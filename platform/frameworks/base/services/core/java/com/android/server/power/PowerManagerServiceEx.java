package com.android.server.power;

import android.content.Context;
import android.database.ContentObserver;
import android.provider.Settings;
import android.os.IPowerManagerEx;
import android.os.ServiceManager;
import android.util.Log;

import com.android.server.display.DisplayManagerServiceEx;
import com.android.server.power.ShutdownThread;

public class PowerManagerServiceEx extends IPowerManagerEx.Stub{

    private static final String TAG = "PowerManagerServiceEx";

    private Context mContext;

    private static PowerManagerServiceEx sInstance;

    private DisplayManagerServiceEx mDisplayManagerServiceEx;

    private boolean mBootCompleted;

    public static PowerManagerServiceEx getInstance(Context context){

        synchronized(PowerManagerServiceEx.class){
            if (sInstance == null ){
                sInstance = new PowerManagerServiceEx(context);
            }
        }

        return sInstance;
    }

    public static PowerManagerServiceEx init(Context context){

        synchronized(PowerManagerServiceEx.class){
            if (sInstance == null ){
                sInstance = new PowerManagerServiceEx(context);
            }else {
                Log.wtf(TAG,"PowerManagerServiceEx has been init more than once");
            }
        }

        return sInstance;
    }

    private PowerManagerServiceEx(Context context){

        mContext = context;
        mDisplayManagerServiceEx = DisplayManagerServiceEx.getInstance(context);

        ServiceManager.addService("power_ex",this);

    }

    public void setBootCompleted(boolean bootCompleted){
        mBootCompleted = bootCompleted;
    }

    public void shutdownForAlarm(boolean confirm, boolean isPowerOffAlarm){

        ShutdownThread.shutdownForAlarm(mContext, confirm, isPowerOffAlarm);

    }

    public void rebootAnimation(){

        ShutdownThread.rebootAnimation(mContext);

    }

    public void scheduleButtonLightTimeout(long now){
        Log.v(TAG, "scheduleButtonTimeout");
        if(mDisplayManagerServiceEx != null){
        int buttonOffTimeoutSetting = PowerManagerServiceUtils.getInstance(mContext).getButtonOffTimeoutSetting();
        Log.d(TAG, "ButtonOffTimeoutSetting = " + buttonOffTimeoutSetting);
        mDisplayManagerServiceEx.updateButtonTimeout(buttonOffTimeoutSetting);
        if (mBootCompleted) {
            mDisplayManagerServiceEx.scheduleButtonTimeout(now);
          }
        }
    }

}
