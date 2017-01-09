package com.android.server.display;

import android.content.Context;
import android.os.Handler;
import android.util.Slog;

import com.android.server.LocalServices;
import com.android.server.lights.Light;
import com.android.server.lights.LightsManager;

public class DisplayPowerStateEx extends AbsDisplayPowerState{

    private static final String TAG = "DisplayPowerStateEx";

    private static DisplayPowerStateEx sInstance;

    /* SPRD: definitions variables. @{ */
    private final Light mButtonlight;
    private long buttontimeout;
    private int INITIAL_BUTTON_BRIGHTNESS = 0;
    private int INITIAL_BUTTON_MAX_BRIGHTNESS = 255;

    private final Handler mHandler;

    private DisplayPowerStateEx(Context context){

        mHandler = new Handler(true /*async*/);
        mButtonlight = LocalServices.getService(LightsManager.class).getLight(LightsManager.LIGHT_ID_BUTTONS);

    }

    public static DisplayPowerStateEx getInstance(Context context){

        synchronized(DisplayPowerStateEx.class){
            if (sInstance == null ){
                sInstance = new DisplayPowerStateEx(context);
            }
        }

        return sInstance;
    }

    /* SPRD: Button lights off timeout. @{ */
    public void scheduleButtonTimeout(long now) {
        mHandler.removeCallbacks(mButtonTimeoutTask);
        Slog.d(TAG, "scheduleButtonTimeout buttontimeout == " + buttontimeout);
        //SPRD: if the mButtonOffTimeoutSetting is greater than zero setting key lights timeout.
        if(buttontimeout > 0){
            setButtonOn(true);
            long when = now + buttontimeout;
            mHandler.postAtTime(mButtonTimeoutTask, when);
        } else {
            /*SPRD: if the mButtonOffTimeoutSetting equal -1,set key light always on.
             *if the mButtonOffTimeoutSetting equal -2,set key light always off.
             *@{
             */
            if(buttontimeout == -1){
                setButtonOn(true);
            } else if(buttontimeout == -2){
                setButtonOn(false);
            }
            /*@}*/
        }
    }

    private Runnable mButtonTimeoutTask = new Runnable() {
        public void run() {
            setButtonOn(false);
        }
    };
    public void updateButtonTimeout(int timeout){
        buttontimeout = timeout;
    }
    /* @} */

    /* SPRD :Sets whether the button is on or off. @{ */
    public void setButtonOn(boolean on) {
        if(on) {
            mButtonlight.setBrightness(INITIAL_BUTTON_MAX_BRIGHTNESS);
        } else {
            mButtonlight.setBrightness(INITIAL_BUTTON_BRIGHTNESS);
        }
    }
    /* @} */
}
